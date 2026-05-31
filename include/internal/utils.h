#ifndef CODECRAFTERS_INTERPRETER_INTERNAL_UTILS_H
#define CODECRAFTERS_INTERPRETER_INTERNAL_UTILS_H

#include <string>

namespace lox {
/**
 * Constructs a string composed of "[<filepath>:<line>:<col>]: "
 * @param filepath the filepath
 * @param line the line
 * @param col the col
 * @return the string.
 */
constexpr std::string error_in_file_prefix(const std::string& filepath, size_t line, size_t col) {
    return "[" + filepath + ":" + std::to_string(line) + ":" + std::to_string(col) + "]: ";
}

/**
 * Evaluates if a value is truthy in lox
 * @param value the value being evaluated
 * @return true if the value is truthy, false otherwise
 */
constexpr bool is_truthy(const underlying_t& value) {
    return std::visit([]<typename E>(E&& v) -> bool {
        // according to evaluation system:
        // For truthyness and falsyness, we will follow the convention introduced in the book,
        // where false and nil are falsy, and everything else is truthy.
        // which is kinda weird, what about !0?
        if constexpr (std::is_same_v<std::decay_t<E>, bool>)
            return v == true;
        else if constexpr (std::is_same_v<std::decay_t<E>, nullptr_t>)
            return false;
        return true;

        // if constexpr (std::is_same_v<std::decay_t<E>, nullptr_t>)
            // return false;
        // if constexpr (std::is_same_v<std::decay_t<E>, RealNumber>)
            // return v != RealNumber{0, 0, 0, false};
        // if constexpr (std::is_same_v<std::decay_t<E>, bool>)
            // return v == true;
        // if constexpr (std::is_same_v<std::decay_t<E>, std::string_view>
                    // || std::is_same_v<std::decay_t<E>, std::string>)
            // return v != "";
        return true;
    }, value);
}

constexpr bool is_falsy(const underlying_t& value) {
    return !is_truthy(value);
}
}
#endif
