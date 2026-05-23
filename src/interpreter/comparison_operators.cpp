#include "interpreter.h"

namespace lox {
underlying_t Interpreter::compileGreaterThan(const std::unique_ptr<ASTNode>& astNode) const {
    // compile a > b
    auto lhs = this->resolveOrExecute(astNode->children[0]);
    auto rhs = this->resolveOrExecute(astNode->children[1]);

    // by now, both lhs and rhs are (should be) either a string, a number, a boolean, a null
    return std::visit([this, &astNode]<typename l_type, typename r_type>(l_type&& l, r_type&& r) -> underlying_t {
        // the only valid case for the > operator
        if constexpr (std::is_same_v<std::decay_t<l_type>, RealNumber> && std::is_same_v<std::decay_t<r_type>, RealNumber>)
            return l > r;

        this->registerError("Operands must be numbers.", astNode->token);
        return std::monostate();
    }, lhs, rhs);
}

underlying_t Interpreter::compileGreaterThanOrEqual(const std::unique_ptr<ASTNode>& astNode) const {
    // compile a >= b
    auto lhs = this->resolveOrExecute(astNode->children[0]);
    auto rhs = this->resolveOrExecute(astNode->children[1]);

    // by now, both lhs and rhs are (should be) either a string, a number, a boolean, a null
    return std::visit([this, &astNode]<typename l_type, typename r_type>(l_type&& l, r_type&& r) -> underlying_t {
        // the only valid case for the >= operator
        if constexpr (std::is_same_v<std::decay_t<l_type>, RealNumber> && std::is_same_v<std::decay_t<r_type>, RealNumber>)
            return l >= r;

        this->registerError("Operands must be numbers.", astNode->token);
        return std::monostate();
    }, lhs, rhs);
}

underlying_t Interpreter::compileLessThan(const std::unique_ptr<ASTNode>& astNode) const {
    // compile a < b
    auto lhs = this->resolveOrExecute(astNode->children[0]);
    auto rhs = this->resolveOrExecute(astNode->children[1]);

    // by now, both lhs and rhs are (should be) either a string, a number, a boolean, a null
    return std::visit([this, &astNode]<typename l_type, typename r_type>(l_type&& l, r_type&& r) -> underlying_t {
        // the only valid case for the < operator
        if constexpr (std::is_same_v<std::decay_t<l_type>, RealNumber> && std::is_same_v<std::decay_t<r_type>, RealNumber>)
            return l < r;

        this->registerError("Operands must be numbers.", astNode->token);
        return std::monostate();
    }, lhs, rhs);
}

underlying_t Interpreter::compileLessThanOrEqual(const std::unique_ptr<ASTNode>& astNode) const {
    // compile a <= b
    auto lhs = this->resolveOrExecute(astNode->children[0]);
    auto rhs = this->resolveOrExecute(astNode->children[1]);

    // by now, both lhs and rhs are (should be) either a string, a number, a boolean, a null
    return std::visit([this, &astNode]<typename l_type, typename r_type>(l_type&& l, r_type&& r) -> underlying_t {
        // the only valid case for the <= operator
        if constexpr (std::is_same_v<std::decay_t<l_type>, RealNumber> && std::is_same_v<std::decay_t<r_type>, RealNumber>)
            return l <= r;

        this->registerError("Operands must be numbers.", astNode->token);
        return std::monostate();
    }, lhs, rhs);
}

underlying_t Interpreter::compileEqualEqual(const std::unique_ptr<ASTNode>& astNode) const {
    // compile a == b
    auto lhs = this->resolveOrExecute(astNode->children[0]);
    auto rhs = this->resolveOrExecute(astNode->children[1]);

    // by now, both lhs and rhs are (should be) either a string, a number, a boolean, a null
    return std::visit([]<typename l_type, typename r_type>(l_type&& l, r_type&& r) -> underlying_t {
        constexpr bool is_lhs_string = std::is_same_v<std::decay_t<l_type>, std::string_view>
                                        || std::is_same_v<std::decay_t<l_type>, std::string>;
        constexpr bool is_rhs_string = std::is_same_v<std::decay_t<r_type>, std::string_view>
                                        || std::is_same_v<std::decay_t<r_type>, std::string>;

        // == operator is valid for comparing strings, numbers, booleans, nulls, as long as they are the same type
        if constexpr ((std::is_same_v<std::decay_t<l_type>, RealNumber> && std::is_same_v<std::decay_t<r_type>, RealNumber>)
            || (is_lhs_string && is_rhs_string) // comparing string_view and string is indeed defined and valid
            || (std::is_same_v<std::decay_t<l_type>, bool> && std::is_same_v<std::decay_t<r_type>, bool>)
            || (std::is_same_v<std::decay_t<l_type>, nullptr_t> && std::is_same_v<std::decay_t<r_type>, nullptr_t>))
            return l == r;

        // if operands differ in type, they are inherently different
        return false;
    }, lhs, rhs);
}

underlying_t Interpreter::compileNotEqual(const std::unique_ptr<ASTNode>& astNode) const {
    // compile a != b
    return !std::get<bool>(compileEqualEqual(astNode)); // this get is safe, compileEqual will always return a bool
}
};