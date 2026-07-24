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

TEST(ParserTest, ForStatementFull) {
    std::string result = parse_and_serialize(R"(
for (var i = 0; i < 10; i = i + 1) {
    print i;
}
)");
    // (for
    //   (group
    //      (var (= i 0.0))
    //      (< i 10.0)
    //      (= i (+ i 1.0))
    //   )
    //   ({ (print i))
    // )
    //
    EXPECT_EQ(result, "(for (group (var (= i 0.0)) (< i 10.0) (= i (+ i 1.0))) ({ (print i)))");
}

TEST(ParserTest, ForStatementNoInit) {
    std::string result = parse_and_serialize(R"(
for (; i < 10; i = i + 1) {
    print i;
}
)");
    EXPECT_EQ(result, "(for (group no-op (< i 10.0) (= i (+ i 1.0))) ({ (print i)))");
}

TEST(ParserTest, ForStatementNoIncrement) {
    std::string result = parse_and_serialize(R"(
for (var i = 0; i < 10;) {
    print i;
}
)");
    EXPECT_EQ(result, "(for (group (var (= i 0.0)) (< i 10.0) no-op) ({ (print i)))");
}

TEST(ParserTest, ForStatementAsWhile) {
    std::string result = parse_and_serialize(R"(
for (; i < 10;) {
    print i;
}
)");
    EXPECT_EQ(result, "(for (group no-op (< i 10.0) no-op) ({ (print i)))");
}

TEST(ParserTest, ForStatementWithIncrementOnly) {
    std::string result = parse_and_serialize(R"(
for (; ; i = i + 1) {
    print i;
}
)");
    EXPECT_EQ(result, "(for (group no-op true (= i (+ i 1.0))) ({ (print i)))");
}

TEST(ParserTest, ForStatementInfinite) {
    std::string result = parse_and_serialize(R"(
for (;;) {
    print "loop";
}
)");
    EXPECT_EQ(result, "(for (group no-op true no-op) ({ (print loop)))");
}

TEST(ParserTest, ForEmptyBody) {
    std::string result = parse_and_serialize(R"(
for (; ;) {}
)");
    EXPECT_EQ(result, "(for (group no-op true no-op) ({))");
}

TEST(ParserTest, ForEmptyBody2) {
    std::string result = parse_and_serialize(R"(
var baz = "after";
{
  var baz = "before";

  for (var baz = 0; baz < 1; baz = baz + 1) {
    print baz;
    var baz = -1;
    print baz;
  }
}
)");
    EXPECT_EQ(result, "(var (= baz after))({ (var (= baz before)) (for (group (var (= baz 0.0)) (< baz 1.0) (= baz (+ baz 1.0))) ({ (print baz) (var (= baz (- 1.0))) (print baz))))");
}

TEST(ParserTest, ForStatementSimpleBody) {
    std::string result = parse_and_serialize("for (i = 0; i < 5; i = i + 1) print i;");
    EXPECT_EQ(result, "(for (group (= i 0.0) (< i 5.0) (= i (+ i 1.0))) (print i))");
}

TEST(ParserTest, UserDeclaredFunc) {
    std::string result = parse_and_serialize(R"(
fun sayHi(first, last) {
  print "Hi, " + first + " " + last + "!";
}
)");
    EXPECT_EQ(result, "(fun sayHi (group first last) ({ (print (+ (+ (+ (+ Hi,  first)  ) last) !))))");
}

TEST(ParserTest, UserDeclaredFuncNoArgs) {
    std::string result = parse_and_serialize(R"(
fun count() {
  print 1;
}
)");
    EXPECT_EQ(result, "(fun count (group) ({ (print 1.0)))");
}

TEST(ParserTest, FunctionCall) {
    std::string result = parse_and_serialize("add(1, 2);");
    EXPECT_EQ(result, "(call add 1.0 2.0)");
}

TEST(ParserTest, FunctionCallChain) {
    std::string result = parse_and_serialize("getCallback()(arg1, arg2);");
    EXPECT_EQ(result, "(call getCallback)(call __CHAIN_F__ arg1 arg2)");
}

TEST(ParserTest, FunctionCallChain2) {
    std::string result = parse_and_serialize("f(first)(second)(third);");
    EXPECT_EQ(result, "(call f first)(call __CHAIN_F__ second)(call __CHAIN_F__ third)");
}

