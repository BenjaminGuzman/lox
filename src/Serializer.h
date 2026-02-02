#ifndef CODECRAFTERS_INTERPRETER_PRINTER_H
#define CODECRAFTERS_INTERPRETER_PRINTER_H
#include "Scanner.h"


class Serializer {
public:
    static std::string serialize(const std::string& filepath, const std::vector<Token>& tokens);
};


#endif //CODECRAFTERS_INTERPRETER_PRINTER_H