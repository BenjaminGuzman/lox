#include "Serializer.h"

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