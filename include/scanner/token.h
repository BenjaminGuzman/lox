#ifndef CODECRAFTERS_INTERPRETER_TOKEN_H
#define CODECRAFTERS_INTERPRETER_TOKEN_H
#include <string>
#include <unordered_map>
#include <variant>

#include "real_number.h"

// the actual memory used for a variable will be the biggest type here...
// so it's not very efficient, but way more efficient than storing as unique_ptr, or void*, or something else...
// std::monostate for those tokens that do not represent a literal value
using underlying_t = std::variant<std::monostate, std::string, RealNumber, bool, nullptr_t>;

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
    {UNRECOGNIZED, NOT_OPERATOR},
    {UNTERMINATED_STRING, NOT_OPERATOR},
    {INVALID_NUMBER, NOT_OPERATOR},
    {AST_ROOT, NOT_OPERATOR},
};

/**
 * Generic class that stores a token
 * @tparam T the type of the literal value
 */
template<typename T>
class BasicToken {
public:
    const TokenType type = UNRECOGNIZED;

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

    [[nodiscard]] std::string string(const std::string& filepath) const;
    [[nodiscard]] const T& get_literal() const;

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

template<typename T>
const T& BasicToken<T>::get_literal() const {
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
    case LEFT_PAREN:
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

/**
 * A lightweight, non-owning view of a Token's data.
 * This provides direct, typed access to a token's literal without copying any data.
 * @tparam T the type of the token's literal
 * @tparam E the type of the "view" of the token's literal (e.g., std::string_view for a string literal)
 */
template<typename T, typename E = const T*>
class BasicTokenView {
private:
    // if it's a string, this class should own the string view (which doesn't own the string)
    //   i.e., E = std::string_view: this ->(owns) string_view ->(points to) string
    // but if it's not, then this class just owns a reference to the underlying type
    //   i.e., E = const T*: this ->(points to) literal
    E _literal;
public:
    const Token& token;

    /**
     * Creates a view from an existing Token.
     * @param token the token
     * @throws std::bad_variant_access if the token's literal is not of type T.
     */
    explicit BasicTokenView(const Token& token);

    /**
     *
     * @return the literal as a reference, i.e. @code const T& @endcode, or as a view object, e.g., @code std::string_view @endcode
     */
    [[nodiscard]] auto literal() const;
};

template<typename T, typename E>
BasicTokenView<T, E>::BasicTokenView(const Token &token) : token(token) {
    if constexpr (std::is_same_v<T, std::string>) {
        // in order to save space, literal="" iff lexeme = "literal", i.e., the literal can be computed from the lexeme
        if (std::get<std::string>(token.get_literal()).empty())
            _literal = std::string_view(token.lexeme).substr(1, token.lexeme.length() - 2);
        else // literal actually contains the literal
            _literal = std::string_view(std::get<std::string>(token.literal));
    } else
        _literal = &std::get<T>(token.get_literal());
}

template<typename T, typename E>
auto BasicTokenView<T, E>::literal() const {
    if constexpr (std::is_pointer_v<E>)
        // E = const T*, thus we shall return const E&, i.e., the dereferenced pointer
        return *_literal;
    else
        // E = std::string_view, thus we shall return E.
        return _literal;
}

using StringTokenView = BasicTokenView<std::string, std::string_view>;
using RealNumberTokenView = BasicTokenView<RealNumber>;

template<typename T>
std::string BasicToken<T>::string(const std::string& filepath) const {
    std::ostringstream basic_serialized_token_buff;
    basic_serialized_token_buff << to_string(type) << " " << lexeme << " ";
    std::visit([&basic_serialized_token_buff, this]<typename E>(E&& lit) {
        if constexpr (std::is_same_v<std::decay_t<E>, std::string>)
             basic_serialized_token_buff << StringTokenView(*this).literal();
        if constexpr (std::is_same_v<std::decay_t<E>, RealNumber>)
            basic_serialized_token_buff << to_string(lit);
        if constexpr (std::is_same_v<std::decay_t<E>, std::monostate>)
            basic_serialized_token_buff << "null";
    }, get_literal());

#ifdef TOKEN_PRETTY_PRINT
    if (type == UNRECOGNIZED)
        return "[" + filepath + ":" + std::to_string(line) + ":" + std::to_string(col) + "]: Error! Unexpected character: " + lexeme;

    if (type == UNTERMINATED_STRING)
        return "[" + filepath + ":" + std::to_string(line) + ":" + std::to_string(col) + "]: Error! Unterminated string: " + std::get<std::string>(literal);

    return basic_serialized_token_buff.str() +  " " + filepath + ":" + std::to_string(line) + ":" + std::to_string(col);
#endif

    if (type == UNRECOGNIZED)
        return "[line " + std::to_string(line) + "] Error: Unexpected character: " + lexeme;

    if (type == UNTERMINATED_STRING)
        return "[line " + std::to_string(line) + "] Error: Unterminated string.";

    return basic_serialized_token_buff.str();
}

inline std::unordered_map<TokenType, std::string> tokenTypeStrings = {
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

inline std::string to_string(const TokenType& type) {
    if (tokenTypeStrings.contains(type))
        return tokenTypeStrings[type];

    return "UNKNOWN";
}

#endif //CODECRAFTERS_INTERPRETER_TOKEN_H