#ifndef CODECRAFTERS_INTERPRETER_SCANNER_H
#define CODECRAFTERS_INTERPRETER_SCANNER_H
#include <string>
#include <vector>

inline bool PRETTY_PRINT = false;

enum TokenType {
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    EOF_TOKEN
};

std::string to_string(const TokenType& type);

class Token {
public:
    const TokenType type;

    /**
     * Lexeme. Actual sequence of chars that form the token. May be null iff type = EOF
     */
    const std::string lexeme;

    /**
     * The actual value that composes the token. May be null if it makes sense for the lexeme to not have a value
     * (e.g., EOF, parenthesis, etc...)
     */
    const std::string literal;

    /**
     * Line in the file at which the token was found
     */
    const int line;

    /**
     * Column in the file at which the token was found
     */
    const int col;

    std::string string(const std::string& filepath) const {
        std::string literal_value = literal.empty() ? "null" : literal;
        std::string basic_serialized_token = to_string(type) + " " + lexeme + " " + literal_value;

        if (PRETTY_PRINT)
            return basic_serialized_token +  " " + filepath + ":" + std::to_string(line) + ":" + std::to_string(col);

        return basic_serialized_token;
    }
};

class Scanner {
public:
    /**
     * Tries to read the contents of a file.
     * @param filepath the path to the file to read
     * @return the contents of the file
     * @throws std::runtime_error if failed to read the file
     */
    static std::string read_file(const std::string& filepath);

    /**
     * Tries to tokenize the contents of a file.
     * @param filepath the path to the file to read
     * @return the tokens of the file
     * @throws std::runtime_error propagated from @link Scanner::read_file @endlink
     */
    static std::vector<Token> tokenize(const std::string& filepath);
};


#endif //CODECRAFTERS_INTERPRETER_SCANNER_H