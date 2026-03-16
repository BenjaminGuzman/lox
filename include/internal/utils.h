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
}
#endif
