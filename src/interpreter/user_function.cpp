#include "interpreter/user_function.h"

#include <ranges>

#include "interpreter.h"

namespace lox {
std::vector<underlying_t> UserFunction::execute(Interpreter& interpreter, const std::vector<std::unique_ptr<ASTNode>>& args) const {
    std::unordered_map<std::string, underlying_t> namedParams = args
        | std::views::filter([&interpreter](const std::unique_ptr<ASTNode>& astNode) {
            if (astNode->token.type == COLON) {
                if (astNode->children.size() != 2) {
                    interpreter.registerError("Invalid use of : operator", astNode->token);
                    return false;
                }
                return true;
            }
            return false;
        })
        | std::views::transform([&interpreter](const std::unique_ptr<ASTNode>& astNode) {
            return std::make_pair(astNode->children[0]->token.lexeme, interpreter.resolveOrExecute(astNode->children[1]));
        })
        | std::ranges::to<std::unordered_map<std::string, underlying_t>>();

    // create a stackframe for the function and put all the args there, i.e., assign the value (arg) to the name (param)
    interpreter.stack.emplace_back();
    auto& stackSymbols = interpreter.stackTop().symbols;
    for (size_t i = 0; i < paramNames.size(); ++i) {
        if (namedParams.contains(paramNames[i])) // if given as named arg
            stackSymbols[paramNames[i]] = namedParams[paramNames[i]];
        else if (i < args.size()) // if given as positional arg
            stackSymbols[paramNames[i]] = interpreter.resolveOrExecute(args[i]);
        else // not given, look in the default parameters (TODO)
            stackSymbols[paramNames[i]] = nullptr;
    }

    std::vector<underlying_t> retValues;
    bool shouldReturn = false;
    interpreter.returnFunctions.emplace_back([&retValues, &shouldReturn](std::vector<underlying_t>& ret) {
        retValues = std::move(ret);
        shouldReturn = true;
    });

    // execute the function
    for (const auto& statement : this->funcBody->children) {
        auto _ = interpreter.execute(statement);
        if (shouldReturn)
            break;
    }

    // pop the function's stackframe, avoid memory leaks!
    interpreter.stack.pop_back();
    interpreter.returnFunctions.pop_back();

    if (retValues.empty())
        return {nullptr};

    return retValues;
}

std::vector<underlying_t> UserFunction::operator()(Interpreter& interpreter, const std::vector<std::unique_ptr<ASTNode>>& args) const {
    return this->execute(interpreter, args);
}
}
