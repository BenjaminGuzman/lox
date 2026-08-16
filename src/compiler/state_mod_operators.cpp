#include <ranges>

#include "compiler.h"

namespace lox {

void Compiler::pushScope() const {
    symbol_table.emplace_back();
}

void Compiler::popScope() const {
    if (symbol_table.empty())
        return;

    // TODO clear all the strings in the scope to be popped from the interned_strings map
    //  though, they not necessarily need to be cleaned as they may be reused...
    symbol_table.pop_back();
}

llvm::AllocaInst* Compiler::getVar(const std::string& name) const {
    for (auto & it : std::views::reverse(symbol_table))
        if (it.contains(name))
            return it.at(name);
    return nullptr;
}

llvm::Value* Compiler::compileVar(const std::unique_ptr<ASTNode>& astNode) const {
    if (astNode->children.empty())
        return nullptr;

    if (astNode->children[0]->token.type == IDENTIFIER) {
        std::cerr << "'var' declaration is not valid. Will introduce type-specific declaration later on" << std::endl;
        return nullptr;
    }

    if (astNode->children[0]->token.type == EQ) {
        // declaration and initialization: var a = 10;
        const auto& eqNode = astNode->children[0];
        std::string identifier = eqNode->children[0]->token.lexeme;
        
        llvm::Value* initVal = this->compile(eqNode->children[1]);
        if (!initVal)
            return nullptr;

        llvm::AllocaInst* alloca = builder->CreateAlloca(initVal->getType(), nullptr, identifier);
        symbol_table.back()[identifier] = alloca;
        
        builder->CreateStore(initVal, alloca);
        return initVal;
    }

    std::cerr << "Invalid var declaration." << std::endl;
    return nullptr;
}

llvm::Value* Compiler::compileEqual(const std::unique_ptr<ASTNode>& astNode) const {
    // assignment: a = 10;
    std::string identifier = astNode->children[0]->token.lexeme;
    
    llvm::AllocaInst* alloca = getVar(identifier);
    if (!alloca) {
        std::cerr << "Undefined variable '" << identifier << "'." << std::endl;
        return nullptr;
    }
    
    llvm::Value* val = this->compile(astNode->children[1]);
    if (!val)
        return nullptr;
    
    builder->CreateStore(val, alloca);
    return val;
}

llvm::Value* Compiler::compileIdentifier(const std::unique_ptr<ASTNode>& astNode) const {
    std::string identifier = astNode->token.lexeme;
    
    llvm::AllocaInst* alloca = getVar(identifier);
    if (!alloca) {
        std::cerr << "Undefined variable '" << identifier << "'." << std::endl;
        return nullptr;
    }
    
    return builder->CreateLoad(alloca->getAllocatedType(), alloca, identifier.c_str());
}

} // lox
