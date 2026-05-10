#include "gtest/gtest.h"
#include "scanner/real_number.h"
#include <string>
#include <cmath> // For std::abs

// Helper to compare doubles with a tolerance
bool doubles_are_equal(double a, double b, double epsilon = 1e-9) {
    return std::abs(a - b) < epsilon;
}

TEST(RealNumberTest, ConstructorInitialization) {
    RealNumber num1(1, 0, 0, false);
    ASSERT_EQ(num1.integer, 1ULL);
    ASSERT_EQ(num1.fractional, 0ULL);
    ASSERT_EQ(num1.n_fractional_digits, 0);
    ASSERT_FALSE(num1.is_negative);

    RealNumber num2(0, 123, 3, false);
    ASSERT_EQ(num2.integer, 0ULL);
    ASSERT_EQ(num2.fractional, 123ULL);
    ASSERT_EQ(num2.n_fractional_digits, 3);
    ASSERT_FALSE(num2.is_negative);

    RealNumber num3(5, 6789, 4, true);
    ASSERT_EQ(num3.integer, 5ULL);
    ASSERT_EQ(num3.fractional, 6789ULL);
    ASSERT_EQ(num3.n_fractional_digits, 4);
    ASSERT_TRUE(num3.is_negative);

    RealNumber num4(12345, 0, 0, true);
    ASSERT_EQ(num4.integer, 12345ULL);
    ASSERT_EQ(num4.fractional, 0ULL);
    ASSERT_EQ(num4.n_fractional_digits, 0);
    ASSERT_TRUE(num4.is_negative);
}

TEST(RealNumberTest, ToDoubleConversion) {
    RealNumber num1(1, 0, 0, false);
    ASSERT_TRUE(doubles_are_equal(num1.to_double(), 1.0));

    RealNumber num2(0, 123, 3, false);
    ASSERT_TRUE(doubles_are_equal(num2.to_double(), 0.123));

    RealNumber num3(5, 6789, 4, false);
    ASSERT_TRUE(doubles_are_equal(num3.to_double(), 5.6789));

    // This test will likely fail if is_negative is not handled in to_double
    RealNumber num4(5, 6789, 4, true);
    ASSERT_TRUE(doubles_are_equal(num4.to_double(), -5.6789));

    RealNumber num5(0, 0, 0, false);
    ASSERT_TRUE(doubles_are_equal(num5.to_double(), 0.0));
}

TEST(RealNumberTest, ToStringConversion) {
    RealNumber num1(1, 0, 0, false);
    ASSERT_EQ(to_string(num1), "1");

    RealNumber num2(0, 123, 3, false);
    ASSERT_EQ(to_string(num2), "0.123");

    RealNumber num3(5, 6789, 4, false);
    ASSERT_EQ(to_string(num3), "5.6789");

    // This test will likely fail if is_negative is not handled in to_string
    RealNumber num4(5, 6789, 4, true);
    ASSERT_EQ(to_string(num4), "-5.6789");

    RealNumber num5(0, 0, 0, false);
    ASSERT_EQ(to_string(num5), "0");

    RealNumber num6(123, 4, 2, false); // 123.04
    ASSERT_EQ(to_string(num6), "123.04");
}

TEST(RealNumberTest, ToStringAsExpectedByEvaluationSystem) {
    RealNumber num1(1, 0, 0, false);
    ASSERT_EQ(to_string_as_expected_by_evaluation_system(num1), "1.0");

    RealNumber num2(0, 123, 3, false);
    ASSERT_EQ(to_string_as_expected_by_evaluation_system(num2), "0.123");

    RealNumber num3(5, 6789, 4, false);
    ASSERT_EQ(to_string_as_expected_by_evaluation_system(num3), "5.6789");

    // This test will likely fail if is_negative is not handled
    RealNumber num4(5, 6789, 4, true);
    ASSERT_EQ(to_string_as_expected_by_evaluation_system(num4), "-5.6789");

    RealNumber num5(0, 0, 0, false);
    ASSERT_EQ(to_string_as_expected_by_evaluation_system(num5), "0.0");

    RealNumber num6(123, 4, 2, false); // 123.04
    ASSERT_EQ(to_string_as_expected_by_evaluation_system(num6), "123.04");
}

TEST(RealNumberTest, AdditionOperator) {
    RealNumber num1(1, 0, 0, false); // 1
    RealNumber num2(2, 0, 0, false); // 2
    RealNumber sum1 = num1 + num2;
    ASSERT_TRUE(doubles_are_equal(sum1.to_double(), 3.0));

    RealNumber num3(1, 5, 1, false); // 1.5
    RealNumber num4(2, 5, 1, false); // 2.5
    RealNumber sum2 = num3 + num4;
    ASSERT_TRUE(doubles_are_equal(sum2.to_double(), 4.0));

    RealNumber num5(0, 1, 1, false); // 0.1
    RealNumber num6(0, 1, 2, false); // 0.01
    RealNumber sum3 = num5 + num6;
    ASSERT_TRUE(doubles_are_equal(sum3.to_double(), 0.11));

    // These tests will likely fail if is_negative is not handled in operator+
    RealNumber num7(1, 0, 0, false); // 1
    RealNumber num8(2, 0, 0, true);  // -2
    RealNumber sum4 = num7 + num8;
    ASSERT_TRUE(doubles_are_equal(sum4.to_double(), -1.0));

    RealNumber num9(1, 0, 0, true);  // -1
    RealNumber num10(2, 0, 0, true); // -2
    RealNumber sum5 = num9 + num10;
    ASSERT_TRUE(doubles_are_equal(sum5.to_double(), -3.0));

    RealNumber num11(2, 0, 0, true); // -2
    RealNumber num12(1, 0, 0, false); // 1
    RealNumber sum6 = num11 + num12;
    ASSERT_TRUE(doubles_are_equal(sum6.to_double(), -1.0));
}

