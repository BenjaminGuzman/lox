#include "../include/compiler.h"
#include "scanner/real_number.h"

namespace lox {

llvm::Value* Compiler::compilePlus(const std::unique_ptr<ASTNode>& astNode) const {
    auto opType = astNode->must_be_op_type
                        .or_else([]() {return std::optional<TokenOpType>(UNARY);})
                        .value();
    if (opType == BINARY) {
        llvm::Value* lhs = this->compile(astNode->children[0]);
        llvm::Value* rhs = this->compile(astNode->children[1]);
        if (!lhs || !rhs)
            return nullptr;

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

} // lox
