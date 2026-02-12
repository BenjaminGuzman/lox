#include "parser.h"

namespace lox {

AST::AST(Scanner& scanner, bool autobuild) : scanner(scanner), root({AST_ROOT, "", std::monostate{}, 0, 0}) {
    if (autobuild)
        build();
}

void AST::build(ASTNode &node) {
    while (true) {
        Token token = scanner.nextToken();
        if (token.type == EOF_TOKEN)
            break;

        switch (token.type) {
        case LEFT_PAREN: {
            ASTNode group = {
                .token = token,
                .children = {}
            };
            build(group);
            node.children.push_back(group);
            break;
        }
        default:
            node.children.emplace_back(token);
        }
    }
}

void AST::build() {
    build(root);
}
}
