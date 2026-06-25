#ifndef CODECRAFTERS_INTERPRETER_INTERPRETER_H
#define CODECRAFTERS_INTERPRETER_INTERPRETER_H
#include <any>
#include <memory>
#include <vector>
#include <ranges>

#include "parser.h"
#include "interpreter/stackframe.h"
#include "interpreter/user_function.h"

namespace lox {
/**
 * Function type to register an error during the execution of the program
 * @param msg the message
 * @param token the token at which the error was produced
 */
using registerErrorF = std::function<void(const std::string& msg, const Token& token)>;

class Interpreter {
private:
    const registerErrorF registerError;

    /**
     * The execution stack
     */
    std::vector<std::shared_ptr<Stackframe>> stack;

    /**
     * Stack of callback functions to be executed when found a return statement
     * Each callback corresponds to a function in execution, i.e., if there are 2 callbacks in the stack,
     * that means there are 2 functions being executed, the callback at the top of the stack, when called, will make
     * the most recent executed function return
     */
    std::vector<std::function<void(std::vector<underlying_t>& returnValues)>> returnFunctions;

    /**
     * Flag that tells whether the stack should be unwinded, e.g., a return statement has been executed.
     */
    bool shouldUnwindStack = false;

    /**
     * Registers the global native functions into the first stackframe of the program.
     * the "name" of the functions is actually their signature, composed of the function's name and
     * the number of parameters, i.e.,
     * for the function clock(), the name is "clock_0",
     * for clock(string), the name is "clock_1",
     * for clock(...), i.e., varargs, the name is "clock"
     *
     * The signature was decided to include the number of args instead of their types
     * (e.g., "clock_string") 'cause the lang is loosely typed.
     */
    void registerGlobalNativeFunctions();

    /**
     *
     * @return the stackframe at the top of the stack
     */
    Stackframe& stackTop() {
        return *stack.back();
    }

    /**
     * Generic function that compiles binary operators defined only for @link RealNumber @endlink type operands
     * e.g., '>', '<', '>=', '<='
     * @tparam R return type for the operator
     * @param astNode the AST node to be evaluated (itself should be the operator, its children should be the args for the operator)
     * @param op the operator function
     * @return the result of the operator
     */
    // FIXME BEGIN FIXME
    template<typename R>
    R compileOnlyRealNumberDefinedBinaryOperator(const std::unique_ptr<ASTNode>& astNode, std::function<R(const RealNumber&, const RealNumber&)> op) const {
        auto lhs = this->resolveOrExecute(astNode->children[0]);
        auto rhs = this->resolveOrExecute(astNode->children[1]);

        return std::visit([&op]<typename l_type, typename r_type>(l_type&& l, r_type&& r) -> underlying_t {
            if constexpr (std::is_same_v<std::decay_t<l_type>, RealNumber> && std::is_same_v<std::decay_t<r_type>, RealNumber>)
                return op(l, r);

            throw std::runtime_error("Invalid operands for '>' binary operator."); // FIXME improve this
        }, lhs, rhs);
    }
    bool compileOnlyRealNumberDefinedBinaryOperator(const std::unique_ptr<ASTNode>& astNode, std::function<bool(const RealNumber&, const RealNumber&)> op) const;
    // FIXME END FIXME

    [[nodiscard]] underlying_t compileNumber(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] underlying_t compileString(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] underlying_t compilePlus(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] underlying_t compileMinus(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] underlying_t compileStar(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] underlying_t compileSlash(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] underlying_t compileNot(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] underlying_t compileASTRoot(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] underlying_t compileParenthesis(const std::unique_ptr<ASTNode>& astNode) const;

    [[nodiscard]] underlying_t compileGreaterThan(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] underlying_t compileGreaterThanOrEqual(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] underlying_t compileLessThan(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] underlying_t compileLessThanOrEqual(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] underlying_t compileEqualEqual(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] underlying_t compileNotEqual(const std::unique_ptr<ASTNode>& astNode) const;

    [[nodiscard]] underlying_t compilePrint(const std::unique_ptr<ASTNode>& astNode) const;

    // these functions modify the state (namely, the stack or the symbol table) of the interpreter
    [[nodiscard]] underlying_t compileEqual(const std::unique_ptr<ASTNode>& astNode);
    [[nodiscard]] underlying_t compileVar(const std::unique_ptr<ASTNode>& astNode);
    [[nodiscard]] underlying_t compileLeftBrace(const std::unique_ptr<ASTNode>& astNode);
    [[nodiscard]] std::vector<underlying_t> compileFuncCall(const std::unique_ptr<ASTNode>& astNode);
    [[nodiscard]] underlying_t compileFunc(const std::unique_ptr<ASTNode>& astNode);
    [[nodiscard]] underlying_t compileReturn(const std::unique_ptr<ASTNode>& astNode);

    [[nodiscard]] underlying_t compileIf(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] underlying_t compileWhile(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] underlying_t compileFor(const std::unique_ptr<ASTNode>& astNode);

