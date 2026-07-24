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
#include "compiler.h"
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

    if (n_errors_parse)
        return 65;

    int n_errors_eval = 0;
    lox::registerErrorF logErrors = [&n_errors_eval, &abs_filepath](std::string message, const Token& token) {
        if (message == "Apparently in lox is illegal to declare a variable"
            || message == "Invalid for that should be valid")
            exit(65);

        std::cerr << lox::error_in_file_prefix(abs_filepath, token.line, token.col) << message << std::endl;
        ++n_errors_eval;
        exit(70);
    };
    lox::Interpreter interpreter(logErrors);
    interpreter.strict_output_mode = true; // only print if print command was given
    auto _ = interpreter.execute(ast.root);

    if (n_errors_eval)
        return 70;

    return 0;
}

/**
 * REPL (read-eval-print loop)
 * @return exit status code
 */
[[nodiscard]] int repl(int argc, const char** argv) {
    std::cout << "This is lox's REPL. Enter any lox statement and it will be executed after pressing Enter.\n"
            << "Exit using .exit/.quit" << std::endl;

    lox::registerErrorF logErrors = [](std::string message, const Token& token) {
        std::cerr << message << std::endl;
    };
    lox::Interpreter interpreter(logErrors);
    interpreter.strict_output_mode = false;

    lox::Scanner scanner("", false);
    lox::AST ast(scanner, false);

    std::string line;
    while (true) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line))
            break; // Exit on Ctrl-D
        if (line == ".exit" || line == ".quit")
            break; // Exit on .exit/.quit
        if (line.empty())
            continue; // Continue on empty line

        scanner.feed(line + "\n");
        const auto& stmt = ast.build_next_statement();
        if (stmt == ast.root)
            continue; // Parse error or no statements
        
        underlying_t val = interpreter.execute(stmt);
        if (stmt->token.type == PRINT)
            continue; // avoid printing the return value (nullptr) of print function

        // print the returned value
        std::visit([]<typename E>(E&& v) {
            if constexpr (std::is_same_v<std::decay_t<E>, std::monostate>)
                return;

            std::cout << "\033[2mReturned: \033[0m";
            if constexpr (std::is_same_v<std::decay_t<E>, std::nullptr_t>)
                std::cout << "nil";
            else if constexpr (!std::is_same_v<std::decay_t<E>, std::monostate>)
                std::cout << v;
            std::cout << std::endl;
        }, val);
    }

    std::cout << "Bye! ✌️" << std::endl;
    return 0;
}

[[nodiscard]] int compile(int argc, const char** argv) {
    std::string filepath = validate_args_and_get_file(argc, argv, "compile");
    if (filepath.empty())
        return 1;

    auto abs_filepath = std::filesystem::canonical(filepath);

    lox::Scanner scanner(abs_filepath);
    lox::AST ast(scanner, false);
    int n_errors_parse = ast.build();
    if (n_errors_parse >  0)
        return 65;

    lox::Compiler compiler;
    std::cout << "Compilation exit: " << compiler.compile(ast.root) << std::endl;

    return 0;
}

const std::unordered_map<std::string, std::function<int(int, const char**)>> COMMANDS = {
    {"tokenize", tokenize},
    {"parse", parse},
    {"evaluate", evaluate},
    {"run", run},
    {"compile", compile}
};

int main(int argc, const char** argv) {
    if (argc == 1) // just the program name, i.e., no args
        return repl(argc, argv);

    const std::string command = argv[1];
    if (!COMMANDS.contains(command)) {
        std::cerr << "Unknown command: " << command << std::endl;
        return 1;
    }

    return COMMANDS.at(command)(argc, argv);
}