TEST(ParserTest, ReturnStatement) {
    std::string result = parse_and_serialize("fun f() { var x = 10; var y = 20; return x, y; }");
    EXPECT_EQ(result, "(fun f (group) ({ (var (= x 10.0)) (var (= y 20.0)) (return x y)))");
}

TEST(ParserTest, ReturnEmpty) {
    std::string result = parse_and_serialize(R"(fun f() {
    if true
        return "if true";
    else if false
        return "if false";
    else
        return "otherwise";
})");
    EXPECT_EQ(result, "(fun f (group) ({ (if true "
                        "(return if true) "
                        "(if false "
                            "(return if false) "
                            "(return otherwise)))))");

}

TEST(ParserTest, FunctionCallNamedArgs) {
    std::string result = parse_and_serialize("f(arg1: true, arg2: variable == 10);");
    EXPECT_EQ(result, "(call f (: arg1 true) (: arg2 (== variable 10.0)))");
}

TEST(ParserTest, FunctionCallNamedArgs2) {
    std::string result = parse_and_serialize("f(arg1: true, arg2: variable = 10);");
    EXPECT_EQ(result, "(call f (: arg1 true) (: arg2 (= variable 10.0)))");
}

TEST(ParserTest, FunctionCallMixedArgs) {
    // Positional followed by named
    std::string result = parse_and_serialize("f(1, 2, verbose: true);");
    EXPECT_EQ(result, "(call f 1.0 2.0 (: verbose true))");
}

TEST(ParserTest, FunctionCallPositionalOnly) {
    std::string result = parse_and_serialize("f(1, \"hello\", x);");
    EXPECT_EQ(result, "(call f 1.0 hello x)");
}

TEST(ParserTest, FunctionCallComplexNamedArgs) {
    std::string result = parse_and_serialize("calculate(base: 100, factor: getMultiplier(2), offset: -5);");
    EXPECT_EQ(result, "(call calculate (: base 100.0) (: factor (call getMultiplier 2.0)) (: offset (- 5.0)))");
}

TEST(ParserTest, FunctionDeclarationAndCallNamed) {
    std::string result = parse_and_serialize(R"(
fun greet(name, greeting) {
  print greeting + ", " + name;
}
greet(greeting: "Hello", name: "User");
)");
    EXPECT_EQ(result, "(fun greet (group name greeting) ({ (print (+ (+ greeting , ) name))))(call greet (: greeting Hello) (: name User))");
}

TEST(ParserTest, HighOrderFunctionPassing) {
    std::string result = parse_and_serialize(R"(
fun apply(fn, value) {
  return fn(value);
}
apply(printValue, 10);
)");
    EXPECT_EQ(result, "(fun apply (group fn value) ({ (return (call fn value))))(call apply printValue 10.0)");
}

TEST(ParserTest, HighOrderFunctionReturning) {
    std::string result = parse_and_serialize(R"(
fun makeAdder(x) {
  fun adder(y) {
    return x + y;
  }
  return adder;
}
var add5 = makeAdder(5);
)");
    EXPECT_EQ(result, "(fun makeAdder (group x) ({ (fun adder (group y) ({ (return (+ x y)))) (return adder)))(var (= add5 (call makeAdder 5.0)))");
}

TEST(ParserTest, AnonymousFunctionAsArgument) {
    std::string result = parse_and_serialize("twice(square, 2);");
    EXPECT_EQ(result, "(call twice square 2.0)");
}

TEST(ParserTest, CurryingStyleCalls) {
    std::string result = parse_and_serialize("multiply(5)(10);");
    EXPECT_EQ(result, "(call multiply 5.0)(call __CHAIN_F__ 10.0)");
}

TEST(ParserTest, FunctionDefaultParameters) {
    std::string result = parse_and_serialize(R"(
fun test(a = 1, b = 2) {
  return a + b;
}
)");
    EXPECT_EQ(result, "(fun test (group (= a 1.0) (= b 2.0)) ({ (return (+ a b))))");
}

TEST(ParserTest, FunctionMixedDefaultParameters) {
    std::string result = parse_and_serialize(R"(
fun test(a, b = 2, c = 3) {
  return a + b;
}
)");
    EXPECT_EQ(result, "(fun test (group a (= b 2.0) (= c 3.0)) ({ (return (+ a b))))");
}