TEST(RealNumberTest, SubtractionOperator) {
    RealNumber num1(2, 0, 0, false); // 2
    RealNumber num2(1, 0, 0, false); // 1
    RealNumber diff1 = num1 - num2;
    ASSERT_TRUE(doubles_are_equal(diff1.to_double(), 1.0));

    RealNumber num3(2, 5, 1, false); // 2.5
    RealNumber num4(1, 5, 1, false); // 1.5
    RealNumber diff2 = num3 - num4;
    ASSERT_TRUE(doubles_are_equal(diff2.to_double(), 1.0));

    RealNumber num5(0, 1, 1, false); // 0.1
    RealNumber num6(0, 1, 2, false); // 0.01
    RealNumber diff3 = num5 - num6;
    ASSERT_TRUE(doubles_are_equal(diff3.to_double(), 0.09));

    // These tests will likely fail if is_negative is not handled in operator-
    RealNumber num7(1, 0, 0, false); // 1
    RealNumber num8(2, 0, 0, true);  // -2
    RealNumber diff4 = num7 - num8;
    ASSERT_TRUE(doubles_are_equal(diff4.to_double(), 3.0));

    RealNumber num9(1, 0, 0, true);  // -1
    RealNumber num10(2, 0, 0, true); // -2
    RealNumber diff5 = num9 - num10;
    ASSERT_TRUE(doubles_are_equal(diff5.to_double(), 1.0));

    RealNumber num11(2, 0, 0, true); // -2
    RealNumber num12(1, 0, 0, false); // 1
    RealNumber diff6 = num11 - num12;
    ASSERT_TRUE(doubles_are_equal(diff6.to_double(), -3.0));
}

TEST(RealNumberTest, StringAdditionOperator) {
    RealNumber num1(1, 23, 2, false); // 1.23
    std::string s1 = "hello" + num1;
    ASSERT_EQ(s1, "hello1.23");

    std::string s2 = num1 + "world";
    ASSERT_EQ(s2, "1.23world");

    RealNumber num2(10, 0, 0, false); // 10
    std::string s3 = "value: " + num2;
    ASSERT_EQ(s3, "value: 10");

    // This test will likely fail if is_negative is not handled in to_string
    RealNumber num3(4, 5, 1, true); // -4.5
    std::string s4 = "negative: " + num3;
    ASSERT_EQ(s4, "negative: -4.5");
}

TEST(RealNumberTest, EqualityOperator) {
    // Identical numbers
    RealNumber num1(1, 23, 2, false); // 1.23
    RealNumber num2(1, 23, 2, false); // 1.23
    ASSERT_TRUE(num1 == num2);

    // Numbers with different fractional digit representations but same value
    RealNumber num3(1, 0, 0, false); // 1
    RealNumber num4(1, 0, 1, false); // 1.0
    ASSERT_TRUE(num3 == num4);

    RealNumber num5(1, 20, 2, false); // 1.20
    RealNumber num6(1, 2, 1, false);  // 1.2
    ASSERT_TRUE(num5 == num6);

    // Inequality - different integer parts
    RealNumber num7(1, 0, 0, false); // 1
    RealNumber num8(2, 0, 0, false); // 2
    ASSERT_FALSE(num7 == num8);

    // Inequality - different fractional parts
    RealNumber num9(1, 23, 2, false);  // 1.23
    RealNumber num10(1, 24, 2, false); // 1.24
    ASSERT_FALSE(num9 == num10);

    // Equality with zero and negative zero
    RealNumber zero1(0, 0, 0, false); // 0
    RealNumber zero2(0, 0, 0, true);  // -0
    ASSERT_TRUE(zero1 == zero2);

    // Inequality - positive vs negative with same magnitude
    RealNumber num11(5, 0, 0, false); // 5
    RealNumber num12(5, 0, 0, true);  // -5
    ASSERT_FALSE(num11 == num12);

    // Inequality - different signs, non-zero
    RealNumber num13(1, 23, 2, false); // 1.23
    RealNumber num14(1, 23, 2, true);  // -1.23
    ASSERT_FALSE(num13 == num14);

    // Complex case
    RealNumber num15(123, 456, 3, false); // 123.456
    RealNumber num16(123, 456, 3, false); // 123.456
    ASSERT_TRUE(num15 == num16);

    RealNumber num17(123, 4560, 4, false); // 123.4560
    RealNumber num18(123, 456, 3, false);  // 123.456
    ASSERT_TRUE(num17 == num18);
}
