#include "interpreter.h"

#include <ranges>
#include <algorithm>

#include "interpreter/user_function.h"

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

    // get the scope where the variable was declared
    for (auto& frame : std::views::reverse(stack))
        if (frame->symbols.contains(identifier)) {
            frame->symbols[identifier] = this->resolveOrExecute(astNode->children[1]);
            return frame->symbols[identifier];
        }

    registerError("Undeclared variable '" + identifier + "'", astNode->children[0]->token);
    return std::monostate();
}

underlying_t Interpreter::compileVar(const std::unique_ptr<ASTNode>& astNode) {
    // declaration, i.e., var a;
    if (astNode->children[0]->token.type == IDENTIFIER) {
        // this->registerError("Apparently in lox is illegal to declare a variable", astNode->token); // FIXME remove me
        // return std::monostate();
        const std::string& identifier = astNode->children[0]->token.lexeme;
        if (stackTop().symbols.contains(identifier)) {
            this->registerError("Previously declared variable '" + identifier + "' cannot be redeclared", astNode->children[0]->token);
            return std::monostate();
        }

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

underlying_t Interpreter::compileLeftBrace(const std::unique_ptr<ASTNode>& astNode) {
    // create a new stack frame, execute, and drop the stack frame
    stack.emplace_back(std::make_shared<Stackframe>());
    underlying_t ret = nullptr;
    for (const auto& child: astNode->children) {
        if (shouldUnwindStack)
            break;
        ret = this->execute(child);
    }
    stack.pop_back();
    return ret;
}

std::vector<underlying_t> Interpreter::compileFuncCall(const std::unique_ptr<ASTNode> &astNode) {
    std::string funcName = astNode->token.lexeme;
    size_t n_expected_args = astNode->children.size();
    std::string funcSignature = funcName + "_" + std::to_string(n_expected_args);

    underlying_t func = this->getVar(funcSignature); // search for the function using the signature
    return std::visit([this, &funcName, &astNode]<typename E>(E&& v) -> std::vector<underlying_t> {
        if constexpr (std::is_same_v<std::decay_t<E>, std::shared_ptr<UserFunction>>)
            return v->execute(*this, astNode->children);

        // exact match with signature was not possible, maybe it's a vararg func, use the name to search for it
        underlying_t func = this->getVar(funcName);
        return std::visit([this, &astNode]<typename EE>(EE&& vv) -> std::vector<underlying_t> {
            if constexpr (std::is_same_v<std::decay_t<EE>, std::shared_ptr<UserFunction>>)
                return vv->execute(*this, astNode->children);

            this->registerError("Variable '" + astNode->token.lexeme + "' is not a function, "
                                                                   "but it was invoked as one", astNode->token);
            return {std::monostate()};
        }, func);
    }, func);
}

underlying_t Interpreter::compileFunc(const std::unique_ptr<ASTNode> &astNode) {
    if (astNode->children.size() != 3) {
        this->registerError("Function declaration is invalid", astNode->token);
        return std::monostate();
    }

    std::string funcName = astNode->children[0]->token.lexeme;
    const auto& funcParams = astNode->children[1];
    const auto& funcBody = astNode->children[2];

    std::vector<std::string> paramNames(funcParams->children.size());
    std::ranges::transform(funcParams->children, paramNames.begin(), [](const std::unique_ptr<ASTNode>& child) {
        return child->token.lexeme;
    });

    // add the function to the symbol table
    std::string funcSig = funcName + "_" + std::to_string(funcParams->children.size());
    stackTop().symbols[funcSig] = std::make_shared<UserFunction>(UserFunction{
        .name = astNode->children[0]->token.lexeme,
        .paramNames = paramNames,
        .funcBody = funcBody,
        .nativeFunc = nullptr,
        .isNative = false,
        .closure = this->stack
    });
    stackTop().symbols[funcName] = stackTop().symbols[funcSig]; // FIXME this is dumb but the evaluation system requires function signatures that only include the name

    return funcSig;
}

underlying_t Interpreter::compileReturn(const std::unique_ptr<ASTNode> &astNode) {
    std::vector<underlying_t> returnValues = astNode->children
       | std::views::transform([this](const std::unique_ptr<ASTNode>& node) {
           return this->resolveOrExecute(node);
       })
       | std::ranges::to<std::vector<underlying_t>>();
    this->returnFunctions.back()(returnValues);
    return std::monostate();
}
}
