#include "interpreter.h"

namespace lox {
underlying_t Interpreter::compileEqual(const std::unique_ptr<ASTNode>& astNode) {
    const std::string& identifier = astNode->children[0]->token.lexeme;
    if (astNode->children[0]->token.type != IDENTIFIER) {
        this->registerError("Invalid identifier '" + identifier + "' in equal assignment", astNode->token);
        return std::monostate();
    }
    if (astNode->children.size() != 2) {
        this->registerError("Invalid use of = operator", astNode->token);
        return std::monostate();
    }

    stackTop().symbols[identifier] = this->resolveOrExecute(astNode->children[1]);
    return stackTop().symbols[identifier];
}

underlying_t Interpreter::compileVar(const std::unique_ptr<ASTNode>& astNode) {
    // declaration, i.e., var a;
    if (astNode->children[0]->token.type == IDENTIFIER) {
        const std::string& identifier = astNode->children[0]->token.lexeme;
        // if (stackTop().symbols.contains(identifier)) {
            // this->registerError("Previously declared variable '" + identifier + "' cannot be redeclared", astNode->children[0]->token);
            // return std::monostate();
        // }

        stackTop().symbols[identifier] = nullptr;
        return nullptr;
    }

    // declaration and initialization, i.e., var a = 10;
    if (astNode->children[0]->token.type == EQ) {
        // tree is var -> {= -> {identifier, value}}
        const auto& eqNode = astNode->children[0];
        const std::string& identifier = eqNode->children[0]->token.lexeme;
        if (eqNode->children[0]->token.type != IDENTIFIER) {
            this->registerError("Invalid identifier '" + identifier + "' in var declaration", eqNode->token);
            return std::monostate();
        }
        if (eqNode->children.size() != 2) {
            this->registerError("Invalid use of = operator in var declaration", eqNode->token);
            return std::monostate();
        }

        stackTop().symbols[identifier] = this->resolveOrExecute(eqNode->children[1]);
        return stackTop().symbols[identifier];
    }

    this->registerError("Invalid use of 'var' keyword", astNode->token);
    return std::monostate();
}
}