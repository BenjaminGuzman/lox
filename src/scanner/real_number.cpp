#include <scanner/real_number.h>
#include <iomanip>
#include <sstream>

std::string to_string(const RealNumber& number) {
    if (number.n_fractional_digits == 0 && number.fractional == 0) {
        return std::to_string(number.integer) + ".0";
    }

    std::stringstream ss;
    ss << number.integer << "." << std::setw(number.n_fractional_digits) << std::setfill('0') << number.fractional;
    return ss.str();
}

std::string to_string(const RealNumber* number) {
    return to_string(*number);
}