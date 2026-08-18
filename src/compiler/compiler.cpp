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

llvm::Type* Compiler::get_type_for_token(const TokenType& type) const {
    switch (type) {
    case I8:
        return builder->getInt8Ty();
    case I16:
        return builder->getInt16Ty();
    case I32:
        return builder->getInt32Ty();
    case I64:
        return builder->getInt64Ty();
    case F32:
        return builder->getFloatTy();
    case F64:
        return builder->getDoubleTy();
    case BOOL_TYPE:
        return builder->getInt1Ty();
    case STR_TYPE:
        return builder->getPtrTy();
    default:
        return nullptr;
    }
}

llvm::Value* Compiler::cast_value(llvm::Value* val, llvm::Type* targetType) const {
    if (!val || !targetType)
        return val;

    llvm::Type* srcType = val->getType();
    if (srcType == targetType)
        return val;

    // both integers
    if (srcType->isIntegerTy() && targetType->isIntegerTy()) {
        unsigned srcBits = srcType->getIntegerBitWidth();
        unsigned dstBits = targetType->getIntegerBitWidth();
        if (srcBits > dstBits)
            return builder->CreateTrunc(val, targetType, "trunc");

        // assume signed extension for now
        //  sext copies the MSB into the padding bits, this is good for signed ints, but bad for unsigned
        // FIXME use zext (zero extend) for unsigned
        return builder->CreateSExt(val, targetType, "sext");
    }

    // both floats
    if (srcType->isFloatingPointTy() && targetType->isFloatingPointTy()) {
        if (srcType->isDoubleTy() && targetType->isFloatTy())
            return builder->CreateFPTrunc(val, targetType, "fptrunc");

        return builder->CreateFPExt(val, targetType, "fpext");
    }

    // float to int
    if (srcType->isFloatingPointTy() && targetType->isIntegerTy())
        return builder->CreateFPToSI(val, targetType, "fptosi");

    // int to float
    if (srcType->isIntegerTy() && targetType->isFloatingPointTy())
        return builder->CreateSIToFP(val, targetType, "sitofp");

    // FIXME improve error logging
    std::cerr << "Unsupported cast" << std::endl;
    return val;
}

void Compiler::coerce_types(llvm::Value*& lhs, llvm::Value*& rhs) const {
    if (!lhs || !rhs)
        return;
    llvm::Type* lhsType = lhs->getType();
    llvm::Type* rhsType = rhs->getType();
    if (lhsType == rhsType)
        return;

    // both integers
    if (lhsType->isIntegerTy() && rhsType->isIntegerTy()) {
        unsigned lhsBits = lhsType->getIntegerBitWidth();
        unsigned rhsBits = rhsType->getIntegerBitWidth();
        if (lhsBits < rhsBits)
            lhs = builder->CreateSExt(lhs, rhsType, "sext");
        else
            rhs = builder->CreateSExt(rhs, lhsType, "sext");
        return;
    }

    // both floats
    if (lhsType->isFloatingPointTy() && rhsType->isFloatingPointTy()) {
        if (lhsType->isFloatTy() && rhsType->isDoubleTy())
            lhs = builder->CreateFPExt(lhs, rhsType, "fpext");
        else if (lhsType->isDoubleTy() && rhsType->isFloatTy())
            rhs = builder->CreateFPExt(rhs, lhsType, "fpext");
        return;
    }

    // int and float
    if (lhsType->isIntegerTy() && rhsType->isFloatingPointTy()) {
        lhs = builder->CreateSIToFP(lhs, rhsType, "sitofp");
        return;
    }
    if (lhsType->isFloatingPointTy() && rhsType->isIntegerTy()) {
        rhs = builder->CreateSIToFP(rhs, lhsType, "sitofp");
        return;
    }
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
    return std::visit([this]<typename E>(E&& value) -> llvm::Value* {
        if constexpr (std::is_same_v<std::decay_t<E>, RealNumber>) {
            if (value.n_fractional_digits == 0) // if int
                // TODO implement returning a 32 bit int
                return llvm::ConstantInt::get(this->builder->getInt64Ty(), value.integer, value.is_negative);

            // if float
            return llvm::ConstantFP::get(this->builder->getDoubleTy(), llvm::APFloat(value.to_double()));
        }

        std::cout << "Could not convert to number" << std::endl; // TODO improve error logging
        return nullptr;
    }, astNode->token.get_literal());
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
        lastValue = cast_value(lastValue, builder->getDoubleTy());
        builder->CreateRet(lastValue);
    } else
        builder->CreateRet(llvm::ConstantFP::get(*globalCtx, llvm::APFloat(0.0)));
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