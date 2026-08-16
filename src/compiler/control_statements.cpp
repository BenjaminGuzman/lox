#include "compiler.h"

namespace lox {

llvm::Value* Compiler::compileIf(const std::unique_ptr<ASTNode>& astNode) const {
    if (astNode->children.size() < 2) {
        // this->registerError("Invalid if statement", astNode->token);
        return nullptr;
    }

    bool has_else = astNode->children.size() == 3;
    llvm::Value* cond = this->compile(astNode->children[0]);

    // convert condition to bool (i1)
    cond = this->to_bool(cond);
    if (!cond)
        return nullptr;

    llvm::Function* theFunction = builder->GetInsertBlock()->getParent();
    
    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*globalCtx, "then", theFunction);
    llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(*globalCtx, "else");
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*globalCtx, "ifcont");
    
    builder->CreateCondBr(cond, thenBB, has_else ? elseBB : mergeBB);

    // emit "then" block
    builder->SetInsertPoint(thenBB);
    auto _ = this->compile(astNode->children[1]);
    thenBB = builder->GetInsertBlock(); // the then block could have modified the current block, so we get it again
    if (!thenBB->getTerminator()) // we only create a branch to merge if the block doesn't already have a terminator (like return)
        builder->CreateBr(mergeBB);

    // emit "else" block
    if (has_else) {
        theFunction->insert(theFunction->end(), elseBB);
        builder->SetInsertPoint(elseBB);
        _ = this->compile(astNode->children[2]);
        elseBB = builder->GetInsertBlock();
        if (!elseBB->getTerminator())
            builder->CreateBr(mergeBB);
    }

    // emit "merge" block
    theFunction->insert(theFunction->end(), mergeBB);
    builder->SetInsertPoint(mergeBB);

    // since if statements in Lox are statements and not expressions, they don't return a value
    // we return null or a dummy value. Lox scripts generally don't assign `if` blocks to variables
    return builder->getInt1(false);
}

llvm::Value* Compiler::compileWhile(const std::unique_ptr<ASTNode>& astNode) const {
    if (astNode->children.size() < 2) {
        // this->registerError("Invalid while statement", astNode->token);
        return nullptr;
    }

    llvm::Function* theFunction = builder->GetInsertBlock()->getParent();
    
    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(*globalCtx, "whilecond", theFunction);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(*globalCtx, "whilebody");
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(*globalCtx, "whileend");
    
    builder->CreateBr(condBB); // create the label/branch from which the while loop starts
    
    // condition block
    builder->SetInsertPoint(condBB);
    llvm::Value* cond = this->compile(astNode->children[0]);

    cond = this->to_bool(cond);
    if (!cond)
        return nullptr;
    
    builder->CreateCondBr(cond, bodyBB, endBB);
    
    // body block
    theFunction->insert(theFunction->end(), bodyBB);
    builder->SetInsertPoint(bodyBB);
    const auto _ = this->compile(astNode->children[1]);
    
    // jump back to condition (if while loop was not broken or returned from it)
    if (!builder->GetInsertBlock()->getTerminator())
        builder->CreateBr(condBB);
    
    // end block
    theFunction->insert(theFunction->end(), endBB);
    builder->SetInsertPoint(endBB);
    
    return builder->getInt1(false);
}

llvm::Value* Compiler::compileFor(const std::unique_ptr<ASTNode>& astNode) const {
    const auto& forHeader = astNode->children[0];
    if (forHeader->children.size() != 3) {
        //this->registerError("Invalid for statement. Expected 3 things: init, condition, increment. "
        //                    "This may be an issue with lox parser", astNode->token);
        return nullptr;
    }
    
    pushScope(); // scope for the loop variable

    const auto& initNode = forHeader->children[0];
    const auto& conditionNode = forHeader->children[1];
    const auto& incrementNode = forHeader->children[2];
    
    // compile init (if not IGNORE)
    if (initNode->token.type != IGNORE)
        const auto _ = this->compile(initNode);
    
    llvm::Function* theFunction = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(*globalCtx, "forcond", theFunction);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(*globalCtx, "forbody");
    llvm::BasicBlock* incBB = llvm::BasicBlock::Create(*globalCtx, "forinc");
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(*globalCtx, "forend");
    
    builder->CreateBr(condBB);
    
    // condition block
    builder->SetInsertPoint(condBB);
    llvm::Value* cond = this->compile(conditionNode);

    cond = this->to_bool(cond);
    if (!cond)
        return nullptr;
    
    builder->CreateCondBr(cond, bodyBB, endBB);
    
    // body block
    theFunction->insert(theFunction->end(), bodyBB);
    builder->SetInsertPoint(bodyBB);
    auto _ = this->compile(astNode->children[1]);
    
    if (!builder->GetInsertBlock()->getTerminator())
        builder->CreateBr(incBB);
    
    // increment block
    theFunction->insert(theFunction->end(), incBB);
    builder->SetInsertPoint(incBB);
    if (incrementNode->token.type != IGNORE)
        _ = this->compile(incrementNode);
    builder->CreateBr(condBB);
    
    // end block
    theFunction->insert(theFunction->end(), endBB);
    builder->SetInsertPoint(endBB);
    
    popScope(); // pop the loop variable scope
    
    return builder->getInt1(false);
}

} // lox
