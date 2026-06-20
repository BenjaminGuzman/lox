#ifndef CODECRAFTERS_INTERPRETER_REALNUMBER_H
#define CODECRAFTERS_INTERPRETER_REALNUMBER_H
#include <string>
#include <ostream>

namespace lox {
/**
 * Actual number is = integer + fractional / 10 ^ n_fractional_digits
 * e.g., 3.001 = {integer = 3, fractional = 1, n_fractional_digits = 3}
 */
class RealNumber {
public:
    /**
     * Max number of iterations to perform when the Newton-Raphson method is used
     */
    static const int NEWTON_RAPHSON_MAX_ITERATIONS = 10;

    unsigned long long integer{};
    unsigned long long fractional{};
    int n_fractional_digits{};
    bool is_negative = false;
    [[nodiscard]] double to_double() const;

    /**
     *
     * @return the reciprocal number (1/n) for this number (n)
     */
    [[nodiscard]] RealNumber reciprocal() const;

    /**
     * Rounds the fractional part of the number
     * @param n_digits number of decimal digits expected
     * @return itself (rounded)
     */
    RealNumber& round(unsigned int n_digits);

    /**
     * Cleans/removes the trailing zeroes from this number's fractional part
     * @return itself (with the trailing zeroes removed)
     */
    RealNumber& clean_trailing_zeroes();

    /**
     * Reduces noise (e.g., trailing 999995) in the fractional part
     * @return itself (with noise reduced)
     */
    RealNumber& reduce_noise();

    /**
     * Counts the number of decimal digits that there exists in a number n
     * @param n the number
     * @return the count of decimal digits
     */
    static size_t count_digits(unsigned long long n);
};

/**
 * yeah... this is shit haha
 *   idk why the parsing output expects the numbers as 40.0 if it comes as 40
 *   but the evaluation expects them as 40 if it comes as 40
 * @param number
 * @return
 */
std::string to_string_as_expected_by_evaluation_system(const RealNumber& number);
std::string to_string_as_expected_by_evaluation_system(const RealNumber* number);

std::string to_string(const RealNumber& number);
std::string to_string(const RealNumber* number);

RealNumber operator+(const RealNumber& lhs, const RealNumber& rhs);
RealNumber operator-(const RealNumber& lhs, const RealNumber& rhs);
//RealNumber operator-(int lhs, const RealNumber& rhs); pending implementation
bool operator==(const RealNumber& lhs, const RealNumber& rhs);
bool operator!=(const RealNumber& lhs, const RealNumber& rhs);

std::string operator+(const std::string& lhs, const RealNumber& rhs);
std::string operator+(const RealNumber& lhs, const std::string& rhs);

RealNumber operator*(const RealNumber& lhs, const RealNumber& rhs);
RealNumber operator/(const RealNumber& lhs, const RealNumber& rhs);

bool operator<(const RealNumber& lhs, const RealNumber& rhs);
bool operator>(const RealNumber& lhs, const RealNumber& rhs);
bool operator<=(const RealNumber& lhs, const RealNumber& rhs);
bool operator>=(const RealNumber& lhs, const RealNumber& rhs);


std::ostream& operator<<(std::ostream& os, const RealNumber& number);

static constexpr unsigned long long POW10[] = {
    1, // 0
    10, // 1
    100, // 2
    1'000, // 3
    10'000, // 4
    100'000, // 5
    1'000'000, // 6
    10'000'000, // 7
    100'000'000, // 8
    1'000'000'000, // 9
    10'000'000'000, // 10
    100'000'000'000, // 11
    1'000'000'000'000, // 12
    10'000'000'000'000, // 13
    100'000'000'000'000, // 14
    1'000'000'000'000'000, // 15
    10'000'000'000'000'000, // 16
    100'000'000'000'000'000, // 17
    1'000'000'000'000'000'000, // 18
    10'000'000'000'000'000'000ull, // 19
};

/**
 * Computes the power of 10, i.e., 10 ^ n
 * in an efficient manner
 * @param n the exponent
 * @return the power of 10
 */
constexpr unsigned long long pow10(unsigned int n) {
    if (n < std::size(POW10))
        return POW10[n];

    return POW10[std::size(POW10) - 1]; // that's the max safe value for 10 ^ n
}
};

#endif //CODECRAFTERS_INTERPRETER_REALNUMBER_H