#include "Serializer.h"

#include <iostream>
#include <sstream>

int Serializer::serialize(const std::string& filepath, const std::vector<Token>& tokens) {
    int n_unrecognized = 0;
    for (auto const& token : tokens) {
        if (token.type == UNRECOGNIZED) {
            std::cerr << token.string(filepath) << std::endl;
            ++n_unrecognized;
            continue;
        }

        std::cout << token.string(filepath) << std::endl;
    }

    return n_unrecognized;
}
