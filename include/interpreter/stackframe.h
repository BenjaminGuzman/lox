#ifndef CODECRAFTERS_INTERPRETER_STACKFRAME_H
#define CODECRAFTERS_INTERPRETER_STACKFRAME_H
#include <unordered_map>

#include "scanner/token.h"

namespace lox {
class Stackframe {
    /**
     * Stackframe below this stackframe, i.e., the stackframe that was at the top before this stackframe
     * was inserted into the stack
     */
    const Stackframe* prev;

    /**
     * Stackframe above this stackframe, i.e., the stackframe that was inserted after this stackframe
     */
    std::unique_ptr<Stackframe> next;

    /**
     * Symbol table for the current stackframe
     */
    std::unordered_map<std::string, underlying_t> symbols;

    /**
     * Makes a new stackframe on top of this stackframe
     * @return the new stackframe
     */
    StackFrame& makeStackFrame();

    /**
     * Drop the current stackframe
     */
    void drop();
};
} // lox

#endif //CODECRAFTERS_INTERPRETER_STACKFRAME_H
