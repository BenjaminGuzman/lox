#include "interpreter.h"

#include <chrono>

namespace lox::native_funcs {
underlying_t clock(ASTNode* astNode, Interpreter& interpreter) {
    const auto now = std::chrono::system_clock::now();
    const auto epoch = now.time_since_epoch();
    return RealNumber{
        .integer = static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::seconds>(epoch).count())
    };
}
}

namespace lox {
std::unordered_map<std::string, std::function<underlying_t(ASTNode*, Interpreter& interpreter)>> Interpreter::NATIVE_FUNCTIONS = {
    {"clock_0", native_funcs::clock},
    {"delete", [](ASTNode* astNode, Interpreter& interpreter) {
            for (const auto& arg: astNode->children)
                interpreter.deleteFromStack(arg->token.lexeme);

            return std::monostate();
        }},
};
}
