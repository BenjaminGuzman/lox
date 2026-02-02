#ifndef CODECRAFTERS_INTERPRETER_PRINTER_H
#define CODECRAFTERS_INTERPRETER_PRINTER_H
#include "Scanner.h"

class Serializer {
public:
    /**
     * Serializes and prints to stdout and stderr, the tokens and errors contained in the file, respectively.
     * @param filepath the filepath from which the tokens were obtained from
     * @param tokens the tokens contained in the file
     * @return the count of unrecognized tokens/errors in the file
     */
    static int serialize(const std::string& filepath, const std::vector<Token>& tokens);
};


#endif //CODECRAFTERS_INTERPRETER_PRINTER_H