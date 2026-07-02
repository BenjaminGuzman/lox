#include "scanner.h"

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <cstring>

namespace lox {
Scanner::Scanner(const std::string& filepath, bool is_filepath): filepath(filepath), stream({}) {
    if (is_filepath) {
        filestream.open(filepath);
        stream.rdbuf(filestream.rdbuf());
    } else {
        stringstream.str(filepath); // filepath is not actually a filepath but the string to be scanned
        stream.rdbuf(stringstream.rdbuf());
    }

    if ((is_filepath && !filestream.good()) || !stream.good())
        throw std::runtime_error("Couldn't open \"" + filepath + "\" to read from it. " + strerror(errno));

    tokens.push_back({}); // add the previous token
    tokens.push_back({}); // add the current token
    tokens.push_back(_next_token()); // add the next token
}

Scanner::~Scanner() = default;

void Scanner::feed(const std::string& input) {
    if (filestream.is_open())
        filestream.close();
    stringstream.str(input);
    stringstream.clear(); // Clear EOF flags
    stream.rdbuf(stringstream.rdbuf());
    
    tokens.clear();
    tokens.push_back({});
    tokens.push_back({});
    tokens.push_back(_next_token());
}

Token parse_string(std::istream& stream, const size_t line, const size_t col) {
    std::stringstream lexeme_buff;
    lexeme_buff << '"';
    std::stringstream literal_buff;
    char c;
    while (stream.get(c)) {
        lexeme_buff << c;
        if (c == '\\') { // add support for escape sequences
            if (!stream.get(c)) // string ended at \, e.g. "hello bad string\"
                return Token{UNTERMINATED_STRING, lexeme_buff.str(), literal_buff.str(), line, col};

            lexeme_buff << c;
            switch (c) {
            case '"':
                literal_buff << '"';
                break;
            case 't':
                literal_buff << '\t';
                break;
            case 'n':
                literal_buff << '\n';
                break;
            default:
                // add the unrecognized escape sequence as-is
                literal_buff << '\\' << c;
            }
            continue;
        }

        if (c == '"') {
            const auto& lexeme = lexeme_buff.str();
            const auto& literal_ = literal_buff.str();

            // if lexeme = "literal", then the literal can be computed from the lexeme by removing the quotes,
            // so, in order to save space store only an empty string
            const auto& literal = lexeme.size() == literal_.size() + 2 ? "" : literal_;
            return Token{STRING, lexeme_buff.str(), literal, line, col};
        }
        // if (c == '\n') { // no multi-line strings supported now
            // const auto& s = literal_buff.str();
            // stream.putback(c); // the '\n' is not part of the string, so we need to put it back
            // return Token{UNTERMINATED_STRING, lexeme_buff.str(), s, line, col};
        // }

        literal_buff << c;
    }

    return Token{UNTERMINATED_STRING, literal_buff.str(), literal_buff.str(), line, col};
}

Token parse_number(std::istream& stream, const size_t line, const size_t col) {
    std::stringstream buff;

    // parse integer
    unsigned long long integer = 0;
    char c;
    while (stream.get(c)) {
        if (std::isdigit(c)) {
            buff << c;
            integer = integer * 10 + (c - '0');
        } else {
            stream.putback(c);
            break;
        }
    }

    // parse fractional (if any)
    unsigned long long fractional = 0;
    int fractional_digits = 0;
    if (stream.peek() == '.') {
        stream.get(c);
        buff << c;
        while (stream.get(c)) {
            if (std::isdigit(c)) {
                buff << c;
                fractional = fractional * 10 + (c - '0');
                ++fractional_digits;
            } else {
                stream.putback(c);
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

Token Scanner::_next_token() {
    if (stream.eof())
        return Token{EOF_TOKEN, "", std::monostate{}, line, col + 1};

    for (char c; stream.get(c);) {
        ++col; /* this is needed here! only after scanning a char, the col increments */
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
        case ':':
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
            if (stream.peek() == '=') { // process the current char + '=' (<=, ==, ...)
                stream.get();
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
            if (stream.peek() == '/') { // process the // (comment)
                char c2;
                while (stream.get(c2) && c2 != '\n') {} // ignore everything until the next line

                // next line or EOF was reached
                ++line;
                col = 0;
            } else
                return Token{SLASH, "/", std::monostate{}, line, col};
            break;
        case '\r':
        case ' ':
        case '\t':
            break;
        case '"': { // parse strings
            Token stringToken = parse_string(stream, line, col);

            // col should point to the last char of the string, regardless of it is terminated or not
            // if string is terminated, col = |'"'| - 1 + |"the lexeme"| = |"the lexeme"| = last '"'
            // if string is not terminated, col = |'"'| - 1 + |"the lexeme| = |"the lexeme| = last 'e'
            col += -1 /*(remove the initial '"')*/ + stringToken.lexeme.length();
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
            stream.putback(c);

            Token number = parse_number(stream, line, col);

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
            while (stream.get(c)) {
                if (std::isalpha(c) || std::isdigit(c) || c == '_')
                    buff << c;
                else {
                    stream.putback(c);
                    break;
                }
            }

            std::string lexeme = buff.str();

            size_t token_col = col;
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
                return Token{TOKEN_STRING_MAPPING.at(lexeme), lexeme, std::monostate{}, line, token_col};

            return Token{IDENTIFIER, lexeme, std::monostate{}, line, token_col};
        }
    }

    if (filestream.is_open())
        filestream.close();
    return Token{EOF_TOKEN, "", std::monostate{}, line, col + 1};
}

Token Scanner::next_token() {
    auto& next_token = tokens.back();
    tokens.pop_front(); // pop the previous token
    tokens.push_back(_next_token()); // add the next (actually the next-next token) token
    return next_token;
}

Token Scanner::peek_next() const {
    auto& token = tokens.back();
    return token;
}

Token Scanner::peek_current() const {
    return *std::next(tokens.begin(), 1);
}

Token Scanner::peek_previous() const {
    auto& next_token = tokens.front();
    return next_token;
}

void Scanner::skip_next() {
    tokens.pop_front(); // pop the previous token
    tokens.push_back(_next_token()); // add the next (actually the next-next token) token
}
}
