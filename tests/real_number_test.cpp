#include <gtest/gtest.h>
#include <scanner/real_number.h>

TEST(RealNumberTest, AdditionSameSign) {
    // 1.2 + 1.3 = 2.5
    RealNumber a = {.integer = 1, .fractional = 2, .n_fractional_digits = 1, .is_negative = false};
    RealNumber b = {.integer = 1, .fractional = 3, .n_fractional_digits = 1, .is_negative = false};
    RealNumber res = a + b;
    EXPECT_EQ(res.integer, 2);
    EXPECT_EQ(res.fractional, 5);
    EXPECT_FALSE(res.is_negative);

    // 0.9 + 0.1 = 1.0 (Carry)
    RealNumber c = {.integer = 0, .fractional = 9, .n_fractional_digits = 1, .is_negative = false};
    RealNumber d = {.integer = 0, .fractional = 1, .n_fractional_digits = 1, .is_negative = false};
    RealNumber res2 = c + d;
    EXPECT_EQ(res2.integer, 1);
    EXPECT_EQ(res2.fractional, 0);
}

TEST(RealNumberTest, AdditionDifferentSignsMagnitudeSubtraction) {
    // 1.5 + (-0.5) = 1.0
    RealNumber a = {.integer = 1, .fractional = 5, .n_fractional_digits = 1, .is_negative = false};
    RealNumber b = {.integer = 0, .fractional = 5, .n_fractional_digits = 1, .is_negative = true};
    RealNumber res = a + b;
    EXPECT_EQ(res.integer, 1);
    EXPECT_EQ(res.fractional, 0);
    EXPECT_FALSE(res.is_negative);

    // 0.5 + (-1.5) = -1.0
    RealNumber res2 = b + a;
    EXPECT_EQ(res2.integer, 1);
    EXPECT_EQ(res2.fractional, 0);
    EXPECT_TRUE(res2.is_negative);
}

TEST(RealNumberTest, SubtractionWithBorrow) {
    // 1.2 + (-0.5) = 0.7
    // Larger_frac(2) < smaller_frac(5) -> Borrow 1 from integer
    RealNumber a = {.integer = 1, .fractional = 2, .n_fractional_digits = 1, .is_negative = false};
    RealNumber b = {.integer = 0, .fractional = 5, .n_fractional_digits = 1, .is_negative = true};
    RealNumber res = a + b;
    EXPECT_EQ(res.integer, 0);
    EXPECT_EQ(res.fractional, 7);
    EXPECT_FALSE(res.is_negative);

    // -1.2 + 0.5 = -0.7
    RealNumber c = {.integer = 1, .fractional = 2, .n_fractional_digits = 1, .is_negative = true};
    RealNumber d = {.integer = 0, .fractional = 5, .n_fractional_digits = 1, .is_negative = false};
    RealNumber res2 = c + d;
    EXPECT_EQ(res2.integer, 0);
    EXPECT_EQ(res2.fractional, 7);
    EXPECT_TRUE(res2.is_negative);
}

TEST(RealNumberTest, MixedPrecision) {
    // 0.1 (1 digit) + 0.02 (2 digits) = 0.12
    RealNumber a = {.integer = 0, .fractional = 1, .n_fractional_digits = 1, .is_negative = false};
    RealNumber b = {.integer = 0, .fractional = 2, .n_fractional_digits = 2, .is_negative = false};
    RealNumber res = a + b;
    EXPECT_EQ(res.integer, 0);
    EXPECT_EQ(res.fractional, 12);
    EXPECT_EQ(res.n_fractional_digits, 2);
}

TEST(RealNumberTest, SubtractionOperator) {
    // 5.0 - 2.0 = 3.0
    RealNumber a = {.integer = 5, .fractional = 0, .n_fractional_digits = 1, .is_negative = false};
    RealNumber b = {.integer = 2, .fractional = 0, .n_fractional_digits = 1, .is_negative = false};
    RealNumber res = a - b;
    EXPECT_EQ(res.integer, 3);
    EXPECT_FALSE(res.is_negative);

    // 2.0 - 5.0 = -3.0
    RealNumber res2 = b - a;
    EXPECT_EQ(res2.integer, 3);
    EXPECT_TRUE(res2.is_negative);
}

TEST(RealNumberTest, StringConversion) {
    RealNumber a = {.integer = 1, .fractional = 5, .n_fractional_digits = 2, .is_negative = true};
    // Should be "-1.05" because of the 2-digit padding
    EXPECT_EQ(to_string(a), "-1.05");
    
    RealNumber b = {.integer = 10, .fractional = 0, .n_fractional_digits = 0, .is_negative = true};
    EXPECT_EQ(to_string(b), "-10");
}

TEST(RealNumberTest, ToDouble) {
    RealNumber a = {.integer = 1, .fractional = 5, .n_fractional_digits = 1, .is_negative = true};
    EXPECT_DOUBLE_EQ(a.to_double(), -1.5);
}