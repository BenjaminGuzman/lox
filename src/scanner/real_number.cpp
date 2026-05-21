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

bool operator!=(const RealNumber& lhs, const RealNumber& rhs) {
    return !(lhs == rhs);
}

bool operator<(const RealNumber& lhs, const RealNumber& rhs) {
    // -0.0 and 0.0 are equal, so one is not less than the other
    bool lhs_is_zero = (lhs.integer == 0 && lhs.fractional == 0);
    bool rhs_is_zero = (rhs.integer == 0 && rhs.fractional == 0);
    if (lhs_is_zero && rhs_is_zero)
        return false;

    // if signs differ, the negative one is smaller
    if (lhs.is_negative != rhs.is_negative)
        return lhs.is_negative;

    // same sign
    // compare the magnitudes
    bool is_abs_lhs_less = false;
    if (lhs.integer != rhs.integer)
        is_abs_lhs_less = lhs.integer < rhs.integer;
    else {
        int max_digits = std::max(lhs.n_fractional_digits, rhs.n_fractional_digits);
        auto lhs_f = lhs.fractional * static_cast<unsigned long long>(std::pow(10, max_digits - lhs.n_fractional_digits));
        auto rhs_f = rhs.fractional * static_cast<unsigned long long>(std::pow(10, max_digits - rhs.n_fractional_digits));
        if (lhs_f != rhs_f)
            is_abs_lhs_less = lhs_f < rhs_f;
    }

    // if negative, the relationship is inverted (e.g., -5 < -3).
    return lhs.is_negative ? !is_abs_lhs_less : is_abs_lhs_less;
}

bool operator<=(const RealNumber& lhs, const RealNumber& rhs) {
    bool lhs_is_zero = (lhs.integer == 0 && lhs.fractional == 0);
    bool rhs_is_zero = (rhs.integer == 0 && rhs.fractional == 0);
    if (lhs_is_zero && rhs_is_zero)
        return true;

    // if signs differ, the negative one is smaller
    if (lhs.is_negative != rhs.is_negative)
        return lhs.is_negative;

    // same sign
    // compare the magnitudes
    bool is_abs_lhs_less_or_equal = false;
    if (lhs.integer != rhs.integer)
        is_abs_lhs_less_or_equal = lhs.integer <= rhs.integer;
    else {
        int max_digits = std::max(lhs.n_fractional_digits, rhs.n_fractional_digits);
        auto lhs_f = lhs.fractional * static_cast<unsigned long long>(std::pow(10, max_digits - lhs.n_fractional_digits));
        auto rhs_f = rhs.fractional * static_cast<unsigned long long>(std::pow(10, max_digits - rhs.n_fractional_digits));
        if (lhs_f == rhs_f)
            return true; // magnitudes are equal

        is_abs_lhs_less_or_equal = lhs_f <= rhs_f;
    }

    // if negative, the relationship is inverted (e.g., -5 < -3).
    return lhs.is_negative ? !is_abs_lhs_less_or_equal : is_abs_lhs_less_or_equal;
}

bool operator>(const RealNumber& lhs, const RealNumber& rhs) {
    // -0.0 and 0.0 are equal, so one is not less than the other
    bool lhs_is_zero = (lhs.integer == 0 && lhs.fractional == 0);
    bool rhs_is_zero = (rhs.integer == 0 && rhs.fractional == 0);
    if (lhs_is_zero && rhs_is_zero)
        return false;

    // if signs differ, the positive one is greater
    if (lhs.is_negative != rhs.is_negative)
        return !lhs.is_negative;

    // same sign
    // compare the magnitudes
    bool is_abs_lhs_greater = false;
    if (lhs.integer != rhs.integer)
        is_abs_lhs_greater = lhs.integer > rhs.integer;
    else {
        int max_digits = std::max(lhs.n_fractional_digits, rhs.n_fractional_digits);
        auto lhs_f = lhs.fractional * static_cast<unsigned long long>(std::pow(10, max_digits - lhs.n_fractional_digits));
        auto rhs_f = rhs.fractional * static_cast<unsigned long long>(std::pow(10, max_digits - rhs.n_fractional_digits));
        if (lhs_f != rhs_f)
            is_abs_lhs_greater = lhs_f > rhs_f;
    }

    // if negative, the relationship is inverted (e.g., -5 > -3).
    return lhs.is_negative ? !is_abs_lhs_greater : is_abs_lhs_greater;
}

