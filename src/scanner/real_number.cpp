#include <scanner/real_number.h>
#include <iomanip>
#include <sstream>
#include <cmath>

std::string to_string(const RealNumber& number) {
    std::string sign = number.is_negative ? "-" : "";
    if (number.n_fractional_digits == 0 || number.fractional == 0)
        return sign + std::to_string(number.integer);

    // {:0{}} means: pad with '0', and take the width from the next argument
    return std::format("{}{}.{:0{}}", sign, number.integer, number.fractional, number.n_fractional_digits);
}

std::string to_string(const RealNumber* number) {
    return to_string(*number);
}

std::string to_string_as_expected_by_evaluation_system(const RealNumber& number) {
    std::string sign = number.is_negative ? "-" : "";
    if (number.n_fractional_digits == 0 || number.fractional == 0)
        return sign + std::to_string(number.integer) + ".0";

    std::stringstream ss;
    ss << sign << number.integer << "." << std::setw(number.n_fractional_digits) << std::setfill('0') << number.fractional;
    return ss.str();
}

std::string to_string_as_expected_by_evaluation_system(const RealNumber* number) {
    return to_string_as_expected_by_evaluation_system(*number);
}

[[nodiscard]] double RealNumber::to_double() const {
    if (n_fractional_digits == 0)
        return (is_negative ? -1 : 1) * static_cast<double>(integer);

    return (is_negative ? -1 : 1) * (static_cast<double>(integer) + static_cast<double>(fractional) / std::pow(10.0, n_fractional_digits));
}

RealNumber operator+(const RealNumber& lhs, const RealNumber& rhs) {
    int max_digits = std::max(lhs.n_fractional_digits, rhs.n_fractional_digits);

    // e.g. 0.0001 + 0.1 =
    // 1 / 10^4 + 1 / 10^1 = (1 + 1 * 10^(4 - 1)) / 10^4 = (1 + 1 * 10^3) / 10^4
    unsigned long long lhs_frac = lhs.fractional * static_cast<unsigned long long>(std::pow(10, max_digits - lhs.n_fractional_digits));
    unsigned long long rhs_frac = rhs.fractional * static_cast<unsigned long long>(std::pow(10, max_digits - rhs.n_fractional_digits));

    if (lhs.is_negative == rhs.is_negative) {
        // -a - b = -(a + b)
        // a + b = a + b

        unsigned long long new_frac = lhs_frac + rhs_frac;
        auto carry_threshold = static_cast<unsigned long long>(std::pow(10, max_digits)); // if new_frac >= this value, carrying is needed
        int carry = 0;
        if (new_frac >= carry_threshold) {
            // e.g., 0.99 + 0.09, new_frac = 100, carry_threshold = 100 -> carry = 1, new_frac = 0
            carry = 1;
            new_frac -= carry_threshold;
        }

        return RealNumber{
            .integer = lhs.integer + rhs.integer + carry,
            .fractional = new_frac,
            .n_fractional_digits = max_digits,
            .is_negative = lhs.is_negative //&& rhs.is_negative // -a - b = -(a + b)
        };
    }

    // different signs: Magnitude subtraction (a - b) or (-a + b)
    // determine which operand has the larger absolute value
    bool lhs_is_larger;
    if (lhs.integer != rhs.integer)
        lhs_is_larger = lhs.integer > rhs.integer;
    else
        lhs_is_larger = lhs_frac >= rhs_frac;

    const RealNumber& larger = lhs_is_larger ? lhs : rhs;
    const RealNumber& smaller = lhs_is_larger ? rhs : lhs;
    unsigned long long larger_frac = lhs_is_larger ? lhs_frac : rhs_frac;
    unsigned long long smaller_frac = lhs_is_larger ? rhs_frac : lhs_frac;

    unsigned long long res_frac;
    auto borrow_threshold = static_cast<unsigned long long>(std::pow(10, max_digits));
    int borrow = 0;

    if (larger_frac < smaller_frac) {
        // e.g., 1.2 + (-0.5): larger_frac(2) < smaller_frac(5), we borrow 1 from the integer part.
        borrow = 1;
        // res_frac = (10 + 2) - 5 = 7 (10 is the borrow_threshold for 1 fractional digit)
        res_frac = (borrow_threshold + larger_frac) - smaller_frac;
    } else {
        res_frac = larger_frac - smaller_frac;
    }

    return RealNumber{
        .integer = larger.integer - smaller.integer - borrow,
        .fractional = res_frac,
        .n_fractional_digits = max_digits,
        .is_negative = larger.is_negative
    };
}

std::string operator+(const std::string& lhs, const RealNumber& rhs) {
    return lhs + to_string(rhs);
}

std::string operator+(const RealNumber& lhs, const std::string& rhs) {
    return to_string(lhs) + rhs;
}

RealNumber operator-(const RealNumber& lhs, const RealNumber& rhs) {
    return lhs + RealNumber{rhs.integer, rhs.fractional, rhs.n_fractional_digits, !rhs.is_negative};
}

bool operator==(const RealNumber& lhs, const RealNumber& rhs) {
    // Handle zero case: 0.0 == -0.0 should be true
    bool lhs_is_zero = (lhs.integer == 0 && lhs.fractional == 0);
    bool rhs_is_zero = (rhs.integer == 0 && rhs.fractional == 0);

    if (lhs_is_zero && rhs_is_zero)
        return true;

    // If not both zero, signs must match
    if (lhs.is_negative != rhs.is_negative)
        return false;

    // Compare integer parts
    if (lhs.integer != rhs.integer)
        return false;

    // Normalize fractional parts for comparison
    int max_digits = std::max(lhs.n_fractional_digits, rhs.n_fractional_digits);
    unsigned long long lhs_frac_normalized = lhs.fractional * static_cast<unsigned long long>(std::pow(10, max_digits - lhs.n_fractional_digits));
    unsigned long long rhs_frac_normalized = rhs.fractional * static_cast<unsigned long long>(std::pow(10, max_digits - rhs.n_fractional_digits));

    return lhs_frac_normalized == rhs_frac_normalized;
}

std::ostream& operator<<(std::ostream& os, const RealNumber& number) {
    os << to_string(number);
    return os;
}
