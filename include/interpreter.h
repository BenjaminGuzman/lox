#ifndef CODECRAFTERS_INTERPRETER_INTERPRETER_H
#define CODECRAFTERS_INTERPRETER_INTERPRETER_H
#include <memory>

#include "parser.h"

namespace lox {
class Interpreter {
private:
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
    [[nodiscard]] underlying_t compileNot(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] underlying_t compileASTRoot(const std::unique_ptr<ASTNode>& astNode) const;
    [[nodiscard]] underlying_t compileParenthesis(const std::unique_ptr<ASTNode>& astNode) const;

    /**
     * Stores the token types to the function that should be responsible for compiling the token.
     */
    const std::unordered_map<TokenType, std::function<underlying_t(const std::unique_ptr<ASTNode>& astNode)>> TOKEN_COMPILERS = {
        {NUMBER,     [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileNumber(astNode);}},
        {AST_ROOT,   [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileASTRoot(astNode);}},
        {PLUS,       [this](const std::unique_ptr<ASTNode>& astNode) {return this->compilePlus(astNode);}},
        {MINUS,      [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileMinus(astNode);}},
        {NOT,        [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileNot(astNode);}},
        {LEFT_PAREN, [this](const std::unique_ptr<ASTNode>& astNode) {return this->compileParenthesis(astNode);}}
    };
public:
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