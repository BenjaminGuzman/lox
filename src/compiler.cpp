#include "../include/compiler.h"
#include "scanner/real_number.h"

namespace lox {
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

llvm::Value* Compiler::compilePlus(const std::unique_ptr<ASTNode>& astNode) const {
    auto opType = astNode->must_be_op_type
                        .or_else([]() {return std::optional<TokenOpType>(UNARY);})
                        .value();
    if (opType == BINARY) {
        llvm::Value* lhs = this->compile(astNode->children[0]);
        llvm::Value* rhs = this->compile(astNode->children[1]);
        if (!lhs || !rhs) return nullptr;
        return builder->CreateFAdd(lhs, rhs, "addtmp");
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
        if (!lhs || !rhs) return nullptr;
        return builder->CreateFSub(lhs, rhs, "subtmp");
    }

    // unary minus
    llvm::Value* operand = this->compile(astNode->children[0]);
    if (!operand)
        return nullptr;

    return builder->CreateFNeg(operand, "negtmp");
}

llvm::Value* Compiler::compileStar(const std::unique_ptr<ASTNode>& astNode) const {
    llvm::Value* lhs = this->compile(astNode->children[0]);
    llvm::Value* rhs = this->compile(astNode->children[1]);
    if (!lhs || !rhs)
        return nullptr;

    return builder->CreateFMul(lhs, rhs, "multmp");
}

llvm::Value* Compiler::compileSlash(const std::unique_ptr<ASTNode>& astNode) const {
    llvm::Value* lhs = this->compile(astNode->children[0]);
    llvm::Value* rhs = this->compile(astNode->children[1]);
    if (!lhs || !rhs)
        return nullptr;

    return builder->CreateFDiv(lhs, rhs, "divtmp");
}

llvm::Value* Compiler::compileGroup(const std::unique_ptr<ASTNode>& astNode) const {
    if (astNode->children.empty())
        return nullptr;

    return this->compile(astNode->children[0]);
}

llvm::Value* Compiler::compileASTRoot(const std::unique_ptr<ASTNode> &root) const {
    llvm::Value* lastValue = nullptr;
    for (auto&& ast_node : root->children)
        lastValue = this->compile(ast_node);

    if (lastValue) // return something iff the return value is actually something (not nil)
        builder->CreateRet(lastValue);

    globalModule->print(llvm::errs(), nullptr);
    return lastValue;
}
} // lox