bool operator>=(const RealNumber& lhs, const RealNumber& rhs) {
    bool lhs_is_zero = (lhs.integer == 0 && lhs.fractional == 0);
    bool rhs_is_zero = (rhs.integer == 0 && rhs.fractional == 0);
    if (lhs_is_zero && rhs_is_zero)
        return true;

    // if signs differ, the positive one is greater
    if (lhs.is_negative != rhs.is_negative)
        return !lhs.is_negative;

    // same sign
    // compare the magnitudes
    bool is_abs_rhs_greater_or_equal = false;
    if (lhs.integer != rhs.integer)
        is_abs_rhs_greater_or_equal = lhs.integer >= rhs.integer;
    else {
        int max_digits = std::max(lhs.n_fractional_digits, rhs.n_fractional_digits);
        auto lhs_f = lhs.fractional * static_cast<unsigned long long>(std::pow(10, max_digits - lhs.n_fractional_digits));
        auto rhs_f = rhs.fractional * static_cast<unsigned long long>(std::pow(10, max_digits - rhs.n_fractional_digits));
        if (lhs_f == rhs_f)
            return true; // magnitudes are equal
        is_abs_rhs_greater_or_equal = lhs_f >= rhs_f;
    }

    // if negative, the relationship is inverted (e.g., -5 < -3).
    return lhs.is_negative ? !is_abs_rhs_greater_or_equal : is_abs_rhs_greater_or_equal;
}

std::ostream& operator<<(std::ostream& os, const RealNumber& number) {
    os << to_string(number);
    return os;
}

RealNumber operator*(const RealNumber& lhs, const RealNumber& rhs) {
    bool res_is_negative = lhs.is_negative != rhs.is_negative;
    int res_n_digits = lhs.n_fractional_digits + rhs.n_fractional_digits;

    // (int1 + frac1/10^lhs.n) * (int2 + frac2/10^rhs.n) = int1 * int2 
    //   + int1 * frac2/10^rhs.n  +  int2 * frac1/10^lhs.n  +  frac1/10^lhs.n * frac2/10^rhs.n
    //  = int1 * int2
    //   + (int1 * frac2 * 10^lhs.n  +  int2 * frac1 * 10^rhs.n  +  frac1 * frac2) / 10^(lhs.n + rhs.n)
    // Calculate fractional contributions using a common denominator: 10^(lhs.n + rhs.n)
    // Term 1: (lhs.integer * rhs.fractional) / 10^rhs.n * 10^lhs.n
    //         (int1 * frac2/10^rhs.n) * 10^(lhs.n + rhs.n) = (int1 * frac2) * 10^lhs.n 
    // Term 2: (rhs.integer * lhs.fractional) / 10^lhs.n * 10^rhs.n
    //         (int2 * frac1/10^lhs.n) * 10^(lhs.n + rhs.n) = (int2 * frac1) * 10^rhs.n
    // Term 3: (frac1/10^lhs.n * frac2/10^rhs.n) * 10^(lhs.n + rhs.n) = frac1 * frac2

    unsigned long long t1 = lhs.integer * rhs.fractional * static_cast<unsigned long long>(std::pow(10, lhs.n_fractional_digits));
    unsigned long long t2 = rhs.integer * lhs.fractional * static_cast<unsigned long long>(std::pow(10, rhs.n_fractional_digits));
    unsigned long long t3 = lhs.fractional * rhs.fractional;

    unsigned long long sum_frac = t1 + t2 + t3;
    unsigned long long carry = 0;
    unsigned long long res_frac = 0;

    if (res_n_digits > 0) {
        auto divisor = static_cast<unsigned long long>(std::pow(10, res_n_digits));
        carry = sum_frac / divisor;
        res_frac = sum_frac % divisor;
    }

    unsigned long long res_int = (lhs.integer * rhs.integer) + carry;

    return RealNumber{
        .integer = res_int,
        .fractional = res_frac,
        .n_fractional_digits = res_n_digits,
        .is_negative = (res_int == 0 && res_frac == 0) ? false : res_is_negative
    };
}

