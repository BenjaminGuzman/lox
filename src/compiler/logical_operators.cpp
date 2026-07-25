#include "../include/compiler.h"

namespace lox {

llvm::Value* Compiler::compileTrue(const std::unique_ptr<ASTNode>& astNode) const {
    return builder->getInt1(true);
}

llvm::Value* Compiler::compileFalse(const std::unique_ptr<ASTNode>& astNode) const {
    return builder->getInt1(false);
}

llvm::Value* Compiler::compileNot(const std::unique_ptr<ASTNode>& astNode) const {
    llvm::Value* operand = this->compile(astNode->children[0]);
    if (!operand)
        return nullptr;

    // if it's an int or double, compare against 0 to treat 0 as false and others as true
    if (operand->getType()->isIntegerTy()) {
        llvm::Value* zero = llvm::ConstantInt::get(*globalCtx, llvm::APInt(1, 0));
        operand = builder->CreateICmpNE(operand, zero, "notcmp");
    } else if (operand->getType()->isDoubleTy()) {
        llvm::Value* zero = llvm::ConstantFP::get(*globalCtx, llvm::APFloat(0.0));
        operand = builder->CreateFCmpONE(operand, zero, "notcmp");
    }

    // now operand should be i1, but if it is already i1, CreateNot works directly
    if (operand->getType()->isIntegerTy(1)) {
        return builder->CreateNot(operand, "nottmp");
    }

    std::cerr << "Unsupported type for unary ! operator" << std::endl;
    return nullptr;
}

} // lox
