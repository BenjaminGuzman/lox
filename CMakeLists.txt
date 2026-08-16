cmake_minimum_required(VERSION 3.13)

project(codecrafters-interpreter)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)

option(TOKEN_PRETTY_PRINT "Enables pretty print of a serialized token" OFF)

# Add LLVM
if (EXISTS "/opt/homebrew/opt/llvm")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/llvm")
endif()
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs
        core
        support
        native
        irreader
        executionengine
        mcjit
        interpreter
)

# build the core lox library (parser, scanner, compiler)
set(CORE_SOURCES
        src/scanner/real_number.cpp
        src/scanner/scanner.cpp
        src/parser.cpp
        src/serializer.cpp)

add_library(lox STATIC ${CORE_SOURCES})
target_include_directories(lox PRIVATE include/)
target_link_libraries(lox ${llvm_libs})

if(TOKEN_PRETTY_PRINT)
    target_compile_definitions(lox PRIVATE TOKEN_PRETTY_PRINT)
endif()

# build a library including only the interpreter
set(INTERPRETER_SOURCES
        src/interpreter/interpreter.cpp
        src/interpreter/arithmetic_operators.cpp
        src/interpreter/state_mod_operators.cpp
        src/interpreter/control_statements.cpp
        src/interpreter/logical_operators.cpp
        src/interpreter/native_functions.cpp
        src/interpreter/user_function.cpp
        src/interpreter/comparison_operators.cpp)
add_library(lox-interpreter STATIC ${INTERPRETER_SOURCES})
target_include_directories(lox-interpreter PRIVATE include/)

# build a library including only the compiler
set(COMPILER_SOURCES
        src/compiler/compiler.cpp
        src/compiler/arithmetic_operators.cpp
        src/compiler/logical_operators.cpp
        src/compiler/comparison_operators.cpp
        src/compiler/state_mod_operators.cpp
        src/compiler/string.cpp
        src/compiler/control_statements.cpp)
add_library(lox-compiler STATIC ${COMPILER_SOURCES})
target_include_directories(lox-compiler PRIVATE include/)

# build the interpreter executable
add_executable(lox-exec
        src/main.cpp)
target_include_directories(lox-exec PRIVATE
        include/)
target_link_libraries(lox-exec lox lox-interpreter lox-compiler)


# Add tests
# Google Test
include(FetchContent)
FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(tests
        test/scanner_test.cpp
        test/real_number_test.cpp
        test/interpreter_test.cpp
        test/parser_test.cpp
        test/compiler_test.cpp)
target_include_directories(tests PRIVATE
        src/
        include/)
target_link_libraries(tests
        lox
        lox-interpreter
        lox-compiler
        GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(tests)

# This command runs the test executable immediately after it is built.
# If the tests fail, the build will be marked as failed.
#add_custom_command(TARGET tests POST_BUILD
#        COMMAND $<TARGET_FILE:tests>
#        COMMENT "Running tests after build..."
#)