RealNumber operator/(const RealNumber& lhs, const RealNumber& rhs) {
    // TODO Use Newton-Raphson method
    // FIXME Disclaimer: this code was written by AI!!!


    // 1. Sign and Zero Handling
    bool res_is_negative = lhs.is_negative != rhs.is_negative;
    if (rhs.integer == 0 && rhs.fractional == 0) {
        throw std::runtime_error("Division by zero");
    }
    if (lhs.integer == 0 && lhs.fractional == 0) {
        return RealNumber{0, 0, 0, false};
    }

    // 2. Determine target precision
    const int TARGET_FRACTIONAL_DIGITS = 15; // A reasonable precision for unsigned long long

    // Helper to get power of 10 safely using std::pow
    auto get_pow10_ull = [](int exponent) -> unsigned long long {
        if (exponent < 0) {
            throw std::invalid_argument("Exponent for power of 10 cannot be negative.");
        }
        double p = std::pow(10.0, exponent);
        // Check for overflow before casting to unsigned long long
        if (p > std::numeric_limits<unsigned long long>::max() || p < 0) {
            throw std::overflow_error("Power of 10 calculation overflowed unsigned long long");
        }
        return static_cast<unsigned long long>(std::round(p));
    };

    // 3. Convert lhs and rhs to large integers for division
    // Calculate raw values (integer * 10^n_frac + fractional)
    // This involves multiplying by powers of 10 and adding fractional parts.
    // Overflow checks are crucial here.

    unsigned long long lhs_frac_power = get_pow10_ull(lhs.n_fractional_digits);
    unsigned long long lhs_raw_val = lhs.integer;
    // Check for overflow before multiplication
    if (lhs_frac_power != 0 && std::numeric_limits<unsigned long long>::max() / lhs_frac_power < lhs_raw_val) {
        throw std::overflow_error("RealNumber overflow during division scaling (lhs integer part)");
    }
    lhs_raw_val *= lhs_frac_power;
    // Check for overflow before addition
    if (std::numeric_limits<unsigned long long>::max() - lhs.fractional < lhs_raw_val) {
        throw std::overflow_error("RealNumber overflow during division scaling (lhs fractional part)");
    }
    lhs_raw_val += lhs.fractional;

    unsigned long long rhs_frac_power = get_pow10_ull(rhs.n_fractional_digits);
    unsigned long long rhs_raw_val = rhs.integer;
    // Check for overflow before multiplication
    if (rhs_frac_power != 0 && std::numeric_limits<unsigned long long>::max() / rhs_frac_power < rhs_raw_val) {
        throw std::overflow_error("RealNumber overflow during division scaling (rhs integer part)");
    }
    rhs_raw_val *= rhs_frac_power;
    // Check for overflow before addition
    if (std::numeric_limits<unsigned long long>::max() - rhs.fractional < rhs_raw_val) {
        throw std::overflow_error("RealNumber overflow during division scaling (rhs fractional part)");
    }
    rhs_raw_val += rhs.fractional;

    // We are effectively calculating:
    // (lhs_raw_val / 10^lhs.n_fractional_digits) / (rhs_raw_val / 10^rhs.n_fractional_digits)
    // = (lhs_raw_val * 10^rhs.n_fractional_digits) / (rhs_raw_val * 10^lhs.n_fractional_digits)

    // To get TARGET_FRACTIONAL_DIGITS in the result, we need to multiply the numerator by 10^TARGET_FRACTIONAL_DIGITS.
    // So, the final division is:
    // (lhs_raw_val * 10^(rhs.n_fractional_digits + TARGET_FRACTIONAL_DIGITS)) / (rhs_raw_val * 10^lhs.n_fractional_digits)

    int net_exponent_for_numerator = rhs.n_fractional_digits + TARGET_FRACTIONAL_DIGITS - lhs.n_fractional_digits;

    unsigned long long final_numerator_for_division = lhs_raw_val;
    unsigned long long final_denominator_for_division = rhs_raw_val;

    if (net_exponent_for_numerator > 0) {
        unsigned long long scale_factor = get_pow10_ull(net_exponent_for_numerator);
        if (scale_factor != 0 && std::numeric_limits<unsigned long long>::max() / scale_factor < final_numerator_for_division) {
            throw std::overflow_error("RealNumber overflow during division scaling (final numerator)");
        }
        final_numerator_for_division *= scale_factor;
    } else if (net_exponent_for_numerator < 0) {
        // This means the denominator needs to be scaled up.
        unsigned long long scale_factor = get_pow10_ull(-net_exponent_for_numerator);
        if (scale_factor != 0 && std::numeric_limits<unsigned long long>::max() / scale_factor < final_denominator_for_division) {
            throw std::overflow_error("RealNumber overflow during division scaling (final denominator)");
        }
        final_denominator_for_division *= scale_factor;
    }

    // Perform the integer division
    unsigned long long result_raw = final_numerator_for_division / final_denominator_for_division;
    unsigned long long remainder = final_numerator_for_division % final_denominator_for_division;

    // Rounding: check if the remainder is half or more of the divisor
    if (remainder * 2 >= final_denominator_for_division) {
        result_raw++;
    }

    // Extract integer and fractional parts from result_raw
    unsigned long long target_frac_power = get_pow10_ull(TARGET_FRACTIONAL_DIGITS);
    unsigned long long res_integer = result_raw / target_frac_power;
    unsigned long long res_fractional = result_raw % target_frac_power;

    // Special handling for negative zero
    if (res_integer == 0 && res_fractional == 0) {
        res_is_negative = false;
    }

    // Normalize fractional part by trimming trailing zeros
    int final_n_fractional_digits = TARGET_FRACTIONAL_DIGITS;
    if (res_fractional != 0) {
        while (res_fractional % 10 == 0 && final_n_fractional_digits > 0) {
            res_fractional /= 10;
            final_n_fractional_digits--;
        }
    } else {
        final_n_fractional_digits = 0; // If fractional part is 0, then 0 fractional digits
    }

    return RealNumber{
        .integer = res_integer,
        .fractional = res_fractional,
        .n_fractional_digits = final_n_fractional_digits,
        .is_negative = res_is_negative
    };
}

