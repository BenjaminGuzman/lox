#include "../include/compiler.h"

namespace lox {

void Compiler::pushScope() {
    symbol_table.emplace_back();
}

void Compiler::popScope() {
    if (!symbol_table.empty()) {
        symbol_table.pop_back();
    }
}

llvm::AllocaInst* Compiler::getVar(const std::string& name) const {
    for (auto it = symbol_table.rbegin(); it != symbol_table.rend(); ++it)
        if (it->contains(name))
            return it->at(name);
    return nullptr;
}

llvm::Value* Compiler::compileVar(const std::unique_ptr<ASTNode>& astNode) const {
    if (astNode->children.empty())
        return nullptr;

    if (astNode->children[0]->token.type == IDENTIFIER) {
        // declaration: var a;
        std::string identifier = astNode->children[0]->token.lexeme;
        
        // allocate space for a double by default (nil/uninitialized Lox vars)
        llvm::AllocaInst* alloca = builder->CreateAlloca(builder->getDoubleTy(), nullptr, identifier);
        symbol_table.back()[identifier] = alloca;
        
        // Initialize to 0.0
        builder->CreateStore(llvm::ConstantFP::get(*globalCtx, llvm::APFloat(0.0)), alloca);
        return alloca;
    }

    if (astNode->children[0]->token.type == EQ) {
        // Declaration and Initialization: var a = 10;
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
    // Assignment: a = 10;
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
