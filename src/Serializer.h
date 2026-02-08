#ifndef CODECRAFTERS_INTERPRETER_PRINTER_H
#define CODECRAFTERS_INTERPRETER_PRINTER_H
#include "scanner.h"

class Serializer {
public:
    /**
     * Serializes and prints to stdout and stderr, the tokens and errors contained in the file, respectively.
     * @param scanner the scanner to use
     * @return the count of unrecognized tokens/errors in the file
     */
    static int serialize(lox::Scanner& scanner);
};


#endif //CODECRAFTERS_INTERPRETER_PRINTER_H