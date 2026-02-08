#include "serializer.h"

#include <iostream>
#include <sstream>

using namespace lox;

int Serializer::serialize(Scanner& scanner) {
    int n_unrecognized = 0;
    while (true) {
        Token token = scanner.nextToken();
        if (token.type == EOF_TOKEN) {
            std::cout << token.string(scanner.filepath) << std::endl;
            break;
        }

        if (token.type == UNRECOGNIZED || token.type == UNTERMINATED_STRING) {
            std::cerr << token.string(scanner.filepath) << std::endl;
            ++n_unrecognized;
            continue;
        }

        std::cout << token.string(scanner.filepath) << std::endl;
    }

    return n_unrecognized;
}

int Serializer::serialize(const AST& ast) {
    for (const auto& [token, children] : ast.root.children)
        std::cout << token.lexeme << std::endl;
    return 0;
}
