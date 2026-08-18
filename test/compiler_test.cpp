#include <gtest/gtest.h>
#include "compiler.h"
#include "parser.h"
#include <llvm/IR/Constants.h>
#include <cmath>

double compile_and_get_constant(const std::string& source) {
    lox::Scanner scanner(source, false);
    lox::AST ast(scanner, false);
    auto _ = ast.build();

    lox::Compiler compiler;
    (void)compiler.compile(ast.root);
    
    llvm::GenericValue result = compiler.runJIT();
    return result.DoubleVal;
}

TEST(CompilerTest, ConstantFoldingArithmetic) {
    EXPECT_NEAR(compile_and_get_constant("-5.5 + 3 * 2.1;"), 0.8, 1e-9);
    EXPECT_NEAR(compile_and_get_constant("10 / 2 - 3;"), 2.0, 1e-9);
    EXPECT_NEAR(compile_and_get_constant("-(10 + 5) * 2;"), -30.0, 1e-9);
    EXPECT_NEAR(compile_and_get_constant("1 + 2 + 3 + 4 + 5;"), 15.0, 1e-9);
    EXPECT_NEAR(compile_and_get_constant("100 / 10 / 2;"), 5.0, 1e-9);
    EXPECT_NEAR(compile_and_get_constant("3 * -2;"), -6.0, 1e-9);
}

std::optional<bool> compile_and_get_bool_constant(const std::string& source) {
    // since our compileASTRoot casts bool (i1) to double, the JIT will return a double
    // 1.0 means true, 0.0 means false

    double res = compile_and_get_constant(source);
    if (std::isnan(res)) return std::nullopt;
    return res != 0.0;
}

TEST(CompilerTest, ConstantFoldingBooleans) {
    EXPECT_EQ(compile_and_get_bool_constant("true;").value_or(false), true);
    EXPECT_EQ(compile_and_get_bool_constant("false;").value_or(true), false);
}

TEST(CompilerTest, NegationForBooleans) {
    EXPECT_EQ(compile_and_get_bool_constant("!0;").value_or(false), true);
    EXPECT_EQ(compile_and_get_bool_constant("!1;").value_or(true), false);
    EXPECT_EQ(compile_and_get_bool_constant("!true;").value_or(true), false);
    EXPECT_EQ(compile_and_get_bool_constant("!false;").value_or(false), true);
}

TEST(CompilerTest, ConstantFoldingComparisons) {
    EXPECT_EQ(compile_and_get_bool_constant("10 > 5;").value_or(false), true);
    EXPECT_EQ(compile_and_get_bool_constant("10 < 5;").value_or(true), false);
    EXPECT_EQ(compile_and_get_bool_constant("5 >= 5;").value_or(false), true);
    EXPECT_EQ(compile_and_get_bool_constant("5 <= 4;").value_or(true), false);
    EXPECT_EQ(compile_and_get_bool_constant("5 == 5;").value_or(false), true);
    EXPECT_EQ(compile_and_get_bool_constant("5 != 5;").value_or(true), false);
    EXPECT_EQ(compile_and_get_bool_constant("10 != 5;").value_or(false), true);
}

TEST(CompilerTest, ConstantFoldingMixedLogic) {
    EXPECT_EQ(compile_and_get_bool_constant("!(10 > 5);").value_or(true), false);
    EXPECT_EQ(compile_and_get_bool_constant("(10 > 5) == true;").value_or(false), true);
    EXPECT_EQ(compile_and_get_bool_constant("(10 > 5) == false;").value_or(true), false);
    EXPECT_EQ(compile_and_get_bool_constant("(5 == 5) != false;").value_or(false), true);
}

