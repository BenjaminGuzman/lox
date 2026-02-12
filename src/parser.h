#ifndef CODECRAFTERS_INTERPRETER_PARSER_H
#define CODECRAFTERS_INTERPRETER_PARSER_H
#include "scanner.h"

namespace lox {
struct ASTNode {
    Token token;
    std::vector<ASTNode> children;
};

class AST {
private:
    Scanner& scanner;
    void build(ASTNode& node);
public:
    ASTNode root;
    AST(Scanner& scanner, bool autobuild = true);

    void build();
};

}

#endif //CODECRAFTERS_INTERPRETER_PARSER_H