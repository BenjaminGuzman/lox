cmake_minimum_required(VERSION 3.13)

project(codecrafters-interpreter)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard

option(TOKEN_PRETTY_PRINT "Enables pretty print of a serialized token" OFF)

# Add LLVM
# LLVM is likely not installed in codecrafters
#find_package(LLVM REQUIRED CONFIG)
#include_directories(${LLVM_INCLUDE_DIRS})
#link_directories(${LLVM_LIBRARY_DIRS})
#add_definitions(${LLVM_DEFINITIONS})
#llvm_map_components_to_libnames(llvm_libs
#        core
#        support
#        native
#        irreader
#)

# build the core lox library (parser, scanner, compiler)
set(CORE_SOURCES
        src/scanner/real_number.cpp
        src/scanner.cpp
        src/parser.cpp
        src/serializer.cpp)

add_library(lox STATIC ${CORE_SOURCES})
target_include_directories(lox PRIVATE
        include/)
#target_link_libraries(lox ${llvm_libs})

if(TOKEN_PRETTY_PRINT)
    target_compile_definitions(lox PRIVATE TOKEN_PRETTY_PRINT)
endif()

# build the interpreter executable
add_executable(interpreter
        src/main.cpp
        src/Interpreter.cpp)
target_include_directories(interpreter PRIVATE
        include/)
target_link_libraries(interpreter lox)


# Add tests
# Google Test
include(FetchContent)
FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(tests
        test/scanner_test.cpp
        test/parser_test.cpp)
target_include_directories(tests PRIVATE
        src/
        include/)
target_link_libraries(tests
        lox
        GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(tests)
