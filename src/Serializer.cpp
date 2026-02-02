#include "Serializer.h"

#include <sstream>

std::string Serializer::serialize(const std::string& filepath, const std::vector<Token>& tokens) {
    std::stringstream buff;
    for (auto const& token : tokens) {
        buff << token.string(filepath);
        buff << std::endl;
    }
    return buff.str();
}
