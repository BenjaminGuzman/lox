#ifndef CODECRAFTERS_INTERPRETER_INTERPRETER_H
#define CODECRAFTERS_INTERPRETER_INTERPRETER_H
#include <memory>

#include "parser.h"

namespace lox {
/**
 * Function type to register an error during the execution of the program
 * @param msg the message
 * @param token the token at which the error was produced
 */
using registerErrorF = std::function<void(const std::string& msg, const Token& token)>;

class Interpreter {
private:
    registerErrorF registerError;

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
    };
public:
    /**
     *
     * @param registerError function that gets called when an error during execution is found
     */
    Interpreter(registerErrorF registerError) : registerError(std::move(registerError)) {};

    /**
     * Default constructor with a default function to print errors to stderr
     */
    Interpreter() : Interpreter([](const std::string& msg, const Token& token) {
        std::cerr << msg << std::endl;
    }) {}

    /**
     * Executes/evaluates an ASTNode.
     * This will print to stdout the results of the evaluation.
     * @param root the AST node to be executed
     */
    [[nodiscard]] underlying_t execute(const std::unique_ptr<ASTNode>& root) const;
};

[[nodiscard]] std::string to_string(bool b);
//[[nodiscard]] std::ostream& operator<<(std::ostream& os, bool b);
}

#endif //CODECRAFTERS_INTERPRETER_INTERPRETER_H