TEST(CompilerTest, VariableDeclarationAndUsage) {
    EXPECT_NEAR(compile_and_get_constant("var a = 10; a + 5;"), 15.0, 1e-9);
    EXPECT_NEAR(compile_and_get_constant("var a = 5; var b = 3; a * b;"), 15.0, 1e-9);
    EXPECT_NEAR(compile_and_get_constant("var a = 1; a = a + 2; a = a * 3; a;"), 9.0, 1e-9);
    EXPECT_EQ(compile_and_get_bool_constant("var a = true; a = !a; a;").value_or(true), false);
    EXPECT_EQ(compile_and_get_bool_constant("var a = 5; var b = 10; a < b;").value_or(false), true);
}
TEST(CompilerTest, VariableComplexMath) {
    EXPECT_NEAR(compile_and_get_constant("var a = 5; var b = a + 5; b;"), 10.0, 1e-9);
    EXPECT_NEAR(compile_and_get_constant("var a = 2; a = a * a; a = a * a; a;"), 16.0, 1e-9);
    EXPECT_NEAR(compile_and_get_constant("var a = 1; var b = 2; var c = 3; a + b * c;"), 7.0, 1e-9);
    EXPECT_NEAR(compile_and_get_constant("var a = 10; a = -a; a;"), -10.0, 1e-9);
    EXPECT_NEAR(compile_and_get_constant("var a = 3.14; var b = 2.0; a * b;"), 6.28, 1e-9);
}

TEST(CompilerTest, VariableComplexLogic) {
    EXPECT_EQ(compile_and_get_bool_constant("var a = true; var b = false; a == !b;").value_or(false), true);
    EXPECT_EQ(compile_and_get_bool_constant("var a = 5; a != 5;").value_or(true), false);
    EXPECT_EQ(compile_and_get_bool_constant("var a = 10; var b = 10; a <= b;").value_or(false), true);
    EXPECT_EQ(compile_and_get_bool_constant("var a = 0; !a;").value_or(false), true);
}

TEST(CompilerTest, MultipleAssignments) {
    EXPECT_EQ(compile_and_get_constant("var a = 0; var b = 0; b = 10; a = 10; a + b;"), 20.0);
}

TEST(CompilerTest, BlockScopes) {
    // block scope overrides outer scope, but outer scope remains unaffected after block
    EXPECT_EQ(compile_and_get_constant("var a = 5; { var a = 10; } a;"), 5.0);
    EXPECT_EQ(compile_and_get_constant("var a = 5; { a = 10; } a;"), 10.0);
    EXPECT_EQ(compile_and_get_constant("var a = 5; { var b = 10; a = b; } a;"), 10.0);
}

TEST(CompilerTest, LogicalShortCircuit) {
    EXPECT_EQ(compile_and_get_bool_constant("false or 5;").value_or(false), true);
    EXPECT_EQ(compile_and_get_bool_constant("10 or 5;").value_or(false), true);
    EXPECT_EQ(compile_and_get_bool_constant("false and 5;").value_or(true), false);
    EXPECT_EQ(compile_and_get_bool_constant("10 and 5;").value_or(false), true);
    
    // short circuit: if we didn't short circuit, 1 / 0 would cause NaN or crash
    EXPECT_EQ(compile_and_get_bool_constant("true or (1/0 == 0);").value_or(false), true);
    EXPECT_EQ(compile_and_get_bool_constant("false and (1/0 == 0);").value_or(true), false);
}

TEST(CompilerTest, StaticTypes) {
    EXPECT_EQ(compile_and_get_constant("i32 a = 10; a;"), 10.0);
    EXPECT_NEAR(compile_and_get_constant("f32 b = 3.14; b;"), 3.14, 1e-5);
    EXPECT_EQ(compile_and_get_constant("i32 a; a;"), 0.0);
    EXPECT_EQ(compile_and_get_constant("i64 a = 100; a;"), 100.0);
    EXPECT_EQ(compile_and_get_constant("f64 a = 2.5; a;"), 2.5);
    EXPECT_EQ(compile_and_get_constant("f64 a; a;"), 0.0);
}

TEST(CompilerTest, StaticTypesArithmetic) {
    // integer arithmetic
    EXPECT_EQ(compile_and_get_constant("i32 a = 10; i32 b = 3; a + b;"), 13.0);
    EXPECT_EQ(compile_and_get_constant("i32 a = 10; i32 b = 3; a - b;"), 7.0);
    EXPECT_EQ(compile_and_get_constant("i32 a = 10; i32 b = 3; a * b;"), 30.0);
    EXPECT_EQ(compile_and_get_constant("i32 a = 10; i32 b = 3; a / b;"), 3.0); // integer division truncates to 3
    
    // unary minus
    EXPECT_EQ(compile_and_get_constant("i32 a = 10; -a;"), -10.0);
    
    // type coercion (mixed types)
    EXPECT_NEAR(compile_and_get_constant("i32 a = 10; f64 b = 2.5; a + b;"), 12.5, 1e-5);
    EXPECT_NEAR(compile_and_get_constant("f64 a = 2.5; i32 b = 10; a - b;"), -7.5, 1e-5);
    
    // comparisons
    EXPECT_EQ(compile_and_get_bool_constant("i32 a = 10; i32 b = 15; a < b;").value_or(false), true);
    EXPECT_EQ(compile_and_get_bool_constant("i32 a = 10; i32 b = 15; a > b;").value_or(true), false);
    EXPECT_EQ(compile_and_get_bool_constant("i32 a = 10; i32 b = 10; a <= b;").value_or(false), true);
    EXPECT_EQ(compile_and_get_bool_constant("i32 a = -10; i32 b = 5; a < b;").value_or(false), true); // signed comparison
}

