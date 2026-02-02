#include "Scanner.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <cstring>
#include <functional>


std::string Scanner::read_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open())
        throw std::runtime_error("Couldn't open file " + filepath + " to read from it. " + strerror(errno));

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return buffer.str();
}

std::vector<Token> Scanner::tokenize(const std::string& filepath) {
    std::string contents = read_file(filepath);
    std::vector<Token> tokens;

    size_t line = 1;
    size_t col = 0;
    for (size_t i = 0; i < contents.length(); ++i) {
        char c = contents[i];
        ++col;
        switch (c) {
        case '(':
            tokens.emplace_back(LEFT_PAREN, "(", "", line, col);
            break;
        case ')':
            tokens.emplace_back(RIGHT_PAREN, ")", "", line, col);
            break;
        case '{':
            tokens.emplace_back(LEFT_BRACE, "{", "", line, col);
            break;
        case '}':
            tokens.emplace_back(RIGHT_BRACE, "}", "", line, col);
            break;
        case ',':
            tokens.emplace_back(COMMA, ",", "", line, col);
            break;
        case '.':
            tokens.emplace_back(DOT, ".", "", line, col);
            break;
        case '-':
            tokens.emplace_back(MINUS, "-", "", line, col);
            break;
        case '+':
            tokens.emplace_back(PLUS, "+", "", line, col);
            break;
        case ';':
            tokens.emplace_back(SEMICOLON, ";", "", line, col);
            break;
        case '/':
            tokens.emplace_back(SLASH, "/", "", line, col);
            break;
        case '*':
            tokens.emplace_back(STAR, "*", "", line, col);
            break;
        case '\n':
            ++line;
            col = 1;
            break;
        case ' ':
        case '\t':
            break;
        default:
            std::cerr << "Unrecognized token " << c << " at " << line << ":" << col << std::endl;
         }
    }

    tokens.emplace_back(EOF_TOKEN, "", "", line, col);

    return tokens;
}


std::pmr::unordered_map<TokenType, std::string> tokenTypeStrings = {
    {LEFT_PAREN, "LEFT_PAREN"},
    {RIGHT_PAREN, "RIGHT_PAREN"},
    {EOF_TOKEN, "EOF"},
    {LEFT_BRACE, "LEFT_BRACE"},
    {RIGHT_BRACE, "RIGHT_BRACE"},
    {COMMA, "COMMA"},
    {DOT, "DOT"},
    {MINUS, "MINUS"},
    {PLUS, "PLUS"},
    {SEMICOLON, "SEMICOLON"},
    {SLASH, "SLASH"},
    {STAR, "STAR"}
};
std::string to_string(const TokenType& type) {
    if (tokenTypeStrings.contains(type))
        return tokenTypeStrings[type];

    return "UNKNOWN";
}