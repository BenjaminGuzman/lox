#ifndef CODECRAFTERS_INTERPRETER_SCANNER_H
#define CODECRAFTERS_INTERPRETER_SCANNER_H
#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

inline bool PRETTY_PRINT = false;

struct RealNumber {
    unsigned long long integer{};
    unsigned long long fractional{};
    int n_fractional_digits{};
    bool is_negative = false;
};
std::string to_string(const RealNumber& number);
std::string to_string(const RealNumber* type);

// the actual memory used for a variable will be the biggest type here...
// so it's not very efficient, but way more efficient than storing as unique_ptr, or void*, or something else...
using underlying_t = std::variant<std::string, RealNumber>;

enum TokenType {
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    COMMA,
    DOT,
    MINUS,
    PLUS,
    SEMICOLON,
    SLASH,
    STAR,
    EQ, // EQUAL
    EQEQ, // EQUAL EQUAL
    NOT, // NEGATION (!)
    NEQ, // NOT EQUAL
    LT, // LESS THAN
    LTE, // LESS THAN OR EQUAL
    GT, // GREATER THAN
    GTE, // GREATER THAN OR EQUAL
    EOF_TOKEN,
    STRING,
    NUMBER,
    IDENTIFIER,

    // KEYWORDS
    AND,
    OR,
    CLASS,
    IF,
    ELSE,
    TRUE,
    FALSE,
    FOR,
    WHILE,
    FUN,
    RETURN,
    SUPER,
    THIS,
    VAR,
    NIL,
    PRINT,


    /**
     * Especial value for al those tokens that are not recognized
     */
    UNRECOGNIZED,
    UNTERMINATED_STRING,
    INVALID_NUMBER
};
std::string to_string(const TokenType& type);
inline const std::unordered_map<std::string, TokenType> TOKEN_STRING_MAPPING = {
    {"(", LEFT_PAREN},
    {")", RIGHT_PAREN},
    {"{", LEFT_BRACE},
    {"}", RIGHT_BRACE},
    {",", COMMA},
    {".", DOT},
    {"-", MINUS},
    {"+", PLUS},
    {";", SEMICOLON},
    {"/", SLASH},
    {"*", STAR},
    {"=", EQ},
    {"==", EQEQ},
    {"!", NOT},
    {"!=", NEQ},
    {"<", LT},
    {"<=", LTE},
    {">", GT},
    {">=", GTE},
    {"and", AND},
    {"or", OR},
    {"class", CLASS},
    {"if", IF},
    {"else", ELSE},
    {"true", TRUE},
    {"false", FALSE},
    {"for", FOR},
    {"while", WHILE},
    {"fun", FUN},
    {"return", RETURN},
    {"super", SUPER},
    {"this", THIS},
    {"var", VAR},
    {"nil", NIL},
    {"print", PRINT},
};

template<typename T>
class BasicToken {
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
    const T literal;

    /**
     * Line in the file at which the token was found
     */
    const size_t line;

    /**
     * Column in the file at which the token was found
     */
    const size_t col;

    std::string string(const std::string& filepath) const;
};

using Token = BasicToken<underlying_t>;

/**
 * A lightweight, non-owning view of a Token's data.
 * This provides direct, typed access to a token's literal without copying any data.
 */
template<typename T>
struct BasicTokenView {
    const TokenType type;
    const std::string& lexeme;
    const T& literal;
    const size_t line;
    const size_t col;

    /**
     * Creates a view from an existing Token.
     * @param token the token
     * @throws std::bad_variant_access if the token's literal is not of type T.
     */
    explicit BasicTokenView(const Token& token) :
        type(token.type),
        lexeme(token.lexeme),
        literal(std::get<T>(token.literal)),
        line(token.line),
        col(token.col) {}
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


template<typename T>
std::string BasicToken<T>::string(const std::string& filepath) const {
    std::string literal_value = std::visit([]<typename E>(E&& lit) {
            if constexpr (std::is_same_v<E, const std::string&>)
                return lit.empty() ? "null" : lit;
            if constexpr (std::is_same_v<E, const RealNumber&>)
                return to_string(lit);
            return std::string("");
        }, literal);

    std::string basic_serialized_token = to_string(type) + " " + lexeme + " " + literal_value;

    if (PRETTY_PRINT) {
        if (type == UNRECOGNIZED)
            return "[" + filepath + ":" + std::to_string(line) + ":" + std::to_string(col) + "]: Error! Unexpected character: " + lexeme;

        if (type == UNTERMINATED_STRING)
            return "[" + filepath + ":" + std::to_string(line) + ":" + std::to_string(col) + "]: Error! Unterminated string: " + std::get<std::string>(literal);

        return basic_serialized_token +  " " + filepath + ":" + std::to_string(line) + ":" + std::to_string(col);
    }

    if (type == UNRECOGNIZED)
        return "[line " + std::to_string(line) + "] Error: Unexpected character: " + lexeme;

    if (type == UNTERMINATED_STRING)
        return "[line " + std::to_string(line) + "] Error: Unterminated string.";

    return basic_serialized_token;
}

#endif //CODECRAFTERS_INTERPRETER_SCANNER_H