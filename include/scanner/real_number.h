#ifndef CODECRAFTERS_INTERPRETER_REALNUMBER_H
#define CODECRAFTERS_INTERPRETER_REALNUMBER_H
#include <string>
#include <ostream>

/**
 * Actual number is = integer + fractional / 10 ^ n_fractional_digits
 * e.g., 3.001 = {integer = 3, fractional = 1, n_fractional_digits = 3}
 */
class RealNumber {
public:
    unsigned long long integer{};
    unsigned long long fractional{};
    int n_fractional_digits{};
    bool is_negative = false;
    [[nodiscard]] double to_double() const;
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


#endif //CODECRAFTERS_INTERPRETER_REALNUMBER_H