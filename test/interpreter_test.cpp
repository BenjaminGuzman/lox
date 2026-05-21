#include "gtest/gtest.h"
#include "interpreter.h"
#include "scanner.h" // For Scanner
#include "parser.h"   // For AST (which is the parser)
#include "scanner/real_number.h" // For RealNumber

#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <iostream> // For debugging output if needed
#include <cmath>    // For std::abs

// Helper to extract value from underlying_t variant
template<typename T>
T get_value(const underlying_t& var) {
    return std::get<T>(var);
}

// Integration test helper: scans, parses, and interprets an expression string
underlying_t interpret_expression(const std::string& expression) {
    lox::Scanner scanner(expression, false);
    // The AST class takes a Scanner reference and builds the AST upon construction (autobuild=true by default).
    lox::AST ast(scanner);

    // The Interpreter::execute method takes a const reference to a unique_ptr<ASTNode>.
    // So we pass ast.root directly.
    lox::Interpreter interpreter;
    return interpreter.execute(ast.root);
}

TEST(InterpreterIntegrationTest, NumberLiteral) {
    underlying_t result = interpret_expression("123");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(123, 0, 0, false));
}

TEST(InterpreterIntegrationTest, StringLiteral) {
    underlying_t result = interpret_expression("\"hello\"");
    ASSERT_TRUE(std::holds_alternative<std::string>(result));
    ASSERT_EQ(get_value<std::string>(result), "hello");
}

TEST(InterpreterIntegrationTest, BooleanLiteralTrue) {
    underlying_t result = interpret_expression("true");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_TRUE(get_value<bool>(result));
}

TEST(InterpreterIntegrationTest, BooleanLiteralFalse) {
    underlying_t result = interpret_expression("false");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_FALSE(get_value<bool>(result));
}

TEST(InterpreterIntegrationTest, NilLiteral) {
    underlying_t result = interpret_expression("nil");
    ASSERT_TRUE(std::holds_alternative<nullptr_t>(result));
}

TEST(InterpreterIntegrationTest, UnaryNot) {
    // !true
    underlying_t result_true = interpret_expression("!true");
    ASSERT_TRUE(std::holds_alternative<bool>(result_true));
    ASSERT_FALSE(get_value<bool>(result_true));

    // !false
    underlying_t result_false = interpret_expression("!false");
    ASSERT_TRUE(std::holds_alternative<bool>(result_false));
    ASSERT_TRUE(get_value<bool>(result_false));

    // !nil
    underlying_t result_nil = interpret_expression("!nil");
    ASSERT_TRUE(std::holds_alternative<bool>(result_nil));
    ASSERT_TRUE(get_value<bool>(result_nil));
}

TEST(InterpreterIntegrationTest, UnaryMinus) {
    // -10
    underlying_t result_neg10 = interpret_expression("-10");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result_neg10));
    ASSERT_EQ(get_value<RealNumber>(result_neg10), RealNumber(10, 0, 0, true)); // Expect -10

    // -(-5.5)
    underlying_t result_double_neg = interpret_expression("-(-5.5)");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result_double_neg));
    ASSERT_EQ(get_value<RealNumber>(result_double_neg), RealNumber(5, 5, 1, false)); // Expect 5.5
}

TEST(InterpreterIntegrationTest, BinaryPlus_Numbers) {
    // 5 + 3
    underlying_t result1 = interpret_expression("5 + 3");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result1));
    ASSERT_EQ(get_value<RealNumber>(result1), RealNumber(8, 0, 0, false));

    // 1.5 + 2.5
    underlying_t result2 = interpret_expression("1.5 + 2.5");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result2));
    ASSERT_EQ(get_value<RealNumber>(result2), RealNumber(4, 0, 0, false));
}

TEST(InterpreterIntegrationTest, BinaryPlus_Strings) {
    // "hello" + "world"
    underlying_t result = interpret_expression("\"hello\" + \"world\"");
    ASSERT_TRUE(std::holds_alternative<std::string>(result));
    ASSERT_EQ(get_value<std::string>(result), "helloworld");
}

TEST(InterpreterIntegrationTest, BinaryPlus_NumberAndString) {
    // 123 + "test"
    underlying_t result1 = interpret_expression("123 + \"test\"");
    ASSERT_TRUE(std::holds_alternative<std::string>(result1));
    ASSERT_EQ(get_value<std::string>(result1), "123test");

    // "test" + 123.5
    underlying_t result2 = interpret_expression("\"test\" + 123.5");
    ASSERT_TRUE(std::holds_alternative<std::string>(result2));
    ASSERT_EQ(get_value<std::string>(result2), "test123.5");
}

TEST(InterpreterIntegrationTest, BinaryPlus_Booleans) {
    // true + false
    underlying_t result = interpret_expression("true + false");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(1, 0, 0, false)); // true=1, false=0, so 1+0=1
}

TEST(InterpreterIntegrationTest, BinaryPlus_NumberAndBoolean) {
    // 5 + true
    underlying_t result1 = interpret_expression("5 + true");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result1));
    ASSERT_EQ(get_value<RealNumber>(result1), RealNumber(6, 0, 0, false)); // 5+1=6

    // false + 10
    underlying_t result2 = interpret_expression("false + 10");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result2));
    ASSERT_EQ(get_value<RealNumber>(result2), RealNumber(10, 0, 0, false)); // 0+10=10
}

