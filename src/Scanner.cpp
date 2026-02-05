#include "Scanner.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <cstring>
#include <functional>

#define SKIP_NEXT_CHAR(i, col)\
    ++i;\
    ++col;


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
 * Identifies the string token in the given string
 * @param str the string.
 * @param str_start_at index that should point to the first character that is said to be part of the string
 * (i.e., this index shouldn't point to '"' unless the string is empty)
 * @param line line at which the string starts (used just to store in the returned token)
 * @param col col at which the string starts (used just to store in the returned token)
 * @return the token which can be of type @link TokenType::STRING @endlink
 * or @link TokenType::UNTERMINATED_STRING @endlink. The returned token won't have the line/col populated
 */
Token identifyString(const std::string& str, const size_t str_start_at, const size_t line, const size_t col) {
    for (size_t i = str_start_at; i < str.length(); ++i) {
        char c = str.at(i);
        // TODO add escape sequences??
        if (c == '"') {
            size_t str_len = i - str_start_at;
            const std::string s = str.substr(str_start_at, str_len);
            return Token{STRING, '"' + s + '"', s, line, col};
        } else if (c == '\n') { // no multi-line strings supported now
            size_t str_len = i - str_start_at;
            return Token{UNTERMINATED_STRING, "UNTERMINATED STRING", str.substr(str_start_at, str_len), line, col};
        }
    }
    return Token{UNTERMINATED_STRING, "UNTERMINATED STRING", str.substr(str_start_at), line, col};
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
            if (i + 1 < contents.length() && contents[i + 1] == '/') { // process the // (comment)
                while (i < contents.length() && contents[i] != '\n') // ignore everything until the next line
                    ++i;
                // i is now at EOF, or past the next \n, which we didn't process, so we need to process it (go the case)
                --i;
            } else
                tokens.emplace_back(SLASH, "/", "", line, col);
            break;
        case '*':
            tokens.emplace_back(STAR, "*", "", line, col);
            break;
        case '<':
            if (i + 1 < contents.length() && contents[i + 1] == '=') { // process the <=
                tokens.emplace_back(LTE, "<=", "", line, col);
                SKIP_NEXT_CHAR(i, col);
            } else
                tokens.emplace_back(LT, "<", "", line, col);
            break;
        case '>':
            if (i + 1 < contents.length() && contents[i + 1] == '=') { // process the >=
                tokens.emplace_back(GTE, ">=", "", line, col);
                SKIP_NEXT_CHAR(i, col);
            } else
                tokens.emplace_back(GT, ">", "", line, col);
            break;
        case '!':
            if (i + 1 < contents.length() && contents[i + 1] == '=') { // process the !=
                tokens.emplace_back(NEQ, "!=", "", line, col);
                SKIP_NEXT_CHAR(i, col);
            } else
                tokens.emplace_back(NOT, "!", "", line, col);
            break;
        case '\n':
            ++line;
            col = 1;
            break;
        case '=':
            if (i + 1 < contents.length() && contents[i + 1] == '=') { // process the ==
                tokens.emplace_back(EQEQ, "==", "", line, col);
                SKIP_NEXT_CHAR(i, col);
            } else
                tokens.emplace_back(EQ, "=", "", line, col);
            break;
        case ' ':
        case '\t':
            break;
        case '"': {
            Token stringToken = identifyString(contents, i + 1, line, col);

            // advance the pointers to the next char that is not part of the string
            col += stringToken.literal.length() + 1; // + 1 due to the last '"' (or '\n'/EOF if unterminated)
            i += stringToken.literal.length() + 1;
            if (stringToken.type == UNTERMINATED_STRING)
                // currently i is at either the '\n' (or EOF), so we need to go back, to process the '\n'
                --i;

            tokens.push_back(stringToken);
            break;
        } default:
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
    {EQEQ, "EQUAL_EQUAL"},
    {NOT, "BANG"},
    {NEQ, "BANG_EQUAL"},
    {LT, "LESS"},
    {LTE, "LESS_EQUAL"},
    {GT, "GREATER"},
    {GTE, "GREATER_EQUAL"},
    {STRING, "STRING"}
};
std::string to_string(const TokenType& type) {
    if (tokenTypeStrings.contains(type))
        return tokenTypeStrings[type];

    return "UNKNOWN";
}