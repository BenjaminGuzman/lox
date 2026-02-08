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
    for (const auto& [token, children] : ast.root.children) {
        bool should_use_lexeme = false;
        std::string serialization = std::visit([&should_use_lexeme]<typename E>(E&& lit) {
            if constexpr (std::is_same_v<E, const std::string&>)
                return lit;
            if constexpr (std::is_same_v<E, const RealNumber&>)
                return to_string(lit);
            if constexpr (std::is_same_v<E, const std::monostate&>) {
                should_use_lexeme = true;
                return std::string("");
            }
            return std::string("");
        }, token.literal);

        if (should_use_lexeme)
            serialization = token.lexeme;

        std::cout << serialization << std::endl;
    }
    return 0;
}