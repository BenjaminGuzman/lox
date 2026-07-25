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
    llvm::Value* val = compiler.compile(ast.root);
    
    if (auto* constFp = llvm::dyn_cast_or_null<llvm::ConstantFP>(val)) {
        return constFp->getValueAPF().convertToDouble();
    }
    
    return std::nan("");
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
    lox::Scanner scanner(source, false);
    lox::AST ast(scanner, false);
    auto _ = ast.build();

    lox::Compiler compiler;
    llvm::Value* val = compiler.compile(ast.root);
    
    if (auto* constInt = llvm::dyn_cast_or_null<llvm::ConstantInt>(val)) {
        if (constInt->getBitWidth() == 1) {
            return constInt->getZExtValue() != 0;
        }
    }
    
    return std::nullopt;
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

