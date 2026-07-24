#include <scanner/real_number.h>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <sys/stat.h>

namespace lox {
std::string to_string(const RealNumber& number) {
    std::string sign = number.is_negative ? "-" : "";
    if (number.n_fractional_digits == 0 || number.fractional == 0)
        return sign + std::to_string(number.integer);

    std::stringstream ss;
    ss << sign << number.integer << "." << std::setw(number.n_fractional_digits) << std::setfill('0') << number.fractional;
    return ss.str();
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

    return (is_negative ? -1 : 1) * (static_cast<double>(integer) + static_cast<double>(fractional) / static_cast<double>(pow10(n_fractional_digits)));
}

RealNumber& RealNumber::round(unsigned int target_digits) {
    if (n_fractional_digits <= target_digits)
        return *this;

    unsigned int digits_to_remove = n_fractional_digits - target_digits;
    unsigned long long divisor = pow10(digits_to_remove);
    unsigned long long remainder = fractional % divisor;

    fractional /= divisor;

    // round up if the removed part is >= 0.5
    if (remainder >= divisor / 2) {
        ++fractional;
        // handle rollover (e.g., 0.99 rounded to 1 digit becomes 1.0)
        if (target_digits > 0 && fractional >= pow10(target_digits)) {
            fractional = 0;
            ++integer;
        } else if (target_digits == 0 && fractional > 0) {
            fractional = 0;
            integer++;
        }
    }

    n_fractional_digits = static_cast<int>(target_digits);
    return *this;
}

RealNumber& RealNumber::reduce_noise() {
    if (n_fractional_digits < 6)
        return *this;

    // inspect the extreme tail (last 6 digits) for noise patterns
    unsigned long long tail = fractional % pow10(6);
    if (tail >= 999'995) {
        // found noise like ...9998 -> round up mathematically
        fractional += 1'000'000 - tail;
        if (fractional >= pow10(n_fractional_digits)) {
            ++integer;
            fractional = 0;
            n_fractional_digits = 0;
        }
    } else if (tail <= 5) {
        // Found noise like ...0002 -> Truncate the noise
        fractional -= tail;
    }

    return *this;
}

RealNumber &RealNumber::clean_trailing_zeroes() {
    while (n_fractional_digits > 0 && fractional % 10 == 0) {
        fractional /= 10;
        n_fractional_digits--;
    }

    if (fractional == 0)
        n_fractional_digits = 0;

    return *this;
}

size_t RealNumber::count_digits(unsigned long long n) {
    int count = 0;

    while (n > 0) {
        ++count;
        n /= 10;
    }

    return count;
}

[[nodiscard]] RealNumber RealNumber::reciprocal() const {
    if (integer == 0 && fractional == 0)
        throw std::invalid_argument("Cannot find reciprocal of zero.");

    RealNumber n_abs = {
        .integer = this->integer,
        .fractional = this->fractional,
        .n_fractional_digits = this->n_fractional_digits,
        .is_negative = false
    };

    /*
     For the formula x_{i+1} = x_i (2 − n x_i) (Newton-Raphson formula for reciprocals) to converge to 1/n,
     the initial guess (x_0) must strictly fall within 0 < x_0 < 2 / n, otherwise it converges to -Inf
     We have 2 cases for finding an approximation:
      - When it has an integer part
        If a number has D integer digits, we know the number is strictly less than 10^D (e.g. 305 < 10^3)
        i.e., n < 10 ^ D -> 1 < 10^D / n -> 10^(-D) < 1 / n
        Since 1/n = (2/n) / 2, we have that 0 < x_0 <= 10^(-D) < 1 / n < 2 / n, i.e., 10^(-D) is a safe approximation
      - When it has a fractional part
        We count the number of "leading zeros" (Z) immediately after the decimal point before the first non-zero digit
        e.g., if n=0.0045, it has 2 leading zeros. This means n is strictly less than 10^(-Z) (e.g., 0.0045 < 10^(−2))
        Because n < 10 ^ (-Z) -> n 10^Z < 1 -> 10^Z < 1 / n we have that
        0 < x_0 <= 10^Z < 1 / n < 2 / n, i.e., 10^Z is a safe approximation
    */
    RealNumber x;
    if (integer > 0) { // x_0 = 10^(-D) = 0.000....0001
        size_t digits = count_digits(integer);
        x.integer = 0;
        x.fractional = 1;
        x.n_fractional_digits = static_cast<int>(digits);
    } else { // x_0 = 10^Z = 100....00.0
        size_t actual_frac_digits = count_digits(fractional);
        size_t leading_zeros = n_fractional_digits - actual_frac_digits;
        x.integer = pow10(leading_zeros);
        x.fractional = 0;
        x.n_fractional_digits = 0;
    }

    RealNumber two;
    two.integer = 2;
    two.fractional = 0;
    two.n_fractional_digits = 0;

    // fixed iterations are normal for arbitrary precision algorithms since checking
    // the dynamic distance between two huge string-numbers is computationally heavy.
    for (int i = 0; i < NEWTON_RAPHSON_MAX_ITERATIONS; ++i) {
        // x_{i + 1} = x_i (2 - n x_i)
        RealNumber next_x = x * (two - n_abs * x);
        x = next_x;
    }

    x.is_negative = is_negative;
    return x.reduce_noise().clean_trailing_zeroes();
}

RealNumber operator+(const RealNumber& lhs, const RealNumber& rhs) {
    int max_digits = std::max(lhs.n_fractional_digits, rhs.n_fractional_digits);

    // e.g. 0.0001 + 0.1 =
    // 1 / 10^4 + 1 / 10^1 = (1 + 1 * 10^(4 - 1)) / 10^4 = (1 + 1 * 10^3) / 10^4
    unsigned long long lhs_frac = lhs.fractional * pow10(max_digits - lhs.n_fractional_digits);
    unsigned long long rhs_frac = rhs.fractional * pow10(max_digits - rhs.n_fractional_digits);

    if (lhs.is_negative == rhs.is_negative) {
        // -a - b = -(a + b)
        // a + b = a + b

        unsigned long long new_frac = lhs_frac + rhs_frac;
        auto carry_threshold = pow10(max_digits); // if new_frac >= this value, carrying is needd
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
    auto borrow_threshold = pow10(max_digits);
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
    unsigned long long lhs_frac_normalized = lhs.fractional * pow10(max_digits - lhs.n_fractional_digits);
    unsigned long long rhs_frac_normalized = rhs.fractional * pow10(max_digits - rhs.n_fractional_digits);

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
        auto lhs_f = lhs.fractional * pow10(max_digits - lhs.n_fractional_digits);
        auto rhs_f = rhs.fractional * pow10(max_digits - rhs.n_fractional_digits);
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
        auto lhs_f = lhs.fractional * pow10(max_digits - lhs.n_fractional_digits);
        auto rhs_f = rhs.fractional * pow10(max_digits - rhs.n_fractional_digits);
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
        auto lhs_f = lhs.fractional * pow10(max_digits - lhs.n_fractional_digits);
        auto rhs_f = rhs.fractional * pow10(max_digits - rhs.n_fractional_digits);
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
        auto lhs_f = lhs.fractional * pow10(max_digits - lhs.n_fractional_digits);
        auto rhs_f = rhs.fractional * pow10(max_digits - rhs.n_fractional_digits);
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
    const unsigned long long BASE = 1'000'000'000ull; // base 10^9

    // split into base-B polynomial: N = (A3*B + A2) + (A1*B^-1 + A0*B^-2)
    // e.g., N = 12.000000000000000034 -> A3=0, A2=12, A1=0, A0=34
    unsigned long long A[4] = {0, 0, 0, 0};
    A[3] = rhs.integer / BASE; // 10^9 to 10^18
    A[2] = rhs.integer % BASE; // 10^0 to 10^9

    // scale fractional to exactly 18 digits before extracting chunks
    unsigned long long a_frac = rhs.fractional * pow10(18 - rhs.n_fractional_digits);
    A[1] = a_frac / BASE;    // 10^-1 to 10^-9
    A[0] = a_frac % BASE;    // 10^-10 to 10^-18

    unsigned long long B[4] = {0, 0, 0, 0};
    B[3] = lhs.integer / BASE;
    B[2] = lhs.integer % BASE;
    unsigned long long b_frac = lhs.fractional * pow10(18 - lhs.n_fractional_digits);
    B[1] = b_frac / BASE;
    B[0] = b_frac % BASE;

    // polynomial multiplication: R[k] = sum(A[i] * B[j]) for i+j = k
    // max product A[i]*B[j] < 10^18. max sum per R[k] <= 4 * 10^18.
    // i.e., strictly bounded within uint64 max (~1.84 * 10^19) => overflow impossible
    unsigned long long R[7] = {0, 0, 0, 0, 0, 0, 0};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            R[i + j] += A[i] * B[j];
        }
    }

