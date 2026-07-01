#include "interpreter/user_function.h"

#include <ranges>

#include "interpreter.h"

namespace lox {
std::vector<underlying_t> UserFunction::execute(Interpreter& interpreter, const std::vector<std::unique_ptr<ASTNode>>& args) const {
    if (args.size() > paramNames.size()) {
        interpreter.registerError("Function '" + name + "' receives " + std::to_string(paramNames.size())
            + " params but " + std::to_string(args.size()) + " were given", args[paramNames.size()]->token);
        return {nullptr};
    }

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

    std::vector<underlying_t> positionalArgs = args
        | std::views::transform([&interpreter](const std::unique_ptr<ASTNode>& astNode) {
            return astNode->token.type == COLON
                ? nullptr // if it's a named arg, ignore it
                : interpreter.resolveOrExecute(astNode); // if it's a positional arg, simply resolve its value
        })
        | std::ranges::to<std::vector<underlying_t>>();

    // swap interpreter's stack with the captured closure stack
    auto originalStack = std::move(interpreter.stack);
    interpreter.stack = this->closure;

    // create a stackframe for the function and put all the args there, i.e., assign the value (arg) to the name (param)
    interpreter.stack.emplace_back(std::make_shared<Stackframe>());
    auto& stackSymbols = interpreter.stackTop().symbols;
    for (size_t i = 0; i < paramNames.size(); ++i) {
        if (namedParams.contains(paramNames[i])) // if given as named arg
            stackSymbols[paramNames[i]] = namedParams[paramNames[i]];
        else if (i < args.size() && args[i]->token.type != COLON) // if given as positional arg
            stackSymbols[paramNames[i]] = positionalArgs[i];
        else if (defaultParams.contains(paramNames[i])) {
            // evaluate the default parameter expression at call time
            stackSymbols[paramNames[i]] = interpreter.resolveOrExecute(*defaultParams.at(paramNames[i]));
        } else {
            // evaluation system expects failure in this case
            const auto a = BasicToken<underlying_t>{
                .type = FUNC_CALL,
                .lexeme = name,
                .literal = std::string(name),
                .line = 0,
                .col = 0
            };
            interpreter.registerError("Parameter '" + paramNames[i] + "' of function '" + name + "' was not given", a);
            interpreter.stack.pop_back();
            interpreter.stack = std::move(originalStack); // restore caller's stack
            interpreter.shouldUnwindStack = false;
            return {nullptr};
        }
    }

    std::vector<underlying_t> retValues;

    if (isNative) {
        retValues = nativeFunc(args, interpreter);
    } else {
        interpreter.returnFunctions.emplace_back([&retValues, &interpreter](std::vector<underlying_t>& ret) {
            retValues = std::move(ret);
            interpreter.shouldUnwindStack = true;
        });

        // execute the function
        // TODO implement tail recursion
        for (const auto& statement : this->funcBody->children) {
            auto _ = interpreter.execute(statement);
            if (interpreter.shouldUnwindStack)
                break;
        }
        interpreter.returnFunctions.pop_back();
    }

    // pop the function's stackframe, avoid memory leaks!
cleanup:
    interpreter.stack.pop_back();
    interpreter.stack = std::move(originalStack); // restore caller's stack
    interpreter.shouldUnwindStack = false;

    if (retValues.empty())
        return {nullptr};

    std::visit([&interpreter]<typename E>(E&& v) {
        // if it's a high-order function store it just in case there are chained calls to it
        if constexpr (std::is_same_v<std::decay_t<E>, std::shared_ptr<UserFunction>>)
            interpreter.stackTop().symbols[LAST_HO_FUNCTION_IDENTIFIER] = v;
    }, retValues[0]);

    return retValues;
}

std::vector<underlying_t> UserFunction::operator()(Interpreter& interpreter, const std::vector<std::unique_ptr<ASTNode>>& args) const {
    return this->execute(interpreter, args);
}
}
