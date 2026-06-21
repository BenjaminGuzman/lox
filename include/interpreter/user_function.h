#ifndef CODECRAFTERS_INTERPRETER_USERFUNCTION_H
#define CODECRAFTERS_INTERPRETER_USERFUNCTION_H
#include <string>

#include "parser.h"
#include "scanner/token.h"

namespace lox {
class Interpreter;

/**
 * Represents a user-defined function
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
     * Points to the '{' of the function, i.e., the open brace
     * that contains the full body of the function
     */
    const std::unique_ptr<ASTNode>& funcBody;

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
