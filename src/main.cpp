#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>

#include "../include/parser.h"
#include "../include/scanner.h"
#include "../include/serializer.h"

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
        int n_errors_serialize = Serializer::serialize(ast);

        if (n_errors_serialize > 0 || n_errors_parse > 0)
            return 65;
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}