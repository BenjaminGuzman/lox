#include "compiler.h"

namespace lox {

llvm::Value* Compiler::compileComparison(const std::unique_ptr<ASTNode>& astNode) const {
    llvm::Value* lhs = this->compile(astNode->children[0]);
    llvm::Value* rhs = this->compile(astNode->children[1]);
    if (!lhs || !rhs)
        return nullptr;

    switch (astNode->token.type) {
    case LT:
        return builder->CreateFCmpOLT(lhs, rhs, "lttmp");
    case LTE:
        return builder->CreateFCmpOLE(lhs, rhs, "letmp");
    case GT:
        return builder->CreateFCmpOGT(lhs, rhs, "gttmp");
    case GTE:
        return builder->CreateFCmpOGE(lhs, rhs, "getmp");
    default:
        return nullptr;
    }
}

llvm::Value* Compiler::compileEquality(const std::unique_ptr<ASTNode>& astNode) const {
    llvm::Value* lhs = this->compile(astNode->children[0]);
    llvm::Value* rhs = this->compile(astNode->children[1]);
    if (!lhs || !rhs)
        return nullptr;

    if (lhs->getType()->isDoubleTy() && rhs->getType()->isDoubleTy()) {
        if (astNode->token.type == EQEQ)
            return builder->CreateFCmpOEQ(lhs, rhs, "eqtmp");

        // NEQ
        return builder->CreateFCmpONE(lhs, rhs, "neqtmp");
    }

    if (lhs->getType()->isIntegerTy() && rhs->getType()->isIntegerTy()) {
        if (astNode->token.type == EQEQ)
            return builder->CreateICmpEQ(lhs, rhs, "eqtmp");

        // NEQ
        return builder->CreateICmpNE(lhs, rhs, "neqtmp");
    }

    // compare strings
    if (lhs->getType()->isPointerTy() && rhs->getType()->isPointerTy()) {
        llvm::FunctionType* funcType = llvm::FunctionType::get(
            builder->getInt1Ty(),
            {builder->getPtrTy(), builder->getPtrTy()},
            false
        );
        llvm::FunctionCallee eqFunc = globalModule->getOrInsertFunction("lox_string_eq", funcType);
        llvm::Value* eqRes = builder->CreateCall(eqFunc, {lhs, rhs}, "streqtmp");
        
        if (astNode->token.type == EQEQ)
            return eqRes;
            
        // NEQ
        return builder->CreateNot(eqRes, "strneqtmp");
    }
    
    // for mismatching types (e.g., bool == double), lox considers them unequal
    if (astNode->token.type == EQEQ)
        return builder->getInt1(false);

    return builder->getInt1(true);
}

} // lox