TEST(InterpreterIntegrationTest, BinaryPlus_StringAndNil) {
    // "prefix" + nil
    underlying_t result1 = interpret_expression("\"prefix\" + nil");
    ASSERT_TRUE(std::holds_alternative<std::string>(result1));
    ASSERT_EQ(get_value<std::string>(result1), "prefixnil");

    // nil + "suffix"
    underlying_t result2 = interpret_expression("nil + \"suffix\"");
    ASSERT_TRUE(std::holds_alternative<std::string>(result2));
    ASSERT_EQ(get_value<std::string>(result2), "nilsuffix");
}

TEST(InterpreterIntegrationTest, BinaryMinus_Numbers) {
    // 10 - 3
    underlying_t result1 = interpret_expression("10 - 3");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result1));
    ASSERT_EQ(get_value<RealNumber>(result1), RealNumber(7, 0, 0, false));

    // 5.5 - 2.0
    underlying_t result2 = interpret_expression("5.5 - 2.0");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result2));
    ASSERT_EQ(get_value<RealNumber>(result2), RealNumber(3, 5, 1, false));
}

TEST(InterpreterIntegrationTest, BinaryMinus_Booleans) {
    // true - false
    underlying_t result = interpret_expression("true - false");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(1, 0, 0, false)); // 1-0=1
}

TEST(InterpreterIntegrationTest, BinaryMinus_NumberAndBoolean) {
    // 5 - true
    underlying_t result1 = interpret_expression("5 - true");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result1));
    ASSERT_EQ(get_value<RealNumber>(result1), RealNumber(4, 0, 0, false)); // 5-1=4

    // false - 10
    underlying_t result2 = interpret_expression("false - 10");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result2));
    ASSERT_EQ(get_value<RealNumber>(result2), RealNumber(10, 0, 0, true)); // 0-10=-10
}

TEST(InterpreterIntegrationTest, Multiplication_Numbers) {
    underlying_t result = interpret_expression("1 * 2");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(2, 0, 0, false));

    result = interpret_expression("-9.99 * 0.99");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(9, 8901, 4, true));

    result = interpret_expression("-9.0000000000000009 * -0.000000000000000001");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(0, 90000000000000009, 34, false));
}

TEST(InterpreterIntegrationTest, Multiplication_StringRepeat) {
    underlying_t result = interpret_expression("\"hola\" * 0");
    ASSERT_TRUE(std::holds_alternative<std::string>(result));
    ASSERT_EQ(get_value<std::string>(result), "");

    result = interpret_expression("\"hola\" * 3");
    ASSERT_TRUE(std::holds_alternative<std::string>(result));
    ASSERT_EQ(get_value<std::string>(result), "holaholahola");

    result = interpret_expression("\"hola\" * -4.5"); // this expression should not be valid
    ASSERT_TRUE(std::holds_alternative<std::monostate>(result));

    result = interpret_expression("\"hola\" * 0.5"); // this expression should not be valid
    ASSERT_TRUE(std::holds_alternative<std::monostate>(result));
}

TEST(InterpreterIntegrationTest, Division_Numbers) {
    underlying_t result = interpret_expression("1 / 2");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(0, 5, 1, false));

    result = interpret_expression("51 / 5");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(10, 2, 1, false));

    result = interpret_expression("31 / 5");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(6, 2, 1, false));

    result = interpret_expression("14 / 5");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(2, 8, 1, false));

    result = interpret_expression("-9.99 / 9.99");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(1, 0, 0, true));
}

TEST(InterpreterIntegrationTest, LessThan) {
    underlying_t result = interpret_expression("1 < 2");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_EQ(get_value<bool>(result), true);

    result = interpret_expression("1 < 1");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_EQ(get_value<bool>(result), false);
}

TEST(InterpreterIntegrationTest, LessThanOrEqual) {
    underlying_t result = interpret_expression("1 <= 2");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_EQ(get_value<bool>(result), true);

    result = interpret_expression("1 <= 1");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_EQ(get_value<bool>(result), true);

    result = interpret_expression("2 <= 1");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_EQ(get_value<bool>(result), false);
}

TEST(InterpreterIntegrationTest, GreaterThan) {
    underlying_t result = interpret_expression("2 > 1");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_EQ(get_value<bool>(result), true);

    result = interpret_expression("1 > 1");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_EQ(get_value<bool>(result), false);

    result = interpret_expression("1 > 2");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_EQ(get_value<bool>(result), false);
}

TEST(InterpreterIntegrationTest, GreaterThanOrEqual) {
    underlying_t result = interpret_expression("2 >= 1");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_EQ(get_value<bool>(result), true);

    result = interpret_expression("1 >= 1");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_EQ(get_value<bool>(result), true);

    result = interpret_expression("1 >= 2");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_EQ(get_value<bool>(result), false);
}


TEST(InterpreterIntegrationTest, ParenthesizedExpression) {
    // (1 + 2)
    underlying_t result = interpret_expression("(1 + 2)");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(3, 0, 0, false));
}

TEST(InterpreterIntegrationTest, AST_ROOT_MultipleStatements) {
    // Simulate:
    // 1 + 2;
    // "hello";
    // !true;
    // The interpreter's AST_ROOT execution prints results and returns std::monostate.
    underlying_t result = interpret_expression("1 + 2; \"hello\"; !true;");
    ASSERT_TRUE(std::holds_alternative<std::monostate>(result));
    // The actual output would be to stdout, which is harder to test directly here without mocking std::cout
}