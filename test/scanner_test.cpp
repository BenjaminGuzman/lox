#include <gtest/gtest.h>
#include "scanner.h"
#include <tuple>

namespace lox {

TEST(ScannerTest, HandlesEmptySource) {
    Scanner scanner("", false);
    Token token = scanner.next_token();
    EXPECT_EQ(token.type, EOF_TOKEN) << "Expected EOF token, received " << to_string_as_expected_by_evaluation_system(token.type);
}

TEST(ScannerTest, ScansSingleCharacterTokens) {
    Scanner scanner("(){},.-+;*/", false);
    std::vector expected_types = {
        LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE, COMMA, DOT,
        MINUS, PLUS, SEMICOLON, STAR, SLASH, EOF_TOKEN
    };

    for (const auto& expected_type : expected_types) {
        Token token = scanner.next_token();
        EXPECT_EQ(token.type, expected_type) << "Expected " << to_string_as_expected_by_evaluation_system(expected_type) << ", received " << to_string_as_expected_by_evaluation_system(token.type);
    }
}

TEST(ScannerTest, ScansMultiCharacterTokens) {
    Scanner scanner("! != = == > >= < <=", false);
    std::vector<TokenType> expected_types = {
        NOT, NEQ, EQ, EQEQ, GT, GTE, LT, LTE, EOF_TOKEN
    };

    for (const auto& expected_type : expected_types) {
        Token token = scanner.next_token();
        EXPECT_EQ(token.type, expected_type) << "Expected " << to_string_as_expected_by_evaluation_system(expected_type) << ", received " << to_string_as_expected_by_evaluation_system(token.type);
    }
}

TEST(ScannerTest, IgnoresComments) {
    Scanner scanner("= // this is a comment\n==", false);
    Token token1 = scanner.next_token();
    EXPECT_EQ(token1.type, EQ) << "Expected EQ token, received " << to_string_as_expected_by_evaluation_system(token1.type);
    Token token2 = scanner.next_token();
    EXPECT_EQ(token2.type, EQEQ) << "Expected EQEQ token, received " << to_string_as_expected_by_evaluation_system(token2.type);
    Token token3 = scanner.next_token();
    EXPECT_EQ(token3.type, EOF_TOKEN) << "Expected EOF token, received " << to_string_as_expected_by_evaluation_system(token3.type);
}

TEST(ScannerTest, HandlesUnterminatedComment) {
    Scanner scanner("= // this is an unterminated comment", false);
    Token token1 = scanner.next_token();
    EXPECT_EQ(token1.type, EQ) << "Expected EQ token, received " << to_string_as_expected_by_evaluation_system(token1.type);
    Token token2 = scanner.next_token();
    EXPECT_EQ(token2.type, EOF_TOKEN) << "Expected EOF token, received " << to_string_as_expected_by_evaluation_system(token2.type);
}

TEST(ScannerTest, IgnoresWhitespace) {
    Scanner scanner(" \t \r \n ", false);
    const auto& next_token = scanner.next_token();
    EXPECT_EQ(next_token.type, EOF_TOKEN) << "Expected EOF token, received " << to_string_as_expected_by_evaluation_system(next_token.type);
}

TEST(ScannerTest, TracksLineNumbers) {
    Scanner scanner("\n\n\n", false);
    Token token = scanner.next_token();
    EXPECT_EQ(token.type, EOF_TOKEN) << "Expected EOF token, received " << to_string_as_expected_by_evaluation_system(token.type);
}

TEST(ScannerTest, TracksColumnNumbers) {
    Scanner scanner("var a = 10;\n\"hello\"  world", false);
    std::vector<std::tuple<TokenType, size_t, size_t>> expected_tokens = {
        // line 1
        {VAR, 1, 1},
        {IDENTIFIER, 1, 5},
        {EQ, 1, 7},
        {NUMBER, 1, 9},
        {SEMICOLON, 1, 11},
        // line 2
        {STRING, 2, 1},
        {IDENTIFIER, 2, 10},
        {EOF_TOKEN, 2, 15}
    };

    for (const auto& expected : expected_tokens) {
        Token token = scanner.next_token();
        EXPECT_EQ(token.type, std::get<0>(expected)) << "Expected " << to_string_as_expected_by_evaluation_system(std::get<0>(expected)) << ", received " << to_string_as_expected_by_evaluation_system(token.type);
        EXPECT_EQ(token.line, std::get<1>(expected)) << "For token " << to_string_as_expected_by_evaluation_system(token.type) << ", expected line " << std::get<1>(expected) << ", received " << token.line;
        EXPECT_EQ(token.col, std::get<2>(expected)) << "For token " << to_string_as_expected_by_evaluation_system(token.type) << ", expected col " << std::get<2>(expected) << ", received " << token.col;
    }
}

TEST(ScannerTest, ScansStringLiterals) {
    Scanner scanner("\"hello world\"", false);
    Token token = scanner.next_token();
    EXPECT_EQ(token.type, STRING) << "Expected STRING token, received " << to_string_as_expected_by_evaluation_system(token.type);

    StringTokenView str_token(token);
    EXPECT_EQ(str_token.literal(), "hello world") << "Expected 'hello world', received '" << str_token.literal() << "'";
    EXPECT_EQ(str_token.token.lexeme, "\"hello world\"") << "Expected 'hello world', received '" << str_token.token.lexeme << "'";

    // since the lexeme = "literal", we should only store the lexeme
    EXPECT_EQ(std::get<std::string>(str_token.token.literal), "");
}

TEST(ScannerTest, ScansStringQuotedLiterals) {
    Scanner scanner(R"("hello \"world\"")", false);
    Token token = scanner.next_token();
    EXPECT_EQ(token.type, STRING) << "Expected STRING token, received " << to_string_as_expected_by_evaluation_system(token.type);

    StringTokenView str_token(token);
    EXPECT_EQ(str_token.literal(), "hello \"world\"") << "Expected 'hello \"world\"', received '" << str_token.literal() << "'";
    EXPECT_EQ(str_token.token.lexeme, R"("hello \"world\"")") << R"(Expected '"hello \"world\""', received ')" << str_token.token.lexeme << "'";
}

TEST(ScannerTest, HandlesUnterminatedString) {
    Scanner scanner("\"hello world", false);
    Token token = scanner.next_token();
    EXPECT_EQ(token.type, UNTERMINATED_STRING) << "Expected UNTERMINATED_STRING token, received " << to_string_as_expected_by_evaluation_system(token.type);

    StringTokenView str_token(token);
    EXPECT_EQ(str_token.literal(), "hello world") << "Expected 'hello world', received '" << str_token.literal() << "'";
}

TEST(ScannerTest, HandlesUnterminatedEscapedString) {
    Scanner scanner(R"("hello world\)", false);
    Token token = scanner.next_token();
    EXPECT_EQ(token.type, UNTERMINATED_STRING) << "Expected UNTERMINATED_STRING token, received " << to_string_as_expected_by_evaluation_system(token.type);

    StringTokenView str_token(token);
    EXPECT_EQ(str_token.literal(), "hello world") << "Expected 'hello world', received '" << str_token.literal() << "'";
    EXPECT_EQ(str_token.token.lexeme, R"("hello world\)") << "Expected '\"hello world\\', received '" << str_token.token.lexeme << "'";
}

TEST(ScannerTest, HandlesEscapedStrings) {
    Scanner scanner(R"("\n\t<- that's a line feed and tab and \"this another string\"\t")", false);
    Token token = scanner.next_token();
    EXPECT_EQ(token.type, STRING) << "Expected STRING token, received " << to_string_as_expected_by_evaluation_system(token.type);

    StringTokenView str_token(token);
    EXPECT_EQ(str_token.literal(), "\n\t<- that's a line feed and tab and \"this another string\"\t");
    EXPECT_EQ(str_token.token.lexeme, R"("\n\t<- that's a line feed and tab and \"this another string\"\t")");
}

TEST(ScannerTest, HandlesUnrecognizedEscapedStrings) {
    Scanner scanner(R"("\a<- that's unrecognized\b")", false);
    Token token = scanner.next_token();
    EXPECT_EQ(token.type, STRING) << "Expected STRING token, received " << to_string_as_expected_by_evaluation_system(token.type);

    StringTokenView str_token(token);
    EXPECT_EQ(str_token.literal(), "\\a<- that's unrecognized\\b");
    EXPECT_EQ(str_token.token.lexeme, R"("\a<- that's unrecognized\b")");

    // since the lexeme = "literal", we should only store the lexeme
    EXPECT_EQ(std::get<std::string>(str_token.token.literal), "");
}

TEST(ScannerTest, HandlesMultiLineStrings) {
    std::string multiline = R"("This
is a
multiline
string")";
    Scanner scanner(multiline, false);
    Token token = scanner.next_token();
    EXPECT_EQ(token.type, STRING) << "Expected STRING token, received " << to_string_as_expected_by_evaluation_system(token.type);

    StringTokenView str_token(token);
    EXPECT_EQ(str_token.literal(), multiline.substr(1, multiline.size() - 2) /*strip the quotes*/);
    EXPECT_EQ(str_token.token.lexeme, multiline);

    // since the lexeme = "literal", we should only store the lexeme
    EXPECT_EQ(std::get<std::string>(str_token.token.literal), "");
}

TEST(ScannerTest, ScansIntegerNumbers) {
    Scanner scanner("12345", false);
    Token token = scanner.next_token();
    EXPECT_EQ(token.type, NUMBER) << "Expected NUMBER token, received " << to_string_as_expected_by_evaluation_system(token.type);

    RealNumberTokenView number_token(token);
    const RealNumber& number = number_token.literal();
    EXPECT_EQ(number.integer, 12345) << "Expected integer part 12345, received " << number.integer;
    EXPECT_EQ(number.fractional, 0) << "Expected fractional part 0, received " << number.fractional;
}

TEST(ScannerTest, ScansDecimalNumbers) {
    Scanner scanner("123.45", false);
    Token token = scanner.next_token();
    EXPECT_EQ(token.type, NUMBER) << "Expected NUMBER token, received " << to_string_as_expected_by_evaluation_system(token.type);

    RealNumberTokenView number_token(token);
    const RealNumber& number = number_token.literal();
    EXPECT_EQ(number.integer, 123) << "Expected integer part 123, received " << number.integer;
    EXPECT_EQ(number.fractional, 45) << "Expected fractional part 45, received " << number.fractional;
}

TEST(ScannerTest, ScansIdentifiers) {
    Scanner scanner("my_var an_identifier", false);
    Token token1 = scanner.next_token();
    EXPECT_EQ(token1.type, IDENTIFIER) << "Expected IDENTIFIER token, received " << to_string_as_expected_by_evaluation_system(token1.type);
    EXPECT_EQ(token1.lexeme, "my_var") << "Expected lexeme 'my_var', received '" << token1.lexeme << "'";

    Token token2 = scanner.next_token();
    EXPECT_EQ(token2.type, IDENTIFIER) << "Expected IDENTIFIER token, received " << to_string_as_expected_by_evaluation_system(token2.type);
    EXPECT_EQ(token2.lexeme, "an_identifier") << "Expected lexeme 'an_identifier', received '" << token2.lexeme << "'";
}

TEST(ScannerTest, ScansKeywords) {
    Scanner scanner("and class else false for fun if nil or print return super this true var while", false);
    std::vector<TokenType> expected_types = {
        AND, CLASS, ELSE, FALSE, FOR, FUN, IF, NIL, OR, PRINT,
        RETURN, SUPER, THIS, TRUE, VAR, WHILE, EOF_TOKEN
    };

    for (const auto& expected_type : expected_types) {
        Token token = scanner.next_token();
        EXPECT_EQ(token.type, expected_type) << "Expected " << to_string_as_expected_by_evaluation_system(expected_type) << ", received " << to_string_as_expected_by_evaluation_system(token.type);
    }
}

TEST(ScannerTest, HandlesUnrecognizedCharacter) {
    Scanner scanner("#", false);
    Token token = scanner.next_token();
    EXPECT_EQ(token.type, UNRECOGNIZED) << "Expected UNRECOGNIZED token, received " << to_string_as_expected_by_evaluation_system(token.type);
}

TEST(ScannerTest, next_and_peek_methods_work) {
    Scanner scanner("a b c d", false);
    EXPECT_NE(scanner.peek_previous().lexeme, "a");
    EXPECT_NE(scanner.peek_previous().lexeme, "b");
    EXPECT_NE(scanner.peek_current().lexeme, "b");
    EXPECT_NE(scanner.peek_next().lexeme, "c");

    EXPECT_EQ(scanner.next_token().lexeme, "a") << "Expected 'a'";
    EXPECT_NE(scanner.peek_previous().lexeme, "a");
    EXPECT_EQ(scanner.peek_current().lexeme, "a") << "Expected 'a'";
    EXPECT_EQ(scanner.peek_next().lexeme, "b") << "Expected 'b'";

    EXPECT_EQ(scanner.next_token().lexeme, "b") << "Expected 'b'";
    EXPECT_EQ(scanner.peek_previous().lexeme, "a") << "Expected 'a'";
    EXPECT_EQ(scanner.peek_current().lexeme, "b") << "Expected 'b'";
    EXPECT_EQ(scanner.peek_next().lexeme, "c") << "Expected 'c'";

    EXPECT_EQ(scanner.next_token().lexeme, "c") << "Expected 'c'";
    EXPECT_EQ(scanner.peek_previous().lexeme, "b") << "Expected 'b'";
    EXPECT_EQ(scanner.peek_current().lexeme, "c") << "Expected 'c'";
    EXPECT_EQ(scanner.peek_next().lexeme, "d") << "Expected 'd'";

    EXPECT_EQ(scanner.next_token().lexeme, "d") << "Expected 'd'";
    EXPECT_EQ(scanner.peek_previous().lexeme, "c") << "Expected 'c'";
    EXPECT_EQ(scanner.peek_current().lexeme, "d") << "Expected 'd'";
    EXPECT_EQ(scanner.peek_next().type, EOF_TOKEN) << "Expected EOF";

    EXPECT_EQ(scanner.peek_next().type, EOF_TOKEN) << "Expected EOF";
    EXPECT_EQ(scanner.next_token().type, EOF_TOKEN) << "Expected EOF";
    EXPECT_EQ(scanner.peek_previous().lexeme, "d") << "Expected 'd'";
    EXPECT_EQ(scanner.peek_current().type, EOF_TOKEN) << "Expected EOF";
    EXPECT_EQ(scanner.peek_next().type, EOF_TOKEN) << "Expected EOF";
}

} // namespace lox

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
