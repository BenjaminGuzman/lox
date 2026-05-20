#include "interpreter.h"

namespace lox {
underlying_t Interpreter::compileStar(const std::unique_ptr<ASTNode>& astNode) const {
    // compile the binary token, compile the a * b
    auto lhs = this->resolveOrExecute(astNode->children[0]);
    auto rhs = this->resolveOrExecute(astNode->children[1]);

    // by now, both lhs and rhs are (should be) either a string, a number, a boolean, a null
    return std::visit([]<typename l_type, typename r_type>(l_type&& l, r_type&& r) -> underlying_t {
        // the only case were the * operator doesn't produce a number
        if constexpr (std::is_same_v<std::decay_t<l_type>, RealNumber> && std::is_same_v<std::decay_t<r_type>, RealNumber>)
            return l * r;

        constexpr bool is_l_string = std::is_same_v<std::decay_t<l_type>, std::string> || std::is_same_v<std::decay_t<l_type>, std::string_view>;
        constexpr bool is_r_string = std::is_same_v<std::decay_t<r_type>, std::string> || std::is_same_v<std::decay_t<r_type>, std::string_view>;

        // string * string -> incorrect!
        if constexpr (is_l_string && is_r_string)
            throw std::runtime_error("Invalid operands for '*' binary operator."); // FIXME improve this

        // string * number or number * string
        constexpr bool should_repeat_l_string = is_l_string && std::is_same_v<std::decay_t<r_type>, RealNumber>;
        constexpr bool should_repeat_r_string = std::is_same_v<std::decay_t<l_type>, RealNumber> && is_r_string;
        if constexpr (should_repeat_l_string || should_repeat_r_string) {
            RealNumber multiplicator;
            std::string str;
            if constexpr (should_repeat_l_string) {
                multiplicator = static_cast<RealNumber>(r);
                str = l; // if string_view, this should automatically copy it to str
            } else {
                multiplicator = static_cast<RealNumber>(l);
                str = r;
            }

            if (multiplicator.is_negative || multiplicator.fractional != 0) {
                std::cerr << ("Invalid operands for '*' binary operator (string repeat).") << std::endl; // FIXME improve this
                return std::monostate();
            }

            std::string res;
            res.reserve(res.size() * multiplicator.integer);
            for (size_t i = 0 ; i < multiplicator.integer; ++i)
                res += str;

            return res;
        }

        throw std::runtime_error("Invalid operands for '*' binary operator."); // FIXME improve this
    }, lhs, rhs);
}

underlying_t Interpreter::compileSlash(const std::unique_ptr<ASTNode>& astNode) const {
    // compile the binary token, compile the a / b
    auto lhs = this->resolveOrExecute(astNode->children[0]);
    auto rhs = this->resolveOrExecute(astNode->children[1]);

    // by now, both lhs and rhs are (should be) either a string, a number, a boolean, a null
    return std::visit([]<typename l_type, typename r_type>(l_type&& l, r_type&& r) -> underlying_t {
        // the only valid case for the / operator
        if constexpr (std::is_same_v<std::decay_t<l_type>, RealNumber> && std::is_same_v<std::decay_t<r_type>, RealNumber>)
            return l / r;

        throw std::runtime_error("Invalid operands for '/' binary operator."); // FIXME improve this
    }, lhs, rhs);
}

underlying_t Interpreter::compilePlus(const std::unique_ptr<ASTNode>& astNode) const {
    auto opType = astNode->must_be_op_type
                        .or_else([]() {return std::optional<TokenOpType>(UNARY);})
                        .value();

    // compile the unary token, compile the + (something), e.g., + + + 123.5
    if (opType == UNARY)
        return this->resolveOrExecute(astNode->children[0]);

    // compile the binary token, compile the a + b
    auto lhs = this->resolveOrExecute(astNode->children[0]);
    auto rhs = this->resolveOrExecute(astNode->children[1]);

    // by now, both lhs and rhs are (should be) either a string, a number, a boolean, a null
    return std::visit([]<typename l_type, typename r_type>(l_type&& l, r_type&& r) -> underlying_t {
        // the only cases were the + operator doesn't produce a string
        if constexpr (std::is_same_v<std::decay_t<l_type>, RealNumber> && std::is_same_v<std::decay_t<r_type>, RealNumber>)
            // both are numbers, number + number -> number
            return l + r;
        if constexpr (std::is_same_v<std::decay_t<l_type>, bool> && std::is_same_v<std::decay_t<r_type>, bool>)
            // both are bool, bool + bool = int(bool) + int(bool)
            return RealNumber{static_cast<u_long>(l) + static_cast<u_long>(r), 0, 0};
        if constexpr (std::is_same_v<std::decay_t<l_type>, RealNumber> && std::is_same_v<std::decay_t<r_type>, bool>)
            // number + bool = number + int(bool)
            return l + RealNumber{static_cast<u_long>(r), 0, 0};
        if constexpr (std::is_same_v<std::decay_t<l_type>, bool> && std::is_same_v<std::decay_t<r_type>, RealNumber>)
            // bool + number = int(bool) + number
            return RealNumber{static_cast<u_long>(l), 0, 0} + r;

        constexpr bool is_l_string = std::is_same_v<std::decay_t<l_type>, std::string> || std::is_same_v<std::decay_t<l_type>, std::string_view>;
        constexpr bool is_r_string = std::is_same_v<std::decay_t<r_type>, std::string> || std::is_same_v<std::decay_t<r_type>, std::string_view>;
        std::string l_str;
        std::string r_str;

        if constexpr (std::is_same_v<std::decay_t<l_type>, std::string_view>) {
            l_str = std::string(l);
        } else if constexpr (std::is_same_v<std::decay_t<l_type>, std::string>) {
            l_str = l;
        }

        if constexpr (std::is_same_v<std::decay_t<r_type>, std::string_view>) {
            r_str = std::string(r);
        } else if constexpr (std::is_same_v<std::decay_t<r_type>, std::string>) {
            r_str = r;
        }

        // string + string, or string + number
        if constexpr (is_l_string && is_r_string)
            return l_str + r_str;
        if constexpr (is_l_string && std::is_same_v<std::decay_t<r_type>, RealNumber>)
            return l_str + r;
        if constexpr (std::is_same_v<std::decay_t<l_type>, RealNumber> && is_r_string)
            return l + r_str;

        // string + boolean
        if constexpr (is_l_string && std::is_same_v<std::decay_t<r_type>, bool>)
            return l_str + to_string(r);
        if constexpr (std::is_same_v<std::decay_t<l_type>, bool> && is_r_string)
            return to_string(l) + r_str;

        // string + nullptr
        if constexpr (is_l_string && std::is_same_v<std::decay_t<r_type>, nullptr_t>)
            return l_str + "nil";
        if constexpr (std::is_same_v<std::decay_t<l_type>, std::nullptr_t> && is_r_string)
            return "nil" + r_str;

        throw std::runtime_error("Invalid operands for '+' binary operator."); // FIXME improve this
    }, lhs, rhs);
}

underlying_t Interpreter::compileMinus(const std::unique_ptr<ASTNode>& astNode) const {
    auto opType = astNode->must_be_op_type
                        .or_else([]() {return std::optional<TokenOpType>(UNARY);})
                        .value();

    // compile the unary token, compile the - (something), e.g., - - - 123.5
    if (opType == UNARY) {
        auto res = this->resolveOrExecute(astNode->children[0]);
        return std::visit([]<typename T>(T&& value) -> underlying_t {
            if constexpr (std::is_same_v<std::decay_t<T>, RealNumber>) { // negate a number
                const auto num = static_cast<RealNumber>(value);
                return RealNumber{num.integer, num.fractional, num.n_fractional_digits, !num.is_negative};
            }

            throw std::runtime_error("Invalid operands for '-' unary operator."); // FIXME improve this
        }, res);
    }

    // compile the binary token, compile the a - b
    auto lhs = this->resolveOrExecute(astNode->children[0]);
    auto rhs = this->resolveOrExecute(astNode->children[1]);

    // by now, both lhs and rhs are (should be) either a string, a number, a boolean, a null
    return std::visit([]<typename l_type, typename r_type>(l_type&& l, r_type&& r) -> underlying_t {
        if constexpr (std::is_same_v<std::decay_t<l_type>, RealNumber> && std::is_same_v<std::decay_t<r_type>, RealNumber>)
            // both are numbers, number - number -> number
            return l - r;
        if constexpr (std::is_same_v<std::decay_t<l_type>, bool> && std::is_same_v<std::decay_t<r_type>, bool>)
            // both are bool, bool - bool = int(bool) - int(bool)
            return RealNumber{static_cast<u_long>(l) - static_cast<u_long>(r), 0, 0};
        if constexpr (std::is_same_v<std::decay_t<l_type>, RealNumber> && std::is_same_v<std::decay_t<r_type>, bool>)
            // number - bool = number - int(bool)
            return l - RealNumber{static_cast<u_long>(r), 0, 0};
        if constexpr (std::is_same_v<std::decay_t<l_type>, bool> && std::is_same_v<std::decay_t<r_type>, RealNumber>)
            // bool - number = int(bool) - number
            return RealNumber{static_cast<u_long>(l), 0, 0} - r;

        throw std::runtime_error("Invalid operands for '-' binary operator."); // FIXME improve this
    }, lhs, rhs);
}
};