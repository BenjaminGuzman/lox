#include "compiler.h"

namespace lox {

llvm::Value* Compiler::compileTrue(const std::unique_ptr<ASTNode>& astNode) const {
    return builder->getInt1(true);
}

llvm::Value* Compiler::compileFalse(const std::unique_ptr<ASTNode>& astNode) const {
    return builder->getInt1(false);
}

llvm::Value* Compiler::compileNot(const std::unique_ptr<ASTNode>& astNode) const {
    llvm::Value* operand = this->compile(astNode->children[0]);
    operand = this->to_bool(operand);
    if (!operand)
        return nullptr;

    return builder->CreateNot(operand, "nottmp");
}

/**
 * will create the following IR (for AND operator):
 * lhs:
 *   br lhs_bool, rhs, mergeBB ; short-circuit, if lhs is truthy check rhs, if not, simply return lhs
 *
 * rhs:
 *   br mergeBB:
 *
 * merge:
 *  ret phi <T> [rhs_bool, %rhs], [lhs_bool, %lhs] ; if came from rhs evaluation, return rhs, if came from lhs evaluation, return lhs
 *
 * will create the following IR (for OR operator):
 * lhs:
 *   br lhs_bool, mergeBB, rhs ; short-circuit, if lhs is truthy return lhs, if not, check rhs
 *
 * rhs:
 *   br mergeBB:
 *
 * merge:
 *  ret phi <T> [rhs_bool, %rhs], [lhs_bool, %lhs] ; if came from rhs evaluation, return rhs, if came from lhs evaluation, return lhs
 *
 *
 * Note that these operators will return true/false instead of the actual values like in the interpreter
 * This is due to LLVM's strict typing, a phi node should always return same type regardless of the branch it came from
 */
static llvm::Value* compileLogicalHelper(
    const Compiler* compiler, llvm::IRBuilder<>& builder, llvm::LLVMContext& ctx,
    const std::unique_ptr<ASTNode>& astNode, const TokenType& opType
) {
    llvm::Function* theFunction = builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* lhsBB = llvm::BasicBlock::Create(ctx, "lhs", theFunction);
    llvm::BasicBlock* rhsBB = llvm::BasicBlock::Create(ctx, "rhs");
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(ctx, "merge");

    builder.CreateBr(lhsBB);
    builder.SetInsertPoint(lhsBB);

    llvm::Value* lhs = compiler->compile(astNode->children[0]);
    llvm::Value* lhs_bool = compiler->to_bool(lhs);
    lhsBB = builder.GetInsertBlock(); // in case compile changed the block

    switch (opType) {
    case AND: // AND: if lhs is falsy, skip rhs
        builder.CreateCondBr(lhs_bool, rhsBB, mergeBB);
        break;
    case OR: // OR: if lhs is truthy, skip rhs (go to merge)
        builder.CreateCondBr(lhs_bool, mergeBB, rhsBB);
        break;
    default:
        std::cerr << "Unsupported logical operator\n";
        return nullptr;
    }

    // create rhs branch that simply goes to merge
    theFunction->insert(theFunction->end(), rhsBB);
    builder.SetInsertPoint(rhsBB);
    llvm::Value* rhs = compiler->compile(astNode->children[1]);
    llvm::Value* rhs_bool = compiler->to_bool(rhs);
    rhsBB = builder.GetInsertBlock(); // in case compile changed the block
    builder.CreateBr(mergeBB);

    // merge block
    theFunction->insert(theFunction->end(), mergeBB);
    builder.SetInsertPoint(mergeBB);
    llvm::PHINode* phi = builder.CreatePHI(builder.getInt1Ty(), 2, "logicres");
    phi->addIncoming(lhs_bool, lhsBB);
    phi->addIncoming(rhs_bool, rhsBB);

    return phi;
}

llvm::Value* Compiler::compileOr(const std::unique_ptr<ASTNode>& astNode) const {
    return compileLogicalHelper(this, *builder, *globalCtx, astNode, OR);
}

llvm::Value* Compiler::compileAnd(const std::unique_ptr<ASTNode>& astNode) const {
    return compileLogicalHelper(this, *builder, *globalCtx, astNode, AND);
}

} // lox
