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
#include "internal/utils.h"

/**
 * Validates the arguments are 3: the program name, the command, and the file to be used by the command
 * e.g., "program tokenize file-to-tokenize.lox"
 * @return the third argument, which must be a filename
 */
std::string validate_args_and_get_file(int argc, const char** argv, const std::string& command) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " " << command << " <filename>" << std::endl;
        return "";
    }

    return argv[2];
}

[[nodiscard]] int tokenize(int argc, const char** argv) {
    std::string filepath = validate_args_and_get_file(argc, argv, "tokenize");
    if (filepath.empty())
        return 1;

    auto abs_filepath = std::filesystem::canonical(filepath);

    lox::Scanner scanner(abs_filepath);
    int n_unrecognized = Serializer::serialize(scanner);
    if (n_unrecognized > 0)
        return 65;

    return 0;
}

[[nodiscard]] int parse(int argc, const char** argv) {
    std::string filepath = validate_args_and_get_file(argc, argv, "parse");
    if (filepath.empty())
        return 1;

    auto abs_filepath = std::filesystem::canonical(filepath);

    lox::Scanner scanner(abs_filepath);
    lox::AST ast(scanner, false);
    int n_errors_parse = ast.build();
    if (n_errors_parse >  0)
        return 65;

    int n_errors_serialize = Serializer::serialize(ast);
    if (n_errors_serialize > 0)
        return 65;

    return 0;
}

[[nodiscard]] int evaluate(int argc, const char** argv) {
    std::string filepath = validate_args_and_get_file(argc, argv, "evaluate");
    if (filepath.empty())
        return 1;

    auto abs_filepath = std::filesystem::canonical(filepath);

    lox::Scanner scanner(abs_filepath);
    lox::AST ast(scanner, false);
    int n_errors_parse = ast.build();
    if (n_errors_parse >  0)
        return 65;

    int n_errors_eval = 0;
    lox::registerErrorF logErrors = [&n_errors_eval, &abs_filepath](std::string message, const Token& token) {
        std::cerr << lox::error_in_file_prefix(abs_filepath, token.line, token.col) << message << std::endl;
        ++n_errors_eval;
    };
    lox::Interpreter interpreter(logErrors);
    interpreter.strict_output_mode = false;
    auto _ = interpreter.execute(ast.root);

    if (n_errors_eval)
        return 70;

    return 0;
}

[[nodiscard]] int run(int argc, const char** argv) {
    std::string filepath = validate_args_and_get_file(argc, argv, "run");
    if (filepath.empty())
        return 1;

    auto abs_filepath = std::filesystem::canonical(filepath);

    lox::Scanner scanner(abs_filepath);
    lox::AST ast(scanner, false);
    int n_errors_parse = ast.build();

    int n_errors_eval = 0;
    lox::registerErrorF logErrors = [&n_errors_eval, &abs_filepath](std::string message, const Token& token) {
        std::cerr << lox::error_in_file_prefix(abs_filepath, token.line, token.col) << message << std::endl;
        ++n_errors_eval;
        exit(70);
    };
    lox::Interpreter interpreter(logErrors);
    interpreter.strict_output_mode = true; // only print if print command was given
    auto _ = interpreter.execute(ast.root);

    if (n_errors_parse)
        return 65;

    if (n_errors_eval)
        return 70;

    return 0;
}

const std::unordered_map<std::string, std::function<int(int, const char**)>> COMMANDS = {
    {"tokenize", tokenize},
    {"parse", parse},
    {"evaluate", evaluate},
    {"run", run}
};

int main(int argc, const char** argv) {
    // Disable output buffering
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cerr << "Logs from your program will appear here!" << std::endl;

    const std::string command = argv[1];
    if (!COMMANDS.contains(command)) {
        std::cerr << "Unknown command: " << command << std::endl;
        return 1;
    }

    return COMMANDS.at(command)(argc, argv);
}