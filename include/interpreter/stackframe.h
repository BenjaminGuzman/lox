#ifndef CODECRAFTERS_INTERPRETER_STACKFRAME_H
#define CODECRAFTERS_INTERPRETER_STACKFRAME_H
#include <unordered_map>

#include "scanner/token.h"

namespace lox {
class Stackframe {
public:
    /**
     * Symbol table for the current stackframe
     */
    std::unordered_map<std::string, underlying_t> symbols = {};
};
} // lox

#endif //CODECRAFTERS_INTERPRETER_STACKFRAME_H
