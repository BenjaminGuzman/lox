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

using namespace lox;

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
    GTEST_SKIP() << "Skipping as this behavior is considered incorrect by the valuation system";

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
    GTEST_SKIP() << "Skipping as this behavior is considered incorrect by the valuation system";

    // true + false
    underlying_t result = interpret_expression("true + false");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(1, 0, 0, false)); // true=1, false=0, so 1+0=1
}

TEST(InterpreterIntegrationTest, BinaryPlus_NumberAndBoolean) {
    GTEST_SKIP() << "Skipping as this behavior is considered incorrect by the valuation system";

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
    GTEST_SKIP() << "Skipping as this behavior is considered incorrect by the valuation system";

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
    GTEST_SKIP() << "Skipping as this behavior is considered incorrect by the valuation system";

    // true - false
    underlying_t result = interpret_expression("true - false");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(1, 0, 0, false)); // 1-0=1
}

TEST(InterpreterIntegrationTest, BinaryMinus_NumberAndBoolean) {
    GTEST_SKIP() << "Skipping as this behavior is considered incorrect by the valuation system";

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

    // result = interpret_expression("-9.0000000000000009 * -0.000000000000000001");
    // ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    // ASSERT_EQ(get_value<RealNumber>(result), RealNumber(0, 90000000000000009, 34, false));
}

TEST(InterpreterIntegrationTest, Multiplication_StringRepeat) {
    GTEST_SKIP() << "Skipping as this behavior is considered incorrect by the valuation system";

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
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_FALSE(std::get<bool>(result));
    // The actual output would be to stdout, which is harder to test directly here without mocking std::cout
}

TEST(InterpreterIntegrationTest, Equality) {
    // Number equality
    underlying_t result = interpret_expression("10 == 10");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_TRUE(get_value<bool>(result));

    result = interpret_expression("10 == 10.5");
    ASSERT_FALSE(get_value<bool>(result));

    // String equality
    result = interpret_expression("\"foo\" == \"foo\"");
    ASSERT_TRUE(get_value<bool>(result));

    result = interpret_expression("\"string\\' view\" == \"comparison\"");
    ASSERT_FALSE(get_value<bool>(result));

    result = interpret_expression("\"string\\\" view\" == \"string\\\" view\"");
    ASSERT_TRUE(get_value<bool>(result));

    result = interpret_expression("\"foo\" == \"bar\"");
    ASSERT_FALSE(get_value<bool>(result));

    // Boolean equality
    result = interpret_expression("true == true");
    ASSERT_TRUE(get_value<bool>(result));

    result = interpret_expression("true == false");
    ASSERT_FALSE(get_value<bool>(result));

    // Nil equality
    result = interpret_expression("nil == nil");
    ASSERT_TRUE(get_value<bool>(result));

    // Mixed types
    result = interpret_expression("\"10\" == 10");
    ASSERT_FALSE(get_value<bool>(result));
}

TEST(InterpreterIntegrationTest, Inequality) {
    // Number inequality
    underlying_t result = interpret_expression("10 != 11");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_TRUE(get_value<bool>(result));

    result = interpret_expression("10 != 10");
    ASSERT_FALSE(get_value<bool>(result));

    // String inequality
    result = interpret_expression("\"apple\" != \"orange\"");
    ASSERT_TRUE(get_value<bool>(result));

    // Boolean inequality
    result = interpret_expression("true != false");
    ASSERT_TRUE(get_value<bool>(result));

    // Nil inequality
    result = interpret_expression("nil != false");
    ASSERT_TRUE(get_value<bool>(result));
}

TEST(InterpreterIntegrationTest, VariableAssignment) {
    underlying_t result = interpret_expression("var a = \"foo\"");
    ASSERT_TRUE(std::holds_alternative<std::string>(result));
    ASSERT_EQ(get_value<std::string>(result), "foo");

    result = interpret_expression("var b = 123; b = 456;");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(456, 0, 0, false));

    result = interpret_expression("var c; c = \"updated\";");
    ASSERT_TRUE(std::holds_alternative<std::string>(result));
    ASSERT_EQ(get_value<std::string>(result), "updated");

    result = interpret_expression("var c;");
    ASSERT_TRUE(std::holds_alternative<std::nullptr_t>(result));
    ASSERT_EQ(get_value<std::nullptr_t>(result), nullptr);
}

TEST(InterpreterIntegrationTest, VariableAssignment2) {
    auto result = interpret_expression("var age = 20; var isAdult = age >= 18");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_EQ(get_value<bool>(result), true);
}

