#include <scanner/real_number.h>
#include <iomanip>
#include <sstream>
#include <cmath>

std::string to_string(const RealNumber& number) {
    if (number.n_fractional_digits == 0 || number.fractional == 0) {
        return std::to_string(number.integer);
    }

    // {:0{}} means: pad with '0', and take the width from the next argument
    return std::format("{}.{:0{}}", number.integer, number.fractional, number.n_fractional_digits);
}

std::string to_string(const RealNumber* number) {
    return to_string(*number);
}

std::string to_string_as_expected_by_evaluation_system(const RealNumber& number) {
    if (number.n_fractional_digits == 0 || number.fractional == 0) {
        return std::to_string(number.integer) + ".0";
    }

    std::stringstream ss;
    ss << number.integer << "." << std::setw(number.n_fractional_digits) << std::setfill('0') << number.fractional;
    return ss.str();
}

std::string to_string_as_expected_by_evaluation_system(const RealNumber* number) {
    return to_string_as_expected_by_evaluation_system(*number);
}

[[nodiscard]] double RealNumber::to_double() const {
    if (n_fractional_digits == 0) {
        return static_cast<double>(integer);
    }
    return static_cast<double>(integer) + (static_cast<double>(fractional) / std::pow(10.0, n_fractional_digits));
}

RealNumber operator+(const RealNumber& lhs, const RealNumber& rhs) {
    int max_digits = std::max(lhs.n_fractional_digits, rhs.n_fractional_digits);

    unsigned long long lhs_frac = lhs.fractional * static_cast<unsigned long long>(std::pow(10, max_digits - lhs.n_fractional_digits));
    unsigned long long rhs_frac = rhs.fractional * static_cast<unsigned long long>(std::pow(10, max_digits - rhs.n_fractional_digits));

    unsigned long long total_frac = lhs_frac + rhs_frac;
    unsigned long long total_int = lhs.integer + rhs.integer;

    auto divisor = static_cast<unsigned long long>(std::pow(10, max_digits));
    if (total_frac >= divisor) {
        total_int += total_frac / divisor;
        total_frac %= divisor;
    }

    return RealNumber{total_int, total_frac, max_digits};
}

std::string operator+(const std::string& lhs, const RealNumber& rhs) {
    return lhs + to_string(rhs);
}

std::string operator+(const RealNumber& lhs, const std::string& rhs) {
    return to_string(lhs) + rhs;
}

std::ostream& operator<<(std::ostream& os, const RealNumber& number) {
    os << to_string(number);
    return os;
}
