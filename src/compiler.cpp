#include "../include/compiler.h"
#include "scanner/real_number.h"

namespace lox {
Compiler::Compiler() {
    globalCtx = std::make_unique<llvm::LLVMContext>();
    globalModule = std::make_unique<llvm::Module>("LoxJIT", *globalCtx);
    builder = std::make_unique<llvm::IRBuilder<>>(*globalCtx);

    // Creamos una función "main" para contener el código evaluado
    llvm::FunctionType* funcType = llvm::FunctionType::get(builder->getDoubleTy(), false);
    llvm::Function* mainFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "main", globalModule.get());

    // Creamos un bloque básico de entrada y posicionamos el builder ahí
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*globalCtx, "entry", mainFunc);
    builder->SetInsertPoint(entry);
}

llvm::Value* Compiler::compile(const std::unique_ptr<ASTNode>& root) const {
    return TOKEN_COMPILERS.at(root->token.type)(root);
}

llvm::Value* Compiler::compileNumber(const std::unique_ptr<ASTNode>& astNode) const {
    double value = std::get<RealNumber>(astNode->token.literal).to_double();
    return llvm::ConstantFP::get(*globalCtx, llvm::APFloat(value));
}

llvm::Value* Compiler::compilePlus(const std::unique_ptr<ASTNode>& astNode) const {
    auto opType = astNode->must_be_op_type
                        .or_else([]() {return std::optional<TokenOpType>(UNARY);})
                        .value();
    if (opType == BINARY) {
        llvm::Value* lhs = this->compile(astNode->children[0]);
        llvm::Value* rhs = this->compile(astNode->children[1]);
        return builder->CreateFAdd(lhs, rhs, "add_"); // TODO complete the name
    }

    // compile the unary token, compile the + (something), e.g., + + + 123.5
    return this->compile(astNode->children[0]);
}

llvm::Value* Compiler::compileMinus(const std::unique_ptr<ASTNode>& astNode) const {
    auto opType = astNode->must_be_op_type
                        .or_else([]() {return std::optional<TokenOpType>(UNARY);})
                        .value();
    if (opType == BINARY) {
        llvm::Value* lhs = this->compile(astNode->children[0]);
        llvm::Value* rhs = this->compile(astNode->children[1]);
        return builder->CreateFSub(lhs, rhs, "substraction_"); // TODO complete the name
    }

    // compile the unary token, compile the - (something), e.g., - - - 123.5
    // TODO How to negate the value? Is there an LLVM construct to negate?
    return this->compile(astNode->children[0]);
}

llvm::Value* Compiler::compileASTRoot(const std::unique_ptr<ASTNode> &root) const {
    llvm::Value* lastValue = nullptr;
    for (auto&& ast_node : root->children)
        lastValue = this->compile(ast_node);

    // Si hay un valor, creamos el retorno de la función
    if (lastValue)
        builder->CreateRet(lastValue);

    globalModule->print(llvm::errs(), nullptr);
    return lastValue;
}
} // lox