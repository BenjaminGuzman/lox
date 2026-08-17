#include "../include/compiler.h"
#include "scanner/real_number.h"
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/ExecutionEngine/JITEventListener.h>
#include <llvm-c/ExecutionEngine.h>

namespace lox {
llvm::Value* Compiler::to_bool(llvm::Value *value) const {
    if (!value)
        return builder->getInt1(false);

    if (value->getType()->isDoubleTy()) {
        llvm::Value* zero = llvm::ConstantFP::get(value->getType(), llvm::APFloat(0.0));
        return builder->CreateFCmpONE(value, zero, "truthy");
    }

    if (value->getType()->isPointerTy())
        return builder->getInt1(true); // a pointer that's not null is always true

    if (value->getType()->isIntegerTy()) {
        if (value->getType()->isIntegerTy(1))
            return value;
        llvm::Value* zero = llvm::ConstantInt::get(value->getType(), 0);
        return builder->CreateICmpNE(value, zero, "truthy");
    }

    // TODO improve error logging
    std::cerr << "Unsupported type for conversion to boolean" << std::endl;
    return nullptr;
}

Compiler::Compiler() {
    globalCtx = std::make_unique<llvm::LLVMContext>();
    globalModule = std::make_unique<llvm::Module>("LoxJIT", *globalCtx);
    builder = std::make_unique<llvm::IRBuilder<>>(*globalCtx);

    // create "main" function to contain the interpreted code
    llvm::FunctionType* funcType = llvm::FunctionType::get(builder->getDoubleTy(), false);
    llvm::Function* mainFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "main", globalModule.get());

    // create initial block from which we'll start executing the code from
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*globalCtx, "entry", mainFunc);
    builder->SetInsertPoint(entry);
    
    // push global scope
    pushScope();
}

llvm::Value* Compiler::compile(const std::unique_ptr<ASTNode>& root) const {
    if (!TOKEN_COMPILERS.contains(root->token.type)) {
        std::cerr << "Compiler for token type " << root->token.lexeme << " is not implemented in LLVM yet.\n";
        return nullptr;
    }
    return TOKEN_COMPILERS.at(root->token.type)(root);
}

llvm::Value* Compiler::compileNumber(const std::unique_ptr<ASTNode>& astNode) const {
    double value = std::get<RealNumber>(astNode->token.literal).to_double();
    return llvm::ConstantFP::get(*globalCtx, llvm::APFloat(value));
}

llvm::Value* Compiler::compileGroup(const std::unique_ptr<ASTNode>& astNode) const {
    if (astNode->children.empty())
        return nullptr;

    return this->compile(astNode->children[0]);
}

llvm::Value* Compiler::compileLeftBrace(const std::unique_ptr<ASTNode>& astNode) const {
    pushScope();

    llvm::Value* lastValue = nullptr;
    for (auto&& stmt : astNode->children)
        lastValue = this->compile(stmt);

    popScope();
    return lastValue;
}

llvm::Value* Compiler::compileASTRoot(const std::unique_ptr<ASTNode> &root) const {
    llvm::Value* lastValue = nullptr;
    for (auto&& ast_node : root->children)
        lastValue = this->compile(ast_node);

    if (lastValue) { // return something iff the return value is actually something (not nil)
        if (lastValue->getType()->isIntegerTy(1)) {
            lastValue = builder->CreateUIToFP(lastValue, builder->getDoubleTy(), "booltodouble");
        }
        builder->CreateRet(lastValue);
    }

    globalModule->print(llvm::errs(), nullptr);
    return lastValue;
}

extern "C" void lox_free_strings();

llvm::GenericValue Compiler::runJIT() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    LLVMLinkInMCJIT();

    // indicate the OS to search for symbols (e.g., lox_concat_string) not in an external
    // library (e.g., .so) but within the current executable
    llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);

    std::string errStr;
    llvm::ExecutionEngine* EE = llvm::EngineBuilder(std::move(globalModule))
        .setErrorStr(&errStr)
        .setEngineKind(llvm::EngineKind::JIT)
        .create();

    if (!EE) {
        std::cerr << "Failed to construct ExecutionEngine: " << errStr << "\n";
        return {};
    }

    EE->finalizeObject();

#ifndef NDEBUG
    // register GDB and LLDB listeners only in debug builds
    EE->RegisterJITEventListener(llvm::JITEventListener::createGDBRegistrationListener());
    EE->RegisterJITEventListener(llvm::JITEventListener::createIntelJITEventListener()); // often useful alongside GDB depending on arch
#endif
    
    llvm::Function* mainFunc = EE->FindFunctionNamed("main");
    if (!mainFunc) {
        std::cerr << "Main function not found.\n";
        return {};
    }

    llvm::GenericValue result = EE->runFunction(mainFunc, {});
    
    // free all dynamically allocated strings during the JIT execution
    lox_free_strings();
    
    return result;
}
} // lox