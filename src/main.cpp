#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>

#include "interpreter.h"
#include "parser.h"
#include "scanner.h"
#include "serializer.h"

int main(int argc, char *argv[]) {
    // Disable output buffering
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cerr << "Logs from your program will appear here!" << std::endl;

    if (argc < 3) {
        std::cerr << "Usage: ./your_program tokenize <filename>" << std::endl;
        return 1;
    }

    const std::string command = argv[1];
    if (command == "tokenize") {
        std::string filepath = argv[2];
        auto abs_filepath = std::filesystem::canonical(filepath);

        lox::Scanner scanner(abs_filepath);
        int n_unrecognized = Serializer::serialize(scanner);
        if (n_unrecognized > 0)
            return 65;
    } else if (command == "parse") {
        std::string filepath = argv[2];
        auto abs_filepath = std::filesystem::canonical(filepath);

        lox::Scanner scanner(abs_filepath);
        lox::AST ast(scanner, false);
        int n_errors_parse = ast.build();
        if (n_errors_parse >  0)
            return 65;

        int n_errors_serialize = Serializer::serialize(ast);
        if (n_errors_serialize > 0)
            return 65;
    } else if (command == "evaluate") {
        std::string filepath = argv[2];
        auto abs_filepath = std::filesystem::canonical(filepath);

        lox::Scanner scanner(abs_filepath);
        lox::AST ast(scanner, false);
        int n_errors_parse = ast.build();
        if (n_errors_parse >  0)
            return 65;

        int n_errors_eval = 0;
        lox::registerErrorF logErrors = [&n_errors_eval](std::string message, const Token& token) {
            std::cerr << message << std::endl;
            ++n_errors_eval;
        };
        lox::Interpreter interpreter(logErrors);
        interpreter.execute(ast.root);

        if (n_errors_eval)
            return 70;
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}