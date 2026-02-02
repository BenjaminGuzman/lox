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

/**
 * Generators for tokens that are composite. These should be composed of an = (e.g., <=, >=, !=, ==)
 * Maps a token (a) to the function (f) that would generate the composite token (b) which is equal to a + =,
 * e.g., f(LT) = LTE,
 */
std::unordered_map<TokenType, std::function<Token(size_t, size_t)>> EQUAL_COMPOSITE_TOKENS_MAKERS = {
    {EQ, [](size_t line, size_t col){ return Token{EQ_EQ, "==", "", line, col}; }},
    {LT, [](size_t line, size_t col){ return Token{LTE, "<=", "", line, col}; }},
    {GT, [](size_t line, size_t col){ return Token{GTE, ">=", "", line, col}; }},
    {NOT, [](size_t line, size_t col){ return Token{NEQ, "!=", "", line, col}; }}
};

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
        case '<':
            tokens.emplace_back(LT, "<", "", line, col);
            break;
        case '>':
            tokens.emplace_back(GT, ">", "", line, col);
            break;
        case '!':
            tokens.emplace_back(NOT, "!", "", line, col);
            break;
        case '\n':
            ++line;
            col = 1;
            break;
        case '=':
            if (!tokens.empty()) {
                auto previous_token = tokens.back().type;

                bool stop_processing_curr_token = false;

                // check if the previous token was token (a) that in composition with the current token (=) would form
                // a composite token (b)
                switch (previous_token) {
                case EQ: // would form >=
                case NOT: // would form !=
                case LT: // would form <=
                case GT: {
                    // would form >=
                    tokens.pop_back(); // remove the previous token as it shall be replaced by the composite

                    auto makeCompositeTokenFn = EQUAL_COMPOSITE_TOKENS_MAKERS.at(previous_token);
                    auto composite_token = makeCompositeTokenFn(line, col);

                    tokens.push_back(composite_token);
                    stop_processing_curr_token = true; // current token has already been processed
                    break;
                }
                default: break;
                }

                if (stop_processing_curr_token)
                    break;
            }

            tokens.emplace_back(EQ, "=", "", line, col);
            break;
        // case ' ':
        // case '\t':
            // break;
        default:
            tokens.emplace_back(UNRECOGNIZED, std::string(1, c), "", line, col);
            break;
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
    {STAR, "STAR"},
    {EQ, "EQUAL"},
    {EQ_EQ, "EQUAL_EQUAL"},
    {NOT, "BANG"},
    {NEQ, "BANG_EQUAL"},
    {LT, "LESS"},
    {LTE, "LESS_EQUAL"},
    {GT, "GREATER"},
    {GTE, "GREATER_EQUAL"},
};
std::string to_string(const TokenType& type) {
    if (tokenTypeStrings.contains(type))
        return tokenTypeStrings[type];

    return "UNKNOWN";
}