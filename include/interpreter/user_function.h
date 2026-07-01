#ifndef CODECRAFTERS_INTERPRETER_USERFUNCTION_H
#define CODECRAFTERS_INTERPRETER_USERFUNCTION_H
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "parser.h"
#include "scanner/token.h"

namespace lox {
class Interpreter;
struct Stackframe;

/**
 * Represents a function that can be used in lox
 */
class UserFunction {
public:
    /**
     * Function's name
     */
    const std::string name;

    /**
     * Function's parameters names (order is preserved, i.e., first param name corresponds to first
     * parameter)
     */
    const std::vector<std::string> paramNames;

    /**
     * Stores default values for parameters, mapping parameter name to its expression ASTNode
     */
    const std::unordered_map<std::string, const std::unique_ptr<ASTNode>*> defaultParams;

    /**
     * Points to the '{' of the function, i.e., the open brace
     * that contains the full body of the function
     */
    const std::unique_ptr<ASTNode>& funcBody;

    /**
     * If the function is a native function, this field stores it. This implies @link isNative @endlink is true
     * Function receives the function args, and the interpreter the function may use (e.g., to resolve variables)
     */
    const std::function<std::vector<underlying_t>(const std::vector<std::unique_ptr<ASTNode>>&, Interpreter&)> nativeFunc = nullptr;

    /**
     * Tells whether the function is a user-defined or a native function
     */
    const bool isNative = false;

    /**
     * The captured lexical environment (closure), stored as a list of shared stack frames.
     */
    std::vector<std::shared_ptr<Stackframe>> closure;

    /**
     * Execute the user-defined function with the given arguments
     * @param interpreter the interpreter running this function.
     * This function may call other methods of the interpreter, e.g., resolveOrExecute to resolve variables used
     * by the function
     * @param args the arguments for the function
     * @return the return value of the function
     */
    std::vector<underlying_t> execute(Interpreter& interpreter, const std::vector<std::unique_ptr<ASTNode>>& args) const;

    /**
     * Same as @link UserFunction::execute @endlink
     * @see UserFunction::execute
     */
    std::vector<underlying_t> operator()(Interpreter& interpreter, const std::vector<std::unique_ptr<ASTNode>>& args) const;
};
}

#endif //CODECRAFTERS_INTERPRETER_USERFUNCTION_H
