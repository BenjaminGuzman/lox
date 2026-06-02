#include "interpreter.h"
#include "internal/utils.h"

namespace lox {
underlying_t Interpreter::compileIf(const std::unique_ptr<ASTNode> &astNode) const {
    if (astNode->children.size() < 2) {
        this->registerError("Invalid if statement", astNode->token);
        return nullptr;
    }

    underlying_t condition = this->resolveOrExecute(astNode->children[0]);
    if (is_truthy(condition))
        return this->resolveOrExecute(astNode->children[1]);

    if (astNode->children.size() == 3) // execute else (iff exists)
        return this->resolveOrExecute(astNode->children[2]);

    return nullptr;
}

underlying_t Interpreter::compileWhile(const std::unique_ptr<ASTNode>& astNode) const {
    if (astNode->children.size() < 2) {
        this->registerError("Invalid while statement", astNode->token);
        return nullptr;
    }

    underlying_t ret;
    underlying_t condition = this->resolveOrExecute(astNode->children[0]);
    while (is_truthy(condition)) {
        ret = this->resolveOrExecute(astNode->children[1]);
        condition = this->resolveOrExecute(astNode->children[0]);
    }

    return ret;
}
}
