#include "interpreter.h"

#include <chrono>

namespace lox::native_funcs {
std::vector<underlying_t> clock(const std::vector<std::unique_ptr<ASTNode>>& args, Interpreter& interpreter) {
    const auto now = std::chrono::system_clock::now();
    const auto epoch = now.time_since_epoch();
    return {RealNumber{
        .integer = static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::seconds>(epoch).count())
    }};
}

std::vector<underlying_t> deleteVar(const std::vector<std::unique_ptr<ASTNode>>& args, Interpreter& interpreter) {
    for (const auto& arg: args)
        interpreter.deleteFromStack(arg->token.lexeme);

    return {std::monostate()};
}
}

namespace lox {
void Interpreter::registerGlobalNativeFunctions() {
    stackTop().symbols["clock_0"] = std::make_shared<UserFunction>(UserFunction{
        .name = "clock",
        .paramNames = {},
        .funcBody = nullptr,
        .nativeFunc = native_funcs::clock,
        .isNative = true
    });
    stackTop().symbols["clock"] = stackTop().symbols["clock_0"]; // make the "multi-arg" function clock point to clock_0

    stackTop().symbols["delete"] = std::make_shared<UserFunction>(UserFunction{
        .name = "delete",
        .paramNames = {},
        .funcBody = nullptr,
        .nativeFunc = native_funcs::deleteVar,
        .isNative = true
    });
}
}