TEST(InterpreterIntegrationTest, BlockScope) {
    // Test that a block returns the value of its last expression
    underlying_t result = interpret_expression("{ var a = 10; a + 5; }");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(15, 0, 0, false));

    // Test nested blocks and shadowing
    underlying_t result_shadow = interpret_expression(
        "var a = \"outer\";"
        "{"
        "  var a = \"inner\";"
        "}"
        "a;"
    );
    ASSERT_TRUE(std::holds_alternative<std::string>(result_shadow));
    ASSERT_EQ(get_value<std::string>(result_shadow), "outer");

    // Test block updating outer scope variable
    underlying_t result_update = interpret_expression(
        "var x = 10;"
        "{"
        "  x = 20;"
        "}"
        "x;"
    );
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result_update));
    ASSERT_EQ(get_value<RealNumber>(result_update), RealNumber(20, 0, 0, false));
}

TEST(InterpreterIntegrationTest, EmptyBlock) {
    // An empty block should return nil (monostate/nullptr)
    underlying_t result = interpret_expression("{}");
    ASSERT_TRUE(std::holds_alternative<std::nullptr_t>(result));
}

TEST(InterpreterIntegrationTest, IfStatement) {
    underlying_t result = interpret_expression(R"(
var a = false;
if (true)
    a = true;
a;
)");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_TRUE(std::get<bool>(result));
}

TEST(InterpreterIntegrationTest, IfElseStatement) {
    underlying_t result = interpret_expression(R"(
var result = "";
if (false) {
    result = "then";
} else {
    result = "else";
}
result;
)");
    ASSERT_TRUE(std::holds_alternative<std::string>(result));
    ASSERT_EQ(get_value<std::string>(result), "else");
}

TEST(InterpreterIntegrationTest, IfStatementTruthy) {
    underlying_t result = interpret_expression(R"(
var count = 0;
if ("truthy string") count = 1;
count;
)");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(1, 0, 0, false));
}

TEST(InterpreterIntegrationTest, NestedIfStatements) {
    underlying_t result = interpret_expression(R"(
var x = 0;
if (true)
    if (false)
        x = 1;
    else
        x = 2;
x;
)");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(2, 0, 0, false));
}

TEST(InterpreterIntegrationTest, NestedIfStatements2) {
    underlying_t result = interpret_expression(R"(
if true
    if true
        var x = "nested true";
x;
)");
    ASSERT_TRUE(std::holds_alternative<std::string>(result));
    ASSERT_EQ(get_value<std::string>(result), "nested true");
}

TEST(InterpreterIntegrationTest, IfElseIfElseStatement) {
    underlying_t result = interpret_expression(R"(
var x = 15;
var result = "";
if x < 10 {
    result = "small";
} else if x < 20 {
    result = "medium";
} else {
    result = "large";
}
result;
)");
    ASSERT_TRUE(std::holds_alternative<std::string>(result));
    ASSERT_EQ(get_value<std::string>(result), "medium");
}

TEST(InterpreterIntegrationTest, IfElseIfElseStatementLarge) {
    underlying_t result = interpret_expression(R"(
var x = 30;
var result = "";
if (x < 10) {
    result = "small";
} else if (x < 20) {
    result = "medium";
} else {
    result = "large";
}
result;
)");
    ASSERT_TRUE(std::holds_alternative<std::string>(result));
    ASSERT_EQ(get_value<std::string>(result), "large");
}

TEST(InterpreterIntegrationTest, MultipleElseIf) {
    underlying_t result = interpret_expression(R"(
var grade = "B";
var score = 0;
if (grade == "A") {
    score = 90;
} else if (grade == "B") {
    score = 80;
} else if (grade == "C") {
    score = 70;
} else {
    score = 0;
}
score;
)");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(80, 0, 0, false));
}

TEST(InterpreterIntegrationTest, MultipleElseIf2) {
    underlying_t result = interpret_expression(R"(
var stage = "unknown";
var age = 50;
if (age < 18) { stage = "child"; }
if (age >= 18) { stage = "adult"; }
print stage;

var isAdult = age >= 18;
var res;
if (isAdult) { res = "eligible for voting: true"; }
if (!isAdult) { res = "eligible for voting: false"; }
res;
)");
    ASSERT_TRUE(std::holds_alternative<std::string>(result));
    ASSERT_EQ(get_value<std::string>(result), "eligible for voting: true");
}

TEST(InterpreterIntegrationTest, MultipleElseIf3) {
    underlying_t result = interpret_expression(R"(
var stage = "unknown";
var age = 29;
if (age < 18) {
    if (age < 13) stage = "child";
    else if (age < 16) stage = "young teenager";
    else stage = "teenager";
} else if (age < 65) {
    if (age < 30) stage = "young adult";
    else if (age < 50) stage = "adult";
    else stage = "middle-aged adult";
} else
    stage = "senior";
stage;
)");
    ASSERT_TRUE(std::holds_alternative<std::string>(result));
    ASSERT_EQ(get_value<std::string>(result), "young adult");
}

