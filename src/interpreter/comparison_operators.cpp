#include "interpreter.h"

namespace lox {
underlying_t Interpreter::compileGreaterThan(const std::unique_ptr<ASTNode>& astNode) const {
    // compile a > b
    auto lhs = this->resolveOrExecute(astNode->children[0]);
    auto rhs = this->resolveOrExecute(astNode->children[1]);

    // by now, both lhs and rhs are (should be) either a string, a number, a boolean, a null
    return std::visit([]<typename l_type, typename r_type>(l_type&& l, r_type&& r) -> underlying_t {
        // the only valid case for the > operator
        if constexpr (std::is_same_v<std::decay_t<l_type>, RealNumber> && std::is_same_v<std::decay_t<r_type>, RealNumber>)
            return l > r;

        throw std::runtime_error("Invalid operands for '>' binary operator."); // FIXME improve this
    }, lhs, rhs);
}

underlying_t Interpreter::compileGreaterThanOrEqual(const std::unique_ptr<ASTNode>& astNode) const {
    // compile a >= b
    auto lhs = this->resolveOrExecute(astNode->children[0]);
    auto rhs = this->resolveOrExecute(astNode->children[1]);

    // by now, both lhs and rhs are (should be) either a string, a number, a boolean, a null
    return std::visit([]<typename l_type, typename r_type>(l_type&& l, r_type&& r) -> underlying_t {
        // the only valid case for the >= operator
        if constexpr (std::is_same_v<std::decay_t<l_type>, RealNumber> && std::is_same_v<std::decay_t<r_type>, RealNumber>)
            return l >= r;

        throw std::runtime_error("Invalid operands for '>=' binary operator."); // FIXME improve this
    }, lhs, rhs);
}

underlying_t Interpreter::compileLessThan(const std::unique_ptr<ASTNode>& astNode) const {
    // compile a < b
    auto lhs = this->resolveOrExecute(astNode->children[0]);
    auto rhs = this->resolveOrExecute(astNode->children[1]);

    // by now, both lhs and rhs are (should be) either a string, a number, a boolean, a null
    return std::visit([]<typename l_type, typename r_type>(l_type&& l, r_type&& r) -> underlying_t {
        // the only valid case for the < operator
        if constexpr (std::is_same_v<std::decay_t<l_type>, RealNumber> && std::is_same_v<std::decay_t<r_type>, RealNumber>)
            return l < r;

        throw std::runtime_error("Invalid operands for '<' binary operator."); // FIXME improve this
    }, lhs, rhs);
}

underlying_t Interpreter::compileLessThanOrEqual(const std::unique_ptr<ASTNode>& astNode) const {
    // compile a <= b
    auto lhs = this->resolveOrExecute(astNode->children[0]);
    auto rhs = this->resolveOrExecute(astNode->children[1]);

    // by now, both lhs and rhs are (should be) either a string, a number, a boolean, a null
    return std::visit([]<typename l_type, typename r_type>(l_type&& l, r_type&& r) -> underlying_t {
        // the only valid case for the <= operator
        if constexpr (std::is_same_v<std::decay_t<l_type>, RealNumber> && std::is_same_v<std::decay_t<r_type>, RealNumber>)
            return l <= r;

        throw std::runtime_error("Invalid operands for '<=' binary operator."); // FIXME improve this
    }, lhs, rhs);
}
};