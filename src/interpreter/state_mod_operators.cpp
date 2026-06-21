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
        if (frame.symbols.contains(identifier)) {
            frame.symbols[identifier] = this->resolveOrExecute(astNode->children[1]);
            return frame.symbols[identifier];
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
    stack.emplace_back();
    underlying_t ret = nullptr;
    for (const auto& child: astNode->children)
        ret = this->execute(child);
    stack.pop_back();
    return ret;
}

std::vector<underlying_t> Interpreter::compileFuncCall(const std::unique_ptr<ASTNode> &astNode) {
    std::string funcName = astNode->token.lexeme;
    size_t n_expected_args = astNode->children.size();
    std::string funcSignature = funcName + "_" + std::to_string(n_expected_args);

    if (const auto it = NATIVE_FUNCTIONS.find(funcSignature); it != NATIVE_FUNCTIONS.end())
        // try matching by the exact signature (name + args)
        return {it->second(astNode.get(), *this)};
    if (const auto it = NATIVE_FUNCTIONS.find(funcName); it != NATIVE_FUNCTIONS.end())
        // try matching by func name (meaning it's a vararg func)
        return {it->second(astNode.get(), *this)};
    if (const auto it = USER_FUNCTIONS.find(funcSignature); it != USER_FUNCTIONS.end())
        // if not a native function, try a user-defined function
        return it->second(*this, astNode->children);

    this->registerError("Function '" + funcName + "' does not exists", astNode->token);
    return {std::monostate()};
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
    USER_FUNCTIONS.emplace(funcSig, UserFunction{
        .name = astNode->children[0]->token.lexeme,
        .paramNames = paramNames,
        .funcBody = funcBody
    });
    USER_FUNCTIONS.emplace(funcName, USER_FUNCTIONS.at(funcSig)); // FIXME this is dumb but the evaluation system requires function signatures that only include the name

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
