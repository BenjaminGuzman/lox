#ifndef CODECRAFTERS_INTERPRETER_REALNUMBER_H
#define CODECRAFTERS_INTERPRETER_REALNUMBER_H
#include <string>

class RealNumber {
public:
    const unsigned long long integer{};
    const unsigned long long fractional{};
    const int n_fractional_digits{};
    const bool is_negative = false;
    [[nodiscard]] double to_double() const;
};
std::string to_string(const RealNumber& number);
std::string to_string(const RealNumber* number);

#endif //CODECRAFTERS_INTERPRETER_REALNUMBER_H