TEST(CompilerTest, IfElseControlFlow) {
    EXPECT_EQ(compile_and_get_constant("var a = 0; if (true) a = 1; a;"), 1.0);
    EXPECT_EQ(compile_and_get_constant("var a = 0; if (false) a = 1; a;"), 0.0);
    EXPECT_EQ(compile_and_get_constant("var a = 0; if (true) a = 1; else a = 2; a;"), 1.0);
    EXPECT_EQ(compile_and_get_constant("var a = 0; if (false) a = 1; else a = 2; a;"), 2.0);
    
    // nested ifs
    EXPECT_EQ(compile_and_get_constant("var a = 0; if (true) { if (false) a = 1; else a = 2; } a;"), 2.0);
}

TEST(CompilerTest, TruthinessEvaluation) {
    // tests that verify if (cond) treats 0 as false and non-zero as true
    // (which is what CreateFCmpONE implements)
    EXPECT_EQ(compile_and_get_constant("var a = 10; if (0) a = 20; a;"), 10.0);
    EXPECT_EQ(compile_and_get_constant("var a = 10; if (0.0) a = 20; a;"), 10.0);
    EXPECT_EQ(compile_and_get_constant("var a = 10; if (1) a = 20; a;"), 20.0);
    EXPECT_EQ(compile_and_get_constant("var a = 10; if (-1.5) a = 20; a;"), 20.0);
}

TEST(CompilerTest, WhileLoops) {
    EXPECT_EQ(compile_and_get_constant("var a = 0; while (a < 10) a = a + 1; a;"), 10.0);
    EXPECT_EQ(compile_and_get_constant("var a = 1; while (a < 100) a = a * 2; a;"), 128.0);
}

TEST(CompilerTest, ForLoops) {
    EXPECT_EQ(compile_and_get_constant("var a = 0; for (var i = 0; i < 10; i = i + 1) a = a + i; a;"), 45.0);
    EXPECT_EQ(compile_and_get_constant("var a = 0; var i = 0; for (; i < 5; i = i + 1) a = a + 2; a;"), 10.0);
}

TEST(CompilerTest, ConstantFoldingStrings) {
    EXPECT_EQ(compile_and_get_bool_constant("\"hello\" == \"hello\";").value_or(false), true);
    EXPECT_EQ(compile_and_get_bool_constant("\"hello\" == \" world\";").value_or(true), false);
    EXPECT_EQ(compile_and_get_bool_constant("\"hello\" + \" world\" == \"hello world\";").value_or(false), true);
}

TEST(CompilerTest, VariablesAndStrings) {
    EXPECT_EQ(compile_and_get_bool_constant("var a = \"foo\"; var b = \"bar\"; a + b == \"foobar\";").value_or(false), true);
    EXPECT_EQ(compile_and_get_bool_constant("var a = \"foo\"; a = a + \"baz\"; a == \"foobaz\";").value_or(false), true);
}

extern "C" char* lox_concat_string(const char* a, const char* b);
extern "C" void lox_free_strings();

TEST(CompilerTest, StringInterningReuseTest) {
    lox_free_strings();
    
    char* ptr1 = lox_concat_string("hello", "world");
    char* ptr2 = lox_concat_string("hello", "world");
    char* ptr3 = lox_concat_string("hello", "world");
    char* ptr4 = lox_concat_string("hello", "world");

    // all pointers should be exactly the same memory address
    EXPECT_EQ(ptr1, ptr2);
    EXPECT_EQ(ptr1, ptr3);
    EXPECT_EQ(ptr1, ptr4);
    
    EXPECT_STREQ(ptr1, "helloworld");
    
    lox_free_strings();
}
