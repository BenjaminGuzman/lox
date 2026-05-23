#include <gtest/gtest.h>
#include "parser.h"
#include "serializer.h"
#include <sstream>

// Helper to parse a string and capture the serialized AST output
std::string parse_and_serialize(const std::string& source) {
    lox::Scanner scanner(source, false);
    lox::AST ast(scanner, false);
    int n_errors = ast.build();

    // Redirect cout to a stringstream to capture output
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

    Serializer::serialize(ast);

    std::cout.rdbuf(old); // Restore cout
    std::string output = buffer.str();
    // Trim trailing newline
    if (!output.empty() && output.back() == '\n') {
        output.pop_back();
    }
    return output;
}

TEST(ParserTest, Parenthesis) {
    std::string result = parse_and_serialize("(hello + (\"world\" * 10))");
    EXPECT_EQ(result, "(group (+ hello (group (* world 10.0))))");
}

TEST(ParserTest, ArithmeticNegation) {
    std::string result = parse_and_serialize("- 10");
    EXPECT_EQ(result, "(- 10.0)");
}

TEST(ParserTest, LogicNegation) {
    std::string result = parse_and_serialize("! true");
    EXPECT_EQ(result, "(! true)");
}

TEST(ParserTest, HandlesPrecedence) {
    // 10 - 4 * 2 -> (- 10 (* 4 2))
    std::string result = parse_and_serialize("10 - 4 * 2");
    EXPECT_EQ(result, "(- 10.0 (* 4.0 2.0))");
}

TEST(ParserTest, HandlesGrouping) {
    // (10 - 4) * 2 -> (* (group (- 10 4)) 2)
    std::string result = parse_and_serialize("(10 - 4) * 2");
    EXPECT_EQ(result, "(* (group (- 10.0 4.0)) 2.0)");
}

TEST(ParserTest, HandlesUnary) {
    // -4 * 10 -> (* (- 4) 10)
    std::string result = parse_and_serialize("-4 * 10");
    EXPECT_EQ(result, "(* (- 4.0) 10.0)");
}

TEST(ParserTest, HandlesComparison) {
    // 10 == 10 < 90 -> (< (== 10.0 10.0) 90.0)
    std::string result = parse_and_serialize("10 == 10 < 90");
    EXPECT_EQ(result, "(< (== 10.0 10.0) 90.0)");
}

TEST(ParserTest, PrintStatement) {
    std::string result = parse_and_serialize("print \"hello\" + \"world\";");
    EXPECT_EQ(result, "print(+ hello world)");
}

TEST(ParserTest, PrintExpression) {
    std::string result = parse_and_serialize("print 1 + 2 * 3;");
    EXPECT_EQ(result, "print(+ 1.0 (* 2.0 3.0))");
}
