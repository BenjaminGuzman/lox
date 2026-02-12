#include "serializer.h"

#include <iostream>
#include <numeric>
#include <sstream>
#include <stack>

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

std::string to_string_parsed(const Token& token) {
    switch (token.type) {
    case LEFT_PAREN:
        return "(group";
    case RIGHT_PAREN:
        return ")";
    default:{}
    }

    bool should_use_lexeme = false;
    std::string serialization = std::visit([&should_use_lexeme]<typename E>(E&& lit) {
        if constexpr (std::is_same_v<E, const std::string&> || std::is_same_v<E, std::string>)
            return lit;
        if constexpr (std::is_same_v<E, const RealNumber&> || std::is_same_v<E, RealNumber>)
            return to_string(lit);
        if constexpr (std::is_same_v<E, const std::monostate&> || std::is_same_v<E, std::monostate>) {
            should_use_lexeme = true;
            return std::string("");
        }
        return std::string("");
    }, token.get_literal());

    if (should_use_lexeme)
        serialization = token.lexeme;

    return serialization;
}

int Serializer::serialize(const AST& ast) {
    std::stack<const ASTNode*> nodes;
    std::vector<std::string> serialized_tokens;
    nodes.push(&ast.root);
    while (!nodes.empty()) {
        const auto curr = nodes.top();
        nodes.pop();

        serialized_tokens.push_back(to_string_parsed(curr->token));
        if (!curr->children.empty()) {
            for (size_t i = curr->children.size() - 1; i != 0; --i)
                nodes.push(&curr->children.at(i));
            nodes.push(&curr->children.at(0));
        }
    }

    const std::string s = std::accumulate(
        std::next(serialized_tokens.begin()),
        serialized_tokens.end(),
        serialized_tokens.at(0),
        [](const std::string& a, const std::string& b) {
            if (b == ")")
                return a + b;
            if (a.empty())
                return b;
            if (b.empty())
                return a;

            return a + " " + b;
        }
    );
    std::cout << s << std::endl;

    return 0;
}