    // resolve carries sequentially from smallest exponent (10^-36) to largest (10^18)
    for (int k = 0; k < 6; ++k) {
        unsigned long long carry = R[k] / BASE;
        R[k] %= BASE;
        R[k + 1] += carry;
    }

    // guard digit rounding: R[1] represents 10^-19 to 10^-27.
    // If R[1] >= B/2 (i.e., >= 0.5), round up the 18th fractional digit in R[2]
    if (R[1] >= BASE / 2) {
        ++R[2];
        // Propagate rounding carries upward (e.g., 0.999...9 -> 1.0)
        for (int k = 2; k < 6; ++k) {
            if (R[k] != BASE)
                break;
            R[k] = 0;
            ++R[k + 1];
        }
    }

    RealNumber res;
    res.is_negative = (rhs.is_negative != lhs.is_negative);

    // reconstruct integer (R6*B^2 + R5*B^1 + R4*B^0)
    // and fractional (R3*B^1 + R2*B^0, forming the 18 exact digits)
    res.integer = R[6] * BASE * BASE + R[5] * BASE + R[4];
    res.fractional = R[3] * BASE + R[2];
    res.n_fractional_digits = 18;

    res.clean_trailing_zeroes();

    if (res.integer == 0 && res.fractional == 0)
        res.is_negative = false;

    return res;
}

RealNumber operator/(const RealNumber& lhs, const RealNumber& rhs) {
    RealNumber reciprocal = rhs.reciprocal();

    // 1. initial estimate
    RealNumber Q = lhs * reciprocal;

    // 2. iterative Refinement (Goldschmidt division)
    // We recover the lost precision by calculating exactly how much we missed,
    // and adding the reciprocal of that remainder back into the quotient.
    for (int i = 0; i < 2; ++i) { // 2 iterations are presumed to be enough
        // calculate the exact missing remainder
        RealNumber R = lhs - (rhs * Q);
        if (R.integer == 0 && R.fractional == 0)
            break;

        // shift the missing precision back into the tail
        Q = Q + (R * reciprocal);
    }
    return Q.reduce_noise().clean_trailing_zeroes();
}
}
