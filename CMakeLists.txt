cmake_minimum_required(VERSION 3.10)
project(PL0_Compiler VERSION 2.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Include header files directory
include_directories(${PROJECT_SOURCE_DIR}/include)

# Collect all source files (excluding main files)
set(COMMON_SOURCES
    ${PROJECT_SOURCE_DIR}/src/diagnostics.cpp
    ${PROJECT_SOURCE_DIR}/src/lexer.cpp
    ${PROJECT_SOURCE_DIR}/src/codegen.cpp
    ${PROJECT_SOURCE_DIR}/src/parser_codegen.cpp
    ${PROJECT_SOURCE_DIR}/src/interpreter.cpp
)

# Create the main compiler executable
add_executable(pl0
    ${PROJECT_SOURCE_DIR}/src/main.cpp
    ${COMMON_SOURCES}
)

# Set compile options
if(MSVC)
    target_compile_options(pl0 PRIVATE /W4)
else()
    target_compile_options(pl0 PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Installation rules (optional)
install(TARGETS pl0
    RUNTIME DESTINATION bin
)

# Print configuration info
message(STATUS "")
message(STATUS "════════════════════════════════════════════════════════")
message(STATUS "  PL/0 Compiler v2.0 Configuration")
message(STATUS "════════════════════════════════════════════════════════")
message(STATUS "  Build type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard:    ${CMAKE_CXX_STANDARD}")
message(STATUS "  Source dir:      ${PROJECT_SOURCE_DIR}")
message(STATUS "  Binary dir:      ${CMAKE_BINARY_DIR}")
message(STATUS "")
message(STATUS "  Features:")
message(STATUS "    - Clang-style error reporting with colors")
message(STATUS "    - Granular output control via CLI flags")
message(STATUS "    - Full PL/0 grammar support")
message(STATUS "    - Stack-based virtual machine interpreter")
message(STATUS "")
message(STATUS "  Usage:")
message(STATUS "    ./pl0 <file.pl0>              Compile and run")
message(STATUS "    ./pl0 <file.pl0> --tokens     Show token list")
message(STATUS "    ./pl0 <file.pl0> --ast        Show parse tree")
message(STATUS "    ./pl0 <file.pl0> --symbols    Show symbol table")
message(STATUS "    ./pl0 <file.pl0> --code       Show generated code")
message(STATUS "    ./pl0 <file.pl0> --all        Show everything")
message(STATUS "    ./pl0 <file.pl0> --no-run     Compile only")
message(STATUS "    ./pl0 <file.pl0> --debug      Debug execution")
message(STATUS "    ./pl0 --help                  Show all options")
message(STATUS "════════════════════════════════════════════════════════")
message(STATUS "")
