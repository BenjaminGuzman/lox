#ifndef CODECRAFTERS_INTERPRETER_PARSER_H
#define CODECRAFTERS_INTERPRETER_PARSER_H
#include "scanner.h"
#include <memory>
#include <stack>

namespace lox {
/**
 * The identifier for the last returned high-order function. This is mainly used for chaining function calls,
 * e.g. f(x)(y), where f(x) returns g(y)
 */
const std::string LAST_HO_FUNCTION_IDENTIFIER = "__CHAIN_F__";

class ASTNode {
public:
    const Token token;
    std::vector<std::unique_ptr<ASTNode>> children;
    ASTNode* parent;

    /**
     * Indicates the token operation type that this operator must have (based on the context)
     */
    std::optional<TokenOpType> must_be_op_type;

    /**
     *
     * @return the operator type of the token as in @link TokenOpType @endlink, except that this is not ambiguous, if
     * the operator by its nature can be unary or binary, this method will resolve whether it is unary or binary, thus
     * resolving only @link TokenOpType#UNARY @endlink OR @link TokenOpType#BINARY @endlink.
     */
    [[nodiscard]] TokenOpType op_type() const;
};

class AST {
private:
    Scanner& scanner;
    void handle_binary_operators(const Token& curr_token, ASTNode* parent, std::stack<ASTNode*>& parentNodes) const;
public:
    std::unique_ptr<ASTNode> root;
    AST(Scanner& scanner, bool autobuild = true);

    /**
     * Builds the tree using the provided scanner
     * @return number of errors
     */
    [[nodiscard]] int build() const;

    /**
     * Parses the next statement from the scanner and returns it.
     * Useful for REPLs.
     */
    [[nodiscard]] const std::unique_ptr<ASTNode>& build_next_statement() const;
};

}

#endif //CODECRAFTERS_INTERPRETER_PARSER_H