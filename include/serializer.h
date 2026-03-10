#ifndef CODECRAFTERS_INTERPRETER_PRINTER_H
#define CODECRAFTERS_INTERPRETER_PRINTER_H
#include "parser.h"
#include "scanner.h"

class Serializer {
public:
    /**
     * Serializes and prints to stdout and stderr the tokens and errors contained in the file, respectively.
     * @param scanner the scanner to use
     * @return the count of unrecognized tokens/errors in the file
     */
    static int serialize(lox::Scanner& scanner);

    /**
     * Serializes and prints to stdout and stderr the AST.
     * @param ast the AST
     * @return the count of errors
     */
    static int serialize(const lox::AST& ast);
};


#endif //CODECRAFTERS_INTERPRETER_PRINTER_H