#include "interpreter.h"

#include <ranges>

namespace lox {
underlying_t Interpreter::resolveToken(const Token& token) const {
    switch (token.type) {
    case STRING:
        return StringTokenView(token).literal();
    case NUMBER:
        return token.get_literal();
    case TRUE:
        return true;
    case FALSE:
        return false;
    case NIL:
        return nullptr;
    case IDENTIFIER:
        // search from the innermost scope (top of stack) to the global scope (bottom)
        for (const auto& it : std::views::reverse(stack))
            if (it.symbols.contains(token.lexeme))
                return it.symbols.at(token.lexeme);
        this->registerError("Undefined variable '" + token.lexeme + "'.", token);
        return std::monostate();
    default:
        return std::monostate{};
    }
}

underlying_t Interpreter::resolveOrExecute(const std::unique_ptr<ASTNode>& node) const {
    auto resolved_value = resolveToken(node->token);

    return std::visit([this, &node, &resolved_value]<typename T>(T&& value) {
        if constexpr (std::is_same_v<std::decay_t<T>, std::monostate>) {
            // monostate indicates the value is likely an expression as it may come from an =, *, -, etc...
            return this->execute(node);
        }

        return resolved_value;
    }, resolved_value);
}

underlying_t Interpreter::compileNumber(const std::unique_ptr<ASTNode>& astNode) const {
    return astNode->token.get_literal();
}

underlying_t Interpreter::compileString(const std::unique_ptr<ASTNode>& astNode) const {
    return astNode->token.get_literal();
}

underlying_t Interpreter::compilePrint(const std::unique_ptr<ASTNode>& astNode) const {
    for (const auto & child: astNode->children) {
        std::visit([]<typename T>(T&& value) {
            if constexpr (std::is_same_v<std::decay_t<T>, std::monostate>) {
                std::cout << "";
            } else if constexpr (std::is_same_v<std::decay_t<T>, std::nullptr_t>) {
                std::cout << "nil";
            } else if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
                std::cout << to_string(value);
            } else {
                std::cout << value;
            }
        }, resolveOrExecute(child));
    }
    std::cout << std::endl;

    return nullptr;
}

underlying_t Interpreter::compileNot(const std::unique_ptr<ASTNode>& astNode) const {
    return std::visit([]<typename T>(T&& value) -> underlying_t {
        // according to evaluation system:
        // For truthyness and falsyness, we will follow the convention introduced in the book,
        // where false and nil are falsy, and everything else is truthy.
        // which is kinda weird, what about !0?
        bool value_as_bool = true;
        if constexpr (std::is_same_v<std::decay_t<T>, bool>)
            value_as_bool = value;
        else if constexpr (std::is_same_v<std::decay_t<T>, nullptr_t>)
            value_as_bool = false;

        return !value_as_bool;
    }, this->resolveOrExecute(astNode->children[0]));
}

underlying_t Interpreter::compileASTRoot(const std::unique_ptr<ASTNode>& astNode) const {
    underlying_t res = std::monostate();
    for (auto&& node : astNode->children) {
        res = execute(node);
        if (!this->strict_output_mode) {
            std::visit([]<typename T>(T&& value) {
                if constexpr (std::is_same_v<std::decay_t<T>, std::string>
                    || std::is_same_v<std::decay_t<T>, std::string_view>
                    || std::is_same_v<std::decay_t<T>, RealNumber>) {
                    std::cout << value << std::endl;
                } else if constexpr (std::is_same_v<std::decay_t<T>, nullptr_t>) {
                    std::cout << "nil" << std::endl;
                } else if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
                    std::cout << to_string(value) << std::endl;
                }
            }, res);
        }
    }

    return std::visit([]<typename T>(T&& value) -> underlying_t {
        // returning a string view as end result is dangerous as it may get deleted after the interpreter is done
        if constexpr (std::is_same_v<std::decay_t<T>, std::string_view>)
            return std::string(value);
        return value;
    }, res);
}

underlying_t Interpreter::compileParenthesis(const std::unique_ptr<ASTNode>& astNode) const {
    if (astNode->children.empty())
        return std::monostate();
    return execute(astNode->children[0]); // for until right parent is reached?
}

underlying_t Interpreter::execute(const std::unique_ptr<ASTNode>& root) const {
    // TODO write an interpreter (and a compiler) that works bottom-up, no stack-overflow
    if (TOKEN_COMPILERS.contains(root->token.type))
        return TOKEN_COMPILERS.at(root->token.type)(root);

    // no handler was defined for this token, let's try to resolve it, maybe it's a symbol?
    //  or... it simply doesn't need to resolve to nothing, e.g., ';' resolves to monostate
    return resolveToken(root->token);
}

[[nodiscard]] std::string to_string(const bool b) {
    return b ? "true" : "false";
}
}
