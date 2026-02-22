#ifndef CODECRAFTERS_INTERPRETER_PARSER_H
#define CODECRAFTERS_INTERPRETER_PARSER_H
#include "scanner.h"
#include <memory>

namespace lox {
class ASTNode {
public:
    const Token token;
    std::vector<std::unique_ptr<ASTNode>> children;
    ASTNode* parent;

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
public:
    std::unique_ptr<ASTNode> root;
    AST(Scanner& scanner, bool autobuild = true);

    void build();
};

}

#endif //CODECRAFTERS_INTERPRETER_PARSER_H