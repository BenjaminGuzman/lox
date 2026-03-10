#ifndef CODECRAFTERS_INTERPRETER_SCANNER_H
#define CODECRAFTERS_INTERPRETER_SCANNER_H
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <string_view>
#include <variant>
#include <vector>
#include <list>
#include <type_traits>
#include <sstream>

#include "scanner/real_number.h"
#include "scanner/token.h"

namespace lox {
template<typename T>
using parsingFunctionT = std::function<T(std::ifstream& filestream, const size_t line, const size_t col)>;

/**
 * Parses a string token in the next characters provided by the filestream
 * @param stream the stream. First call to @link std::get(char) @endlink should return the first character that
 * is said to be part of the string, i.e., this index shouldn't point to '"' unless the string is empty.
 * At the return of this function, the next call to @link std::get(char) @endlink on this filestream
 * should return the next char that is not part of the string.
 * @param line line at which the string starts (used only to store it in the returned token)
 * @param col col at which the string starts (used only to store it in the returned token)
 * @return the token which can be of type @link TokenType::STRING @endlink
 * or @link TokenType::UNTERMINATED_STRING @endlink.
 */
Token parse_string(std::istream& stream, const size_t line, const size_t col);

/**
 * Parses a number token in the next characters provided by the filestream
 * @param stream the stream. First call to @link std::get(char) @endlink should return the first character that
 * is said to be part of the number
 * @param line line at which the number starts (used just to store it in the returned token)
 * @param col col at which the number starts (used just to store it in the returned token)
 * @return the token which can be of type @link TokenType::NUMBER @endlink
 */
Token parse_number(std::istream& stream, const size_t line, const size_t col);

/**
 * Scans tokens.
 * This is a forward-only scanner.
 */
class Scanner {
private:
    std::ifstream filestream;
    std::istringstream stringstream;
    std::istream stream;

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
    /**
     * The filepath the scanner is reading the tokens from.
     * If it's reading it directly from a string, then this value is empty.
     */
    const std::string filepath;

    /**
     * Constructs the scanner from either a filepath or a string
     * @param s the content to be scanned, or the filepath to the file containing the content to be scanned
     * @param is_filepath true if the given string should be treated as a filepath
     */
    explicit Scanner(const std::string& s, bool is_filepath=true);
    ~Scanner();

    /**
     * Scans the next token of the file
     * @note this will advance the file iterator, i.e., it's NOT idempotent and it's an inmpure function
     * @return the next token of the file
     */
    [[nodiscard]] Token next_token();

    /**
     * Returns the next token of the file without moving the pointer/iterator so that the next call to
     * @link next_token() @endlink shall return the same token.
     * @note This method doesn't modify the file iterator, it's an idempotent and pure function
     * @return the next token of the file.
     */
    [[nodiscard]] Token peek_next() const;

    /**
     * Returns the current token as per returned by the last call to @link next_token() @endlink
     * If no call to @link next_token() @endlink has been made, you shall ignore the output of this function
     * (or better yet, don't call it)
     * It's recommended that you cache this value yourself rather than calling this method
     * @note This method doesn't modify the file iterator, it's an idempotent and pure function
     * @return the current token returned by the last call to @link next_token() @endlink
     */
    [[nodiscard]] Token peek_current() const;

    /**
     * Returns the previous token returned by the last call to @link next_token() @endlink
     * If no call to @link next_token() @endlink has been made, you shall ignore the output of this function
     * (or better yet, don't call it)
     * @note This method doesn't modify the file iterator, it's an idempotent and pure function
     * @return the previous token
     */
    [[nodiscard]] Token peek_previous() const;
};

}
#endif //CODECRAFTERS_INTERPRETER_SCANNER_H