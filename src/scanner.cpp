#include "scanner.h"

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <cstring>
#include <functional>

namespace lox {
Scanner::Scanner(const std::string& filepath): filepath(filepath), filestream(filepath) {
    if (!filestream.is_open())
        throw std::runtime_error("Couldn't open file " + filepath + " to read from it. " + strerror(errno));
}
Scanner::~Scanner() {
    filestream.close();
}

Token parseString(std::ifstream& filestream, const size_t line, const size_t col) {
    std::stringstream buff;
    char c;
    while (filestream.get(c)) {
        // TODO add escape sequences??
        if (c == '"') {
            auto s = buff.str();
            return Token{STRING, '"' + s + '"', s, line, col};
        } else if (c == '\n') { // no multi-line strings supported now
            auto s = buff.str();
            filestream.putback(c); // the '\n' is not part of the string, so we need to put it back
            return Token{UNTERMINATED_STRING, "UNTERMINATED STRING", s, line, col};
        }

        buff << c;
    }

    return Token{UNTERMINATED_STRING, "UNTERMINATED STRING", buff.str(), line, col};
}

Token parseNumber(std::ifstream& filestream, const size_t line, const size_t col) {
    std::stringstream buff;

    // parse integer
    unsigned long long integer = 0;
    char c;
    while (filestream.get(c)) {
        if (std::isdigit(c)) {
            buff << c;
            integer = integer * 10 + (c - '0');
        } else {
            filestream.putback(c);
            break;
        }
    }

    // parse fractional (if any)
    unsigned long long fractional = 0;
    int fractional_digits = 0;
    if (filestream.peek() == '.') {
        filestream.get(c);
        buff << c;
        while (filestream.get(c)) {
            if (std::isdigit(c)) {
                buff << c;
                fractional = fractional * 10 + (c - '0');
                ++fractional_digits;
            } else {
                filestream.putback(c);
                break;
            }
        }

        // remove trailing 0s
        while (fractional > 0 && fractional % 10 == 0) {
            fractional /= 10;
            --fractional_digits;
        }

        // if (fractional_digits == 0) {
            // let's just say "40.whatever" makes the number 40 as a valid integer...
        // }
    }

    RealNumber number = {
        .integer = integer,
        .fractional = fractional,
        .n_fractional_digits = fractional_digits
    };

    const std::string lexeme = buff.str();
    return Token{
        .type = NUMBER,
        .lexeme = lexeme,
        .literal = number,
        .line = line,
        .col = col
    };
}

Token Scanner::nextToken() {
    if (!filestream.is_open())
        return Token{EOF_TOKEN, "", std::monostate{}, line, col};

    for (char c; filestream.get(c); ++col /* this is needed here! only after scanning a char, the col increments */) {
        switch (c) {
        // non-composite or especial chars
        case '(':
        case ')':
        case '{':
        case '}':
        case ',':
        case '.':
        case '-':
        case '+':
        case ';':
        case '*': {
            auto lexeme = std::string(1, c);
            return Token{TOKEN_STRING_MAPPING.at(lexeme), lexeme, std::monostate{}, line, col};
        }
        // composite chars (they all can be composed with =, making the token <=, ==, >=, !=)
        case '=':
        case '!':
        case '>':
        case '<': {
            auto lexeme = std::string(1, c);
            if (filestream.peek() == '=') { // process the current char + '=' (<=, ==, ...)
                filestream.get();
                lexeme += "=";
                auto token = Token{TOKEN_STRING_MAPPING.at(lexeme), lexeme, std::monostate{}, line, col};
                ++col;
                return token;
            }
            return Token{TOKEN_STRING_MAPPING.at(lexeme), lexeme, std::monostate{}, line, col};
        }
        case '\n':
            ++line;
            col = 0;
            break;
        case '/':
            if (filestream.peek() == '/') { // process the // (comment)
                char c2;
                while (filestream.get(c2) && c2 != '\n') {} // ignore everything until the next line

                // next line or EOF was reached
                ++line;
                col = 0;
            } else
                return Token{SLASH, "/", std::monostate{}, line, col};
            break;
        case ' ':
        case '\t':
            break;
        case '"': { // parse strings
            Token stringToken = parseString(filestream, line, col);

            // col should point to the last char of the string, regardless of it is terminated or not
            // if string is terminated, col = |'"'| - 1 + |"the lexeme"| = |"the lexeme"| = last '"'
            // if string is not terminated, col = |'"'| - 1 + |"the lexeme| = |"the lexeme| = last 'e'
            col = -1 /*(remove the initial '"')*/ + stringToken.lexeme.length();
            return stringToken;
        }
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '0': { // parse number regardless of their sign
            // "unread" the digit. The first call to get() from parseNumber should return the first digit
            filestream.putback(c);

            Token number = parseNumber(filestream, line, col);

            // col currently points to the first char in the lexeme (first digit), so col = index_of_first_digit + 1
            // but col should point to the last char of the lexeme/digit, so we need
            // col = col + |lexeme| - 1 = index_of_first_digit + 1 + |lexeme| - 1 = index_of_first_digit + |lexeme|
            //     = index_of_last_char
            // we need it at the last char 'cause the next iteration will increment it and put it in the next
            // non-number char.
            // Example col = 3, contents = "  789". col = 3 + |"789"| - 1 = 3 + 3 - 1 = 5
            col += number.lexeme.length() - 1;
            return number;
        }
        default:
            // scan identifiers and keywords
            std::stringstream buff;
            if (!(std::isalpha(c) || std::isdigit(c) || c == '_')) // not a valid identifier/keyword
                return Token{UNRECOGNIZED, std::string(1, c), std::monostate{}, line, col};

            buff << c;
            while (filestream.get(c)) {
                if (std::isalpha(c) || std::isdigit(c) || c == '_')
                    buff << c;
                else {
                    filestream.putback(c);
                    break;
                }
            }

            std::string lexeme = buff.str();

            // col currently points to the first char in the lexeme, so col = index_of_first_char + 1
            // but col should point to the last char of the lexeme, so we need
            // col = col + |lexeme| - 1 = index_of_first_char + 1 + |lexeme| - 1 = index_of_first_char + |lexeme|
            //     = index_of_last_char
            // (this is the same logic for the numbers)
            // we need it at the last char 'cause the next iteration will increment it and put it in the next
            // non-identifier char
            // Example col = 3, contents = "  abc". col = 3 + |"abc"| - 1 = 3 + 3 - 1 = 5
            col += lexeme.length() - 1;

            if (TOKEN_STRING_MAPPING.contains(lexeme)) // if it is a keyword
                // TODO will adding the lexeme occupy more memory? I mean, will we have 2+ copies of "while"?
                return Token{TOKEN_STRING_MAPPING.at(lexeme), lexeme, std::monostate{}, line, col};

            return Token{IDENTIFIER, lexeme, std::monostate{}, line, col};
        }
    }

    filestream.close();
    return Token{EOF_TOKEN, "", std::monostate{}, line, col};
}

double RealNumber::to_double() const {
    return static_cast<double>(integer) + static_cast<double>(fractional) / pow(10, n_fractional_digits);
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
    {STRING, "STRING"},
    {NUMBER, "NUMBER"},
    {IDENTIFIER, "IDENTIFIER"},

    // keywords
    {AND, "AND"},
    {OR, "OR"},
    {CLASS, "CLASS"},
    {IF, "IF"},
    {ELSE, "ELSE"},
    {TRUE, "TRUE"},
    {FALSE, "FALSE"},
    {FOR, "FOR"},
    {WHILE, "WHILE"},
    {FUN, "FUN"},
    {RETURN, "RETURN"},
    {SUPER, "SUPER"},
    {THIS, "THIS"},
    {VAR, "VAR"},
    {NIL, "NIL"},
    {PRINT, "PRINT"},
};
std::string to_string(const TokenType& type) {
    if (tokenTypeStrings.contains(type))
        return tokenTypeStrings[type];

    return "UNKNOWN";
}

std::string to_string(const RealNumber& number) {
    return std::to_string(number.integer) + "." + std::to_string(number.fractional);
}
}