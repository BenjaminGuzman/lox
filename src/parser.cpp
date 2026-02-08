#include "parser.h"

namespace lox {
AST::AST(Scanner& scanner, bool autobuild) : scanner(scanner), root({AST_ROOT, "", std::monostate{}, 0, 0}) {
    if (autobuild)
        build();
}

void AST::build() {
    while (true) {
        Token token = scanner.nextToken();
        if (token.type == EOF_TOKEN)
            break;

        root.children.emplace_back(token);
    }
}
}
