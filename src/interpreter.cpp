#include "interpreter.h"

namespace lox {
underlying_t Interpreter::resolveToken(const Token& token) const {
    switch (token.type) {
    case STRING:
    case NUMBER:
        return token.get_literal();
    case TRUE:
        return true;
    case FALSE:
        return false;
    case NIL:
        return nullptr;
    case IDENTIFIER: // TODO search in the symbol table
    default:
        return std::monostate{};
    }
}

underlying_t Interpreter::resolveOrExecute(const std::unique_ptr<ASTNode>& node) const {
    auto resolved_value = resolveToken(node->token);

    return std::visit([this, &node, &resolved_value]<typename T>(T&& value) {
        if constexpr (std::is_same_v<std::decay_t<T>, std::monostate>) {
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

underlying_t Interpreter::compilePlus(const std::unique_ptr<ASTNode>& astNode) const {
    auto opType = astNode->must_be_op_type
                        .or_else([]() {return std::optional<TokenOpType>(UNARY);})
                        .value();

    // compile the unary token, compile the + (something), e.g., + + + 123.5
    if (opType == UNARY)
        return this->execute(astNode->children[0]);

    // compile the binary token, compile the a + b
    auto lhs = this->resolveOrExecute(astNode->children[0]);
    auto rhs = this->resolveOrExecute(astNode->children[1]);

    // by now, both lhs and rhs are (should be) either a string or a number
    return std::visit([](auto&& l, auto&& r) -> underlying_t {
        using l_type = std::decay_t<decltype(l)>;
        using r_type = std::decay_t<decltype(r)>;

        if constexpr ((std::is_same_v<l_type, RealNumber> && std::is_same_v<r_type, RealNumber>)
            || (std::is_same_v<l_type, std::string> && std::is_same_v<r_type, std::string>))
            return l + r;
        else if constexpr (std::is_same_v<l_type, std::string> && std::is_same_v<r_type, RealNumber>)
            return l + to_string(r);
        else if constexpr (std::is_same_v<l_type, RealNumber> && std::is_same_v<r_type, std::string>)
            return to_string(l) + r;
        
        throw std::runtime_error("Invalid operands for '+' operator.");
    }, lhs, rhs);
}

underlying_t Interpreter::compileMinus(const std::unique_ptr<ASTNode>& astNode) const {

}

underlying_t Interpreter::compileASTRoot(const std::unique_ptr<ASTNode>& astNode) const {

}

underlying_t Interpreter::execute(const std::unique_ptr<ASTNode>& root) const {
    switch (root->token.type) {
    case AST_ROOT: {
        //underlying_t last_value;
        for (auto&& ast_node : root->children) {
            std::visit([]<typename T>(T&& value) {
                if constexpr (std::is_same_v<std::decay_t<T>, std::string> || std::is_same_v<std::decay_t<T>, RealNumber>
                    || std::is_same_v<std::decay_t<T>, bool>) {
                    if constexpr (std::is_same_v<std::decay_t<T>, RealNumber>) {
                        // FIXME REMOVE THIS. THIS IS DUMB, THE EVALUATION SYSTEM EXPECTS THIS
                        std::cout << (static_cast<RealNumber>(value).to_double() == 0.0f ? "false" : "true") << std::endl;
                        return;
                    }
                    std::cout << value << std::endl;
                } else if constexpr (std::is_same_v<std::decay_t<T>, nullptr_t>) {
                    std::cout << "nil" << std::endl;
                }
            }, execute(ast_node));
        }
        return std::monostate();
    }
    case PLUS:
        return compilePlus(root);
    default:
        // likely a constant
        auto resolved = resolveToken(root->token);
        std::visit([]<typename T>(T&& value) {
            if constexpr (std::is_same_v<std::decay_t<T>, std::monostate>) {
                std::cerr << "Will come later..." << std::endl;
            }
        }, resolved);
        return resolved;
    }
}
}
