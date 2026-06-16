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

underlying_t Interpreter::compileFor(const std::unique_ptr<ASTNode>& astNode) {
    if (astNode->children.size() < 2) {
        this->registerError("Invalid for statement", astNode->token);
        return nullptr;
    }

    const auto& forGroup = astNode->children[0];
    if (forGroup->children.size() != 3) {
        this->registerError("Invalid for statement. Expected 3 things: init, condition, increment. "
                            "This may be an issue with lox parser", astNode->token);
        return nullptr;
    }
    const auto& forExec = astNode->children[1];

    stack.emplace_back(); // open a new stackframe for the for loop
    auto _ = this->resolveOrExecute(forGroup->children[0]); // execute initialization
    const auto& condition = forGroup->children[1];
    while (is_truthy(this->resolveOrExecute(condition))) {
        _ = this->resolveOrExecute(forExec); // execute execute statement
        auto _discarded = this->resolveOrExecute(forGroup->children[2]); // execute increment
    }
    stack.pop_back();

    return _;
}
}
