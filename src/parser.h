#ifndef CODECRAFTERS_INTERPRETER_PARSER_H
#define CODECRAFTERS_INTERPRETER_PARSER_H
#include "scanner.h"
#include <memory>
#include <stack>

namespace lox {
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

    void build();
};

}

#endif //CODECRAFTERS_INTERPRETER_PARSER_H