//
// Created by Benjamín Guzmán on 25/03/26.
//

#ifndef CODECRAFTERS_INTERPRETER_COMPILER_H
#define CODECRAFTERS_INTERPRETER_COMPILER_H
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Support/TargetSelect.h>

#include "parser.h"


namespace lox {

class Compiler {
private:
    std::unique_ptr<llvm::LLVMContext> globalCtx;
    std::unique_ptr<llvm::Module> globalModule;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    
    // Stack of scopes mapping variable names to their LLVM alloca instructions
    mutable std::vector<std::unordered_map<std::string, llvm::AllocaInst*>> symbol_table;


    [[nodiscard]] llvm::Value* compileNumber(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] llvm::Value* compileString(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] llvm::Value* compilePlus(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] llvm::Value* compileMinus(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] llvm::Value* compileStar(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] llvm::Value* compileSlash(const std::unique_ptr<ASTNode>& astNode) const;
    
    [[nodiscard]] llvm::Value* compileTrue(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] llvm::Value* compileFalse(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] llvm::Value* compileNot(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] llvm::Value* compileComparison(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] llvm::Value* compileEquality(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] llvm::Value* compileGroup(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] llvm::Value* compileVar(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] llvm::Value* compileEqual(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] llvm::Value* compileIdentifier(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] llvm::Value* compileASTRoot(const std::unique_ptr<ASTNode>& astNode) const;
    
    llvm::AllocaInst* getVar(const std::string& name) const;

    /**
     * Stores the token types to the function that should be responsible for compiling the token.
     */
    const std::unordered_map<TokenType, std::function<llvm::Value*(const std::unique_ptr<ASTNode>& astNode)>> TOKEN_COMPILERS = {
        {NUMBER,    [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileNumber(astNode);}},
        {AST_ROOT,  [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileASTRoot(astNode);}},
        {PLUS,      [this](const std::unique_ptr<ASTNode>& astNode) {return this->compilePlus(astNode);}},
        {MINUS,     [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileMinus(astNode);}},
        {STAR,      [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileStar(astNode);}},
        {SLASH,     [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileSlash(astNode);}},
        {LEFT_PAREN,[this](const std::unique_ptr<ASTNode>& astNode) {return this->compileGroup(astNode);}},
        
        {TRUE,      [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileTrue(astNode);}},
        {FALSE,     [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileFalse(astNode);}},
        {NOT,       [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileNot(astNode);}},
        
        {LT,        [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileComparison(astNode);}},
        {LTE,       [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileComparison(astNode);}},
        {GT,        [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileComparison(astNode);}},
        {GTE,       [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileComparison(astNode);}},
        
        {EQEQ,      [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileEquality(astNode);}},
        {NEQ,       [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileEquality(astNode);}},
        
        {VAR,       [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileVar(astNode);}},
        {EQ,        [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileEqual(astNode);}},
        {IDENTIFIER,[this](const std::unique_ptr<ASTNode>& astNode) {return this->compileIdentifier(astNode);}},
    };

public:
    Compiler();
    
    void pushScope();
    void popScope();
    //llvm::Value* compile(const std::unique_ptr<ASTNode> root);
    [[nodiscard]] llvm::Value* compile(const std::unique_ptr<ASTNode>& root) const;
    
    llvm::GenericValue runJIT();
};
} // lox

#endif //CODECRAFTERS_INTERPRETER_COMPILER_H