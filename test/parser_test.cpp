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

TEST(ParserTest, HandlesGrouping2) {
    std::string result = parse_and_serialize("(10 - 4)");
    EXPECT_EQ(result, "(group (- 10.0 4.0))");
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
    EXPECT_EQ(result, "(print (+ hello world))");
}

TEST(ParserTest, PrintExpression) {
    std::string result = parse_and_serialize("print 1 + 2 * 3;");
    EXPECT_EQ(result, "(print (+ 1.0 (* 2.0 3.0)))");
}

TEST(ParserTest, AssignmentExpression) {
    std::string result = parse_and_serialize("a = 10;");
    EXPECT_EQ(result, "(= a 10.0)");
}

TEST(ParserTest, VarAssignmentExpression) {
    std::string result = parse_and_serialize("var a = 15;");
    EXPECT_EQ(result, "(var (= a 15.0))");
}

TEST(ParserTest, VarExpression) {
    std::string result = parse_and_serialize("var a;");
    EXPECT_EQ(result, "(var a)");
}

TEST(ParserTest, VarExpression2) {
    std::string result = parse_and_serialize("var isAdult = age >= 18;");
    EXPECT_EQ(result, "(var (= isAdult (>= age 18.0)))");
}

TEST(ParserTest, BlockStatement) {
    std::string result = parse_and_serialize("{ var a = 10; print a; }");
    EXPECT_EQ(result, "({ (var (= a 10.0)) (print a))");
}

TEST(ParserTest, NestedBlocks) {
    std::string result = parse_and_serialize("{ var a = 1; { var a = 2; print a; } print a; }");
    EXPECT_EQ(result, "({ (var (= a 1.0)) ({ (var (= a 2.0)) (print a)) (print a))");
}

TEST(ParserTest, EmptyBlock) {
    std::string result = parse_and_serialize("{}");
    EXPECT_EQ(result, "({)");
}

TEST(ParserTest, IfStatement) {
    std::string result = parse_and_serialize("if (true) print \"yes\";");
    EXPECT_EQ(result, "(if (group true) (print yes))");
}

TEST(ParserTest, IfElseStatement) {
    std::string result = parse_and_serialize(R"(
if (x > 0)
    print "pos";
else
    print "neg";
)");
    EXPECT_EQ(result, "(if (group (> x 0.0)) (print pos) (print neg))");
}

TEST(ParserTest, NestedIfStatement) {
    std::string result = parse_and_serialize(R"(
if (a)
    if (b)
        print "both";
    else
        print "a only";
)");
    EXPECT_EQ(result, "(if (group a) (if (group b) (print both) (print a only)))");
}

TEST(ParserTest, NestedIf2) {
    auto result = parse_and_serialize(R"(
if (true)
    if (false)
        x = 1;
    else
        x = 2;
)");
    EXPECT_EQ(result, "(if (group true) (if (group false) (= x 1.0) (= x 2.0)))");
}

TEST(ParserTest, IfElseIfStatement) {
    std::string result = parse_and_serialize(R"(
if (a)
    print "a";
else if (b)
    print "b";
else
    print "c";
)");
    EXPECT_EQ(result, "(if (group a) (print a) (if (group b) (print b) (print c)))");
}

TEST(ParserTest, MultipleElseIfStatements) {
    std::string result = parse_and_serialize(R"(
if (x == 1)
    print 1;
else if (x == 2)
    print 2;
else if (x == 3)
    print 3;
)");
    EXPECT_EQ(result, "(if (group (== x 1.0)) (print 1.0) (if (group (== x 2.0)) (print 2.0) (if (group (== x 3.0)) (print 3.0))))");
}

TEST(ParserTest, MultipleNestedElseIfStatements) {
    std::string result = parse_and_serialize(R"(
if x == 1 {
    if age < 13 {
        stage = "child";
    } else if age < 16 {
        stage = "young teenager";
    } else
        stage = "teenager";

    whatever = "hello";
} else if x == 2
    print 2;
else if x == 3
    print 3;
)");
    EXPECT_EQ(result, "(if (== x 1.0) ({ (if (< age 13.0) ({ (= stage child)) (if (< age 16.0) ({ (= stage young teenager)) (= stage teenager))) (= whatever hello)) (if (== x 2.0) (print 2.0) (if (== x 3.0) (print 3.0))))");
}

TEST(ParserTest, MultipleNestedElseIfStatements2) {
    std::string result = parse_and_serialize(R"(
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
)");
    EXPECT_EQ(result, "(var (= stage unknown))(var (= age 29.0))(if (group (< age 18.0)) ({ (if (group (< age 13.0)) (= stage child) (if (group (< age 16.0)) (= stage young teenager) (= stage teenager)))) (if (group (< age 65.0)) ({ (if (group (< age 30.0)) (= stage young adult) (if (group (< age 50.0)) (= stage adult) (= stage middle-aged adult)))) (= stage senior)))");
}

TEST(ParserTest, OrKeyword) {
    std::string result = parse_and_serialize(R"(
var x = 1 < 2 or 1 == 2 or (20 * 30 < 30) or true
)");
    EXPECT_EQ(result, "(var (= x (or (or (or (< 1.0 2.0) (== 1.0 2.0)) (group (< (* 20.0 30.0) 30.0))) true)))");
}

TEST(ParserTest, OrAndKeyword) {
    std::string result = parse_and_serialize(R"(
var x = 1 < 2 or 1 == 2 and 20 * 30 < 30 or true and false
)");
    EXPECT_EQ(result, "(var (= x (or (or (< 1.0 2.0) (and (== 1.0 2.0) (< (* 20.0 30.0) 30.0))) (and true false))))");
}

TEST(ParserTest, While) {
    std::string result = parse_and_serialize(R"(
while (true) {
    x = x + 1;
}
)");
    EXPECT_EQ(result, "(while (group true) ({ (= x (+ x 1.0))))");

    result = parse_and_serialize(R"(
while x == 10 and y < 10 {
    x = x + 1;
}
)");
    EXPECT_EQ(result, "(while (and (== x 10.0) (< y 10.0)) ({ (= x (+ x 1.0))))");

    result = parse_and_serialize(R"(
while x == 10 and y < 10
    x = x + 1;
)");
    EXPECT_EQ(result, "(while (and (== x 10.0) (< y 10.0)) (= x (+ x 1.0)))");
}