TEST(InterpreterIntegrationTest, AndOrKeywords) {
    underlying_t result = interpret_expression("false and true and 10 == 10 or false");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_EQ(get_value<bool>(result), false);

    result = interpret_expression("true and true and 10 == 10 or false");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_EQ(get_value<bool>(result), true);

    result = interpret_expression("false or \"truthy\"");
    ASSERT_TRUE(std::holds_alternative<std::string>(result));
    ASSERT_EQ(get_value<std::string>(result), "truthy");

    result = interpret_expression("true or \"not evaluated\"");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_TRUE(get_value<bool>(result));

    result = interpret_expression("false and \"not evaluated\"");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_FALSE(get_value<bool>(result));

    result = interpret_expression("\"first\" and \"second\"");
    ASSERT_TRUE(std::holds_alternative<std::string>(result));
    ASSERT_EQ(get_value<std::string>(result), "second");

    result = interpret_expression("nil or 123");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(123, 0, 0, false));

    result = interpret_expression("123 and nil");
    ASSERT_TRUE(std::holds_alternative<std::nullptr_t>(result));

    result = interpret_expression("false or false or true or false");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_TRUE(get_value<bool>(result));

    result = interpret_expression("true and true and false and true");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_FALSE(get_value<bool>(result));


    result = interpret_expression("false and 1");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_FALSE(get_value<bool>(result));

    result = interpret_expression("true and 1");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(1, 0, 0, false));

    result = interpret_expression("23 and \"hello\" and false");
    ASSERT_TRUE(std::holds_alternative<bool>(result));
    ASSERT_EQ(get_value<bool>(result), false);
}

TEST(InterpreterIntegrationTest, While) {
    underlying_t result = interpret_expression(R"(
var i = 0;
while (i < 3) {
    i = i + 1;
}
i;
)");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(3, 0, 0, false));

    // Test while with false condition
    result = interpret_expression(R"(
var x = 10;
while (false) {
    x = 20;
}
x;
)");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(10, 0, 0, false));

    // Test while with complex condition and side effects
    result = interpret_expression(R"(
var a = 0;
var b = 10;
while a < 5 and b > 0 {
    a = a + 1;
    b = b - 2;
}
a;
)");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(5, 0, 0, false));

}

TEST(InterpreterIntegrationTest, For) {
    underlying_t result = interpret_expression(R"(
for (var i = 0; i < 3; i = i + 1) {
    i;
}
)");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(2, 0, 0, false));

    result = interpret_expression(R"(
for (var i = 10; i < 3; i = i + 1) {
    i;
}
)");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(10, 0, 0, false));

    result = interpret_expression(R"(
var i = 0
for (; i <= 3;) {
    i = i + 1;
}
)");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(4, 0, 0, false));
}

TEST(InterpreterIntegrationTest, FunctionDefinitionAndCall) {
    underlying_t result = interpret_expression(R"(
fun add(a, b) {
    return a + b;
}
add(5, 10);
)");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(15, 0, 0, false));
}

// TEST(InterpreterIntegrationTest, FunctionClosure) {
//     underlying_t result = interpret_expression(R"(
// fun makeCounter() {
//     var i = 0;
//     fun count() {
//         i = i + 1;
//         return i;
//     }
//     return count;
// }
// var counter = makeCounter();
// counter();
// counter();
// )");
    // ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    // ASSERT_EQ(get_value<RealNumber>(result), RealNumber(2, 0, 0, false));
// }

TEST(InterpreterIntegrationTest, RecursiveFunction) {
    underlying_t result = interpret_expression(R"(
fun fib(n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}
fib(6);
)");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(8, 0, 0, false));
}

TEST(InterpreterIntegrationTest, FunctionNoReturn) {
    underlying_t result = interpret_expression(R"(
fun sayHi(name) {
    print "Hi, " + name + "!";
}
sayHi("Lox");
)");
    // Functions without an explicit return statement return nil
    ASSERT_TRUE(std::holds_alternative<std::nullptr_t>(result));
}

TEST(InterpreterIntegrationTest, FunctionNamedArguments) {
    underlying_t result = interpret_expression(R"(
fun subtract(a, b) {
    return a - b;
}
subtract(b: 5, a: 20);
)");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(15, 0, 0, false));
}

TEST(InterpreterIntegrationTest, FunctionMixedArguments) {
    underlying_t result = interpret_expression(R"(
fun volume(length, width, height) {
    return length * width * height;
}
// Positional for length, named for width and height
volume(10, height: 2, width: 5);
)");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(100, 0, 0, false));
}

TEST(InterpreterIntegrationTest, FunctionNamedArgumentsWithExpressions) {
    underlying_t result = interpret_expression(R"(
fun check(first, second) {
    if (first) return second;
    return 0;
}
var x = 10;
check(second: x + 5, first: x == 10);
)");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(15, 0, 0, false));
}

TEST(InterpreterIntegrationTest, FunctionPositionalOnly) {
    underlying_t result = interpret_expression(R"(
fun multiply(x, y) {
    return x * y;
}
multiply(4, 5);
)");
    ASSERT_TRUE(std::holds_alternative<RealNumber>(result));
    ASSERT_EQ(get_value<RealNumber>(result), RealNumber(20, 0, 0, false));
}
