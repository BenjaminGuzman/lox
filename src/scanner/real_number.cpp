#include <scanner/real_number.h>

std::string to_string(const RealNumber& number) {
    return std::to_string(number.integer) + "." + std::to_string(number.fractional);
}

std::string to_string(const RealNumber* number) {
    return to_string(*number);
}