    [[nodiscard]] underlying_t compileOr(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] underlying_t compileAnd(const std::unique_ptr<ASTNode>& astNode) const;

    /**
     * Stores the token types to the function that should be responsible for compiling the token.
     */
    const std::unordered_map<TokenType, std::function<underlying_t(const std::unique_ptr<ASTNode>& astNode)>> TOKEN_COMPILERS = {
        {NUMBER,     [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileNumber(astNode);}},
        {AST_ROOT,   [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileASTRoot(astNode);}},
        {PLUS,       [this](const std::unique_ptr<ASTNode>& astNode) {return this->compilePlus(astNode);}},
        {MINUS,      [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileMinus(astNode);}},
        {STAR,       [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileStar(astNode);}},
        {SLASH,      [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileSlash(astNode);}},
        {NOT,        [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileNot(astNode);}},
        {LEFT_PAREN, [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileParenthesis(astNode);}},

        {GT,         [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileGreaterThan(astNode);}},
        {GTE,        [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileGreaterThanOrEqual(astNode);}},
        {LT,         [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileLessThan(astNode);}},
        {LTE,        [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileLessThanOrEqual(astNode);}},
        {EQEQ,       [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileEqualEqual(astNode);}},
        {NEQ,        [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileNotEqual(astNode);}},

        {PRINT,      [this](const std::unique_ptr<ASTNode>& astNode) {return this->compilePrint(astNode);}},

        {EQ,         [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileEqual(astNode);}},
        {VAR,        [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileVar(astNode);}},
        {LEFT_BRACE, [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileLeftBrace(astNode);}},
        {FUNC_CALL,  [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileFuncCall(astNode)[0];}}, // FIXME add support for multi-return
        {FUN,        [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileFunc(astNode);}},
        {RETURN,     [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileReturn(astNode);}}, // FIXME add support for multi-return

        {IF,         [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileIf(astNode);}},
        {WHILE,      [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileWhile(astNode);}},
        {FOR,        [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileFor(astNode);}},

        {OR,         [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileOr(astNode);}},
        {AND,        [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileAnd(astNode);}},
    };
public:
    /**
     * Tells whether the output should be restricted to only print functions.
     * If false and the interpreted command produces a value, i.e., it's not an assignment
     * the value will be printed to stdout.
     */
    bool strict_output_mode = false;

    /**
     *
     * @param registerError function that gets called when an error during execution is found
     */
    Interpreter(registerErrorF registerError) : registerError(std::move(registerError)) {
        this->stack.emplace_back(std::make_shared<Stackframe>());
        this->registerGlobalNativeFunctions();
    };

    /**
     * Default constructor with a default function to print errors to stderr
     */
    Interpreter() : Interpreter([](const std::string& msg, const Token& token) {
        std::cerr << msg << std::endl;
    }) {}

    /**
     * Searches in the stack for the given symbol. Search is done top to bottom, i.e., latest variable that matches will
     * be always returned first
     * @param symbolName the name of the symbol to search for
     * @return the value of the symbol, or @link std::monostate @endlink if not found
     */
    [[nodiscard]] underlying_t getVar(const std::string& symbolName) const;

    /**
     * Executes/evaluates an ASTNode.
     * This will print to stdout the results of the evaluation.
     * @param root the AST node to be executed
     */
    [[nodiscard]] underlying_t execute(const std::unique_ptr<ASTNode>& root) const;

    /**
     * Deletes the first symbol found in the stack (top to bottom, i.e., recent symbols are deleted first)
     * @param symbol the name of the symbol to remove from the stack
     */
    void deleteFromStack(const std::string& symbol) {
        for (auto& it : std::ranges::views::reverse(stack))
            if (it->symbols.contains(symbol)) {
                it->symbols.erase(symbol);
            }
    }

    /**
     * Resolves the value for a token
     *
     * This will only resolve the token in the following cases:
     * - If it represents a number, the number is returned
     * - If it represents a string, the string is returned
     * - If it represents an identifier (aka variable), the value of the symbol is returned
     * - If it represents a boolean, the boolean is returned
     * - If it represents a null/nil, the null is returned
     *
     * For any other cases, this will return @link std::monostate @endlink
     * @param token the token
     * @return the resolved value of the token
     */
    [[nodiscard]] underlying_t resolveToken(const Token& token) const;

    /**
     * Resolves the value of the node by calling @link resolveToken(const Token&) @endlink,
     * or by calling @link execute(const std::unique_ptr<ASTNode>&) @endlink if needed.
     * @param node the node whose value shall be resolved
     * @return the resolved value of the node
     */
    [[nodiscard]] underlying_t resolveOrExecute(const std::unique_ptr<ASTNode>& node) const;

    friend class UserFunction;
};

[[nodiscard]] std::string to_string(bool b);
//[[nodiscard]] std::ostream& operator<<(std::ostream& os, bool b);
}

#endif //CODECRAFTERS_INTERPRETER_INTERPRETER_H