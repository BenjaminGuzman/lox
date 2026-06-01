#include "interpreter.h"
#include "internal/utils.h"

namespace lox {
underlying_t Interpreter::compileOr(const std::unique_ptr<ASTNode> &astNode) const {
    if (astNode->children.size() != 2) {
        this->registerError("Invalid use of 'or' operator", astNode->token);
        return nullptr;
    }

    auto lhs = this->resolveOrExecute(astNode->children[0]);
    if (is_truthy(lhs)) // use short-circuit operation, if lhs is true, ignore rhs, true or whatever = true
        return true;

    auto rhs = this->resolveOrExecute(astNode->children[1]);
    return is_truthy(rhs);
}

underlying_t Interpreter::compileAnd(const std::unique_ptr<ASTNode> &astNode) const {
    if (astNode->children.size() != 2) {
        this->registerError("Invalid use of 'and' operator", astNode->token);
        return nullptr;
    }

    auto lhs = this->resolveOrExecute(astNode->children[0]);
    if (is_falsy(lhs)) // use short-circuit operation, if lhs is false, ignore rhs, false and whatever = false
        return false;

    auto rhs = this->resolveOrExecute(astNode->children[1]);
    return is_truthy(rhs);
}
};