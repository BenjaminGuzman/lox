#ifndef CODECRAFTERS_INTERPRETER_SCANNER_H
#define CODECRAFTERS_INTERPRETER_SCANNER_H
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <list>

inline bool PRETTY_PRINT = false;

namespace lox {
struct RealNumber {
    unsigned long long integer{};
    unsigned long long fractional{};
    int n_fractional_digits{};
    bool is_negative = false;
    [[nodiscard]] double to_double() const;
};
std::string to_string(const RealNumber& number);
std::string to_string(const RealNumber* type);

// the actual memory used for a variable will be the biggest type here...
// so it's not very efficient, but way more efficient than storing as unique_ptr, or void*, or something else...
// std::monostate for those tokens that do not represent a literal value
using underlying_t = std::variant<std::monostate, std::string, RealNumber>;

// Token operator type
enum TokenOpType {
    NOT_OPERATOR, // token doesn't represent an operator
    UNARY, // token is a unary operator
    BINARY, // token is a binary operator
    UNARY_OR_BINARY, // token can either be a unary or binary operator
    BINARY_OR_NOT_OPERATOR, // token can either be a binary operator or not an operator at all
    MULTI // token is a multi-token operator
};

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
    INVALID_NUMBER,

    AST_ROOT, // especial value
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

inline const std::unordered_map<TokenType, TokenOpType> TOKEN_OP_TYPE_MAPPING = {
    {STRING, NOT_OPERATOR},
    {NUMBER, NOT_OPERATOR},
    {IDENTIFIER, NOT_OPERATOR},
    {LEFT_PAREN, MULTI},
    {RIGHT_PAREN, NOT_OPERATOR},
    {LEFT_BRACE, MULTI},
    {RIGHT_BRACE, NOT_OPERATOR},
    {COMMA, BINARY}, // ?
    {DOT, NOT_OPERATOR}, // ?
    {MINUS, UNARY_OR_BINARY},
    {PLUS, UNARY_OR_BINARY},
    {SEMICOLON, NOT_OPERATOR},
    {SLASH, BINARY_OR_NOT_OPERATOR},
    {STAR, BINARY},
    {EQ, BINARY},
    {EQEQ, BINARY},
    {NOT, UNARY},
    {NEQ, BINARY},
    {LT, BINARY},
    {LTE, BINARY},
    {GT, BINARY},
    {GTE, BINARY},
    {AND, BINARY},
    {OR, BINARY},
    {CLASS, NOT_OPERATOR},
    {IF, UNARY},
    {ELSE, NOT_OPERATOR},
    {TRUE, NOT_OPERATOR},
    {FALSE, NOT_OPERATOR},
    {FOR, NOT_OPERATOR},
    {WHILE, NOT_OPERATOR},
    {FUN, NOT_OPERATOR},
    {RETURN, NOT_OPERATOR},
    {SUPER, NOT_OPERATOR},
    {THIS, NOT_OPERATOR},
    {VAR, NOT_OPERATOR},
    {NIL, NOT_OPERATOR},
    {PRINT, NOT_OPERATOR},
    {AST_ROOT, NOT_OPERATOR},
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
     * The actual value that composes the token. Prefer the get_literal method
     */
    const T literal;

    /**
     * Line in the file at which the token was found
     */
    const size_t line{};

    /**
     * Column in the file at which the token was found
     */
    const size_t col{};

    BasicToken& operator=(const BasicToken& other) {
        if (this == &other)
            return *this;

        const_cast<TokenType&>(type) = other.type;
        const_cast<std::string&>(lexeme) = other.lexeme;
        const_cast<T&>(literal) = other.literal;
        const_cast<size_t&>(line) = other.line;
        const_cast<size_t&>(col) = other.col;
        return *this;
    }

    [[nodiscard]] std::string string(const std::string& filepath) const;
    [[nodiscard]] T get_literal() const;

    /**
     * @return the operator type of the token as in @link TokenOpType @endlink
     */
    [[nodiscard]] TokenOpType op_type() const;

    /**
     *
     * @return true if the token can be an operand of an arithmetic expression
     */
    [[nodiscard]] bool can_be_arithmetic_operand() const;

    /**
     *
     * @return true if the token is an arithmetic operator
     */
    [[nodiscard]] bool is_arithmetic_operator() const;

    [[nodiscard]] bool can_be_comparison_operand() const;
    [[nodiscard]] bool is_comparison_operator() const;

    /**
     *
     * @return the operator priority, the higher the value, the higher the priority
     */
    [[nodiscard]] uint16_t op_priority() const;
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
        literal(std::get<T>(token.get_literal())),
        line(token.line),
        col(token.col) {}
};

template<typename T>
using parsingFunctionT = std::function<T(std::ifstream& filestream, const size_t line, const size_t col)>;

/**
 * Parses a string token in the next characters provided by the filestream
 * @param filestream the filestream. First call to @link std::get(char) @endlink should return the first character that
 * is said to be part of the string, i.e., this index shouldn't point to '"' unless the string is empty.
 * At the return of this function, the next call to @link std::get(char) @endlink on this filestream
 * should return the next char that is not part of the string.
 * @param line line at which the string starts (used only to store it in the returned token)
 * @param col col at which the string starts (used only to store it in the returned token)
 * @return the token which can be of type @link TokenType::STRING @endlink
 * or @link TokenType::UNTERMINATED_STRING @endlink.
 */
Token parseString(std::ifstream& filestream, const size_t line, const size_t col);

/**
 * Parses a number token in the next characters provided by the filestream
 * @param filestream the filestream. First call to @link std::get(char) @endlink should return the first character that
 * is said to be part of the number
 * @param line line at which the number starts (used just to store it in the returned token)
 * @param col col at which the number starts (used just to store it in the returned token)
 * @return the token which can be of type @link TokenType::NUMBER @endlink
 */
Token parseNumber(std::ifstream& filestream, const size_t line, const size_t col);

/**
 * Scans tokens.
 * This is a forward-only scanner.
 */
class Scanner {
private:
    std::ifstream filestream;

    /**
     * Column at which this scanner currently is
     */
    size_t col = 1;

    /**
     * Line at which this scanner currently is
     */
    size_t line = 1;

    /**
     * Stores the previous (0), current (1), and next (2) tokens
     */
    std::list<Token> tokens = {};

    [[nodiscard]] Token _next_token();
public:
    const std::string& filepath;

    explicit Scanner(const std::string& filepath);
    ~Scanner();

    /**
     * Scans the next token of the file
     * @note this will advance the file iterator
     * @return the next token of the file
     */
    [[nodiscard]] Token next_token();

    /**
     * Returns the next token of the file without moving the pointer/iterator so that the next call to
     * @link next_token() @endlink shall return the same token.
     * @return the next token of the file.
     */
    [[nodiscard]] Token peek_next() const;

    /**
     * Returns the previous token returned by the last call to @link next_token() @endlink
     * If no call to @link next_token() @endlink has been made, you shall ignore the output of this function
     * (or better yet, don't call it)
     * @return the previous token
     */
    [[nodiscard]] Token peek_previous() const;
};

template<typename T>
T BasicToken<T>::get_literal() const {
    if (type == STRING)
        return lexeme.substr(1, lexeme.length() - 2);

    return literal;
}

template<typename T>
TokenOpType BasicToken<T>::op_type() const {
    return TOKEN_OP_TYPE_MAPPING.at(type);
}

template<typename T>
bool BasicToken<T>::can_be_arithmetic_operand() const {
    switch (type) {
    case NUMBER:
    case STRING:
    case IDENTIFIER:
    case RIGHT_PAREN: // a group can be an operand, e.g. (10 + 9) * 9
        return true;
    default:
        return false;
    }
}

template<typename T>
bool BasicToken<T>::is_arithmetic_operator() const {
    switch (type) {
    case PLUS:
    case MINUS:
    case STAR:
    case SLASH:
        return true;
    default:
        return false;
    }
}

template<typename T>
bool BasicToken<T>::can_be_comparison_operand() const {
    switch (type) {
    case NUMBER:
    case STRING:
    case IDENTIFIER:
    case RIGHT_PAREN: // a group can be an operand, e.g. (10 + 9) < 9
        return true;
    default:
        return false;
    }
}

template<typename T>
bool BasicToken<T>::is_comparison_operator() const {
    switch (type) {
    case EQEQ:
    case NEQ:
    case GT:
    case GTE:
    case LT:
    case LTE:
        return true;
    default:
        return false;
    }
}

template<typename T>
uint16_t BasicToken<T>::op_priority() const {
    switch (type) {
    case PLUS:
    case MINUS:
        return 10;
    case STAR:
    case SLASH:
        return 20;
    case LEFT_PAREN:
        return 30;
    default:
        return 0;
    }
}

template<typename T>
std::string BasicToken<T>::string(const std::string& filepath) const {
    std::string literal_value = std::visit([]<typename E>(E&& lit) {
            if constexpr (std::is_same_v<std::decay_t<E>, std::string>)
                return lit.empty() ? "null" : lit;
            if constexpr (std::is_same_v<std::decay_t<E>, RealNumber>)
                return to_string(lit);
            if constexpr (std::is_same_v<std::decay_t<E>, std::monostate>)
                return std::string("null");
            return std::string("");
        }, get_literal());

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
}
#endif //CODECRAFTERS_INTERPRETER_SCANNER_H