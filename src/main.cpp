#include "lexer.h"
#include "parser_codegen.h"
#include "codegen.h"
#include "interpreter.h"
#include "diagnostics.h"
#include "banner.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <iomanip>

// Compiler Options Structure

struct CompilerOptions {
    std::string inputFile;
    
    // Output control flags
    bool showTokens;      // --tokens, -t
    bool showAst;         // --ast, -a (parse tree)
    bool showSymbols;     // --symbols, -s
    bool showCode;        // --code, -c
    
    // Execution control
    bool runProgram;      // --run (default), --no-run to disable
    bool debugExecution;  // --debug, -d
    
    // Display options
    bool useColors;       // --no-color to disable
    bool showSource;      // --source to show source code
    bool showHelp;        // --help, -h
    bool showVersion;     // --version, -v
    bool verbose;         // --verbose, -V
    
    // Quick modes (shortcuts)
    bool lexerOnly;       // --lexer-only (equivalent to --tokens --no-run)
    bool parseOnly;       // --parse-only (equivalent to --ast --no-run)
    bool compileOnly;     // --compile-only (equivalent to --no-run)
    
    CompilerOptions() 
        : showTokens(false), showAst(false), showSymbols(false), showCode(false),
          runProgram(true), debugExecution(false),
          useColors(true), showSource(false), showHelp(false), 
          showVersion(false), verbose(false),
          lexerOnly(false), parseOnly(false), compileOnly(false) {}
};

// Help and Version Functions

void printVersion() {
    std::cout << "PL/0 Compiler v1.0\n";
    std::cout << "A compiler and interpreter for the PL/0 programming language\n";
    std::cout << "Supports the full PL/0 grammar with Clang-style error reporting\n";
}

void printHelp(const char* programName, bool useColors) {
    const char* RESET = useColors ? "\033[0m" : "";
    const char* BOLD = useColors ? "\033[1m" : "";
    const char* GREEN = useColors ? "\033[32m" : "";
    const char* CYAN = useColors ? "\033[36m" : "";
    const char* YELLOW = useColors ? "\033[33m" : "";
    const char* BLUE = useColors ? "\033[34m" : "";
    const char* MAGENTA = useColors ? "\033[35m" : "";
    
    std::cout << BOLD << "USAGE:" << RESET << "\n";
    std::cout << "    " << programName << " <input_file> [options]\n\n";
    
    std::cout << BOLD << "DESCRIPTION:" << RESET << "\n";
    std::cout << "    Compiles and optionally executes PL/0 source files.\n";
    std::cout << "    The .pl0 extension is added automatically if not provided.\n\n";
    
    std::cout << BOLD << "OUTPUT OPTIONS:" << RESET << "\n";
    std::cout << "    " << GREEN << "-t, --tokens" << RESET << "      Show lexer output (token list)\n";
    std::cout << "    " << GREEN << "-a, --ast" << RESET << "         Show parser output (parse tree)\n";
    std::cout << "    " << GREEN << "-s, --symbols" << RESET << "     Show symbol table\n";
    std::cout << "    " << GREEN << "-c, --code" << RESET << "        Show generated code\n";
    std::cout << "    " << GREEN << "--source" << RESET << "          Show source code before compilation\n";
    std::cout << "    " << GREEN << "--all" << RESET << "             Show all intermediate outputs\n\n";
    
    std::cout << BOLD << "EXECUTION OPTIONS:" << RESET << "\n";
    std::cout << "    " << CYAN << "--run" << RESET << "             Compile and run (default)\n";
    std::cout << "    " << CYAN << "--no-run" << RESET << "          Compile only, do not execute\n";
    std::cout << "    " << CYAN << "-d, --debug" << RESET << "       Run with debug output (show execution steps)\n\n";
    
    std::cout << BOLD << "QUICK MODES:" << RESET << "\n";
    std::cout << "    " << YELLOW << "--lexer-only" << RESET << "      Run lexer only (same as --tokens --no-run)\n";
    std::cout << "    " << YELLOW << "--parse-only" << RESET << "      Run parser only (same as --ast --no-run)\n";
    std::cout << "    " << YELLOW << "--compile-only" << RESET << "    Compile only (same as --no-run)\n\n";
    
    std::cout << BOLD << "DISPLAY OPTIONS:" << RESET << "\n";
    std::cout << "    " << BLUE << "--no-color" << RESET << "        Disable colored output\n";
    std::cout << "    " << BLUE << "-V, --verbose" << RESET << "     Enable verbose output\n\n";
    
    std::cout << BOLD << "INFORMATION:" << RESET << "\n";
    std::cout << "    " << MAGENTA << "-h, --help" << RESET << "        Show this help message\n";
    std::cout << "    " << MAGENTA << "-v, --version" << RESET << "     Show version information\n\n";
    
    std::cout << BOLD << "EXAMPLES:" << RESET << "\n";
    std::cout << "    " << BOLD << programName << " test.pl0" << RESET << "\n";
    std::cout << "        Compile and run test.pl0\n\n";
    
    std::cout << "    " << BOLD << programName << " test --tokens --symbols" << RESET << "\n";
    std::cout << "        Compile test.pl0, show tokens and symbol table, then run\n\n";
    
    std::cout << "    " << BOLD << programName << " test.pl0 --lexer-only" << RESET << "\n";
    std::cout << "        Run lexer only and show token list\n\n";
    
    std::cout << "    " << BOLD << programName << " test.pl0 --all --no-run" << RESET << "\n";
    std::cout << "        Show all compilation phases without executing\n\n";
    
    std::cout << "    " << BOLD << programName << " test.pl0 --debug" << RESET << "\n";
    std::cout << "        Run with step-by-step execution trace\n\n";
    
    std::cout << "    " << BOLD << programName << " test.pl0 --code --no-color > output.txt" << RESET << "\n";
    std::cout << "        Save generated code to file (no ANSI codes)\n\n";
    
    std::cout << BOLD << "EXIT CODES:" << RESET << "\n";
    std::cout << "    0    Compilation (and execution) successful\n";
    std::cout << "    1    Compilation or execution failed\n";
}


// Argument Parsing Function

bool parseArguments(int argc, char* argv[], CompilerOptions& opts) {
    if (argc < 2) {
        return false;
    }
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        // Help flags (check first)
        if (arg == "-h" || arg == "--help") {
            opts.showHelp = true;
            return true;
        }
        
        // Version flag
        if (arg == "-v" || arg == "--version") {
            opts.showVersion = true;
            return true;
        }
        
        // Output control flags
        if (arg == "-t" || arg == "--tokens") {
            opts.showTokens = true;
        } else if (arg == "-a" || arg == "--ast") {
            opts.showAst = true;
        } else if (arg == "-s" || arg == "--symbols") {
            opts.showSymbols = true;
        } else if (arg == "-c" || arg == "--code") {
            opts.showCode = true;
        } else if (arg == "--source") {
            opts.showSource = true;
        } else if (arg == "--all") {
            opts.showTokens = true;
            opts.showAst = true;
            opts.showSymbols = true;
            opts.showCode = true;
            opts.showSource = true;
        }
        
        // Execution control
        else if (arg == "--run") {
            opts.runProgram = true;
        } else if (arg == "--no-run") {
            opts.runProgram = false;
        } else if (arg == "-d" || arg == "--debug") {
            opts.debugExecution = true;
        }
        
        // Quick modes
        else if (arg == "--lexer-only" || arg == "--lexer") {
            opts.lexerOnly = true;
            opts.showTokens = true;
            opts.runProgram = false;
        } else if (arg == "--parse-only" || arg == "--parser") {
            opts.parseOnly = true;
            opts.showAst = true;
            opts.runProgram = false;
        } else if (arg == "--compile-only" || arg == "--compile") {
            opts.compileOnly = true;
            opts.runProgram = false;
        } else if (arg == "--codegen") {
            // Legacy compatibility
            opts.showSymbols = true;
            opts.showCode = true;
            opts.runProgram = false;
        }
        
        // Display options
        else if (arg == "--no-color") {
            opts.useColors = false;
        } else if (arg == "-V" || arg == "--verbose") {
            opts.verbose = true;
        }
        
        // Unknown flag
        else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            std::cerr << "Use --help for usage information.\n";
            return false;
        }
        
        // Input file
        else {
            if (opts.inputFile.empty()) {
                opts.inputFile = arg;
            } else {
                std::cerr << "Multiple input files specified. Only one file is supported.\n";
                return false;
            }
        }
    }
    
    return true;
}

// File Finding Function

std::string findFile(const std::string& filename) {
    // Try the filename as-is first
    std::ifstream file(filename);
    if (file.is_open()) {
        file.close();
        return filename;
    }
    
    // Try adding .pl0 extension
    std::string withExt = filename + ".pl0";
    file.open(withExt);
    if (file.is_open()) {
        file.close();
        return withExt;
    }
    
    // Try in test/ directory
    std::string inTest = "test/" + filename;
    file.open(inTest);
    if (file.is_open()) {
        file.close();
        return inTest;
    }
    
    // Try in test/ directory with extension
    std::string inTestExt = "test/" + filename + ".pl0";
    file.open(inTestExt);
    if (file.is_open()) {
        file.close();
        return inTestExt;
    }
    
    // Try in ../test/ directory (for build folder)
    std::string inParentTest = "../test/" + filename;
    file.open(inParentTest);
    if (file.is_open()) {
        file.close();
        return inParentTest;
    }
    
    // Try in ../test/ directory with extension
    std::string inParentTestExt = "../test/" + filename + ".pl0";
    file.open(inParentTestExt);
    if (file.is_open()) {
        file.close();
        return inParentTestExt;
    }
    
    // Return original filename (will fail with appropriate error)
    return filename;
}

// Source Code Display Function

void displaySourceCode(const std::string& filepath, bool useColors) {
    const char* RESET = useColors ? "\033[0m" : "";
    const char* BOLD = useColors ? "\033[1m" : "";
    const char* CYAN = useColors ? "\033[36m" : "";
    const char* BLUE = useColors ? "\033[34m" : "";
    
    // Print header
    std::cout << BOLD << CYAN;
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║                   SOURCE CODE                        ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n";
    std::cout << RESET;
    
    // Read and display source file with line numbers
    std::ifstream displayFile(filepath);
    if (!displayFile.is_open()) {
        std::cerr << "Warning: Cannot open file for display: " << filepath << "\n";
        return;
    }
    
    std::string line;
    int lineNum = 1;
    while (std::getline(displayFile, line)) {
        // Remove trailing \r if present (Windows line endings)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        std::cout << BLUE << std::setw(4) << lineNum << " │ " << RESET << line << "\n";
        lineNum++;
    }
    std::cout << "\n";
}

// Main Compiler Driver Function

int main(int argc, char* argv[]) {
    //  Step 1: Parse Command Line Arguments 
    CompilerOptions opts;
    
    if (!parseArguments(argc, argv, opts)) {
        if (opts.inputFile.empty() && !opts.showHelp && !opts.showVersion) {
            printHelp(argv[0], true);
            return 1;
        }
        return 1;
    }
    
    // Step 2: Handle --help and --version
    if (opts.showHelp) {
        printHelp(argv[0], opts.useColors);
        return 0;
    }
    
    if (opts.showVersion) {
        printVersion();
        Banner::printLogo();
        Banner::printVersion();
        return 0;
    }
    
    // Step 3: Check Input File
    if (opts.inputFile.empty()) {
        std::cerr << "Error: no input file specified.\n";
        std::cerr << "Use --help for usage information.\n";
        return 1;
    }
    
    // Step 4: Setup Color Codes 
    const char* RESET = opts.useColors ? "\033[0m" : "";
    const char* BOLD = opts.useColors ? "\033[1m" : "";
    const char* RED = opts.useColors ? "\033[31m" : "";
    const char* GREEN = opts.useColors ? "\033[32m" : "";
    const char* BLUE = opts.useColors ? "\033[34m" : "";
    const char* CYAN = opts.useColors ? "\033[36m" : "";
    
    // Step 5: Initialize Diagnostic Engine 
    DiagnosticEngine diagnostics;
    diagnostics.setColors(opts.useColors);
    
    try {
        // Step 6: Find Source File 
        std::string filepath = findFile(opts.inputFile);
        
        // Step 7: Print Compiler Header (if verbose)
        if (opts.verbose) {
            std::cout << BOLD << CYAN;
            std::cout << "╔══════════════════════════════════════════════════════╗\n";
            std::cout << "║              PL/0 COMPILER v2.0                      ║\n";
            std::cout << "╚══════════════════════════════════════════════════════╝\n";
            std::cout << RESET;
            std::cout << "Input file: " << BOLD << filepath << RESET << "\n";
            
            // Show enabled options
            std::cout << "Options:    ";
            if (opts.showTokens) std::cout << "[tokens] ";
            if (opts.showAst) std::cout << "[ast] ";
            if (opts.showSymbols) std::cout << "[symbols] ";
            if (opts.showCode) std::cout << "[code] ";
            if (opts.runProgram) std::cout << "[run] ";
            if (opts.debugExecution) std::cout << "[debug] ";
            if (!opts.useColors) std::cout << "[no-color] ";
            std::cout << "\n\n";
        }
        
        // Step 8: Display Source Code (if requested) 
        if (opts.showSource) {
            displaySourceCode(filepath, opts.useColors);
        }
        
        
        // PHASE 1: LEXICAL ANALYSIS
        if (opts.verbose) {
            std::cout << BLUE << "[Phase 1]" << RESET << " Lexical Analysis...\n";
        }
        
        // Create lexer with buffer-based input (efficient memory usage)
        // The lexer uses double-buffer scheme with sentinels for large file support
        Lexer lexer(&diagnostics);
        lexer.initFromFile(filepath);
        
        // Perform lexical analysis
        std::vector<Token> tokens = lexer.tokenize();
        
        // Get source lines from lexer for diagnostic system
        // This allows error messages to show the relevant source code
        const auto& sourceLines = lexer.getSourceLines();
        std::string reconstructedSource;
        for (size_t i = 0; i < sourceLines.size(); i++) {
            reconstructedSource += sourceLines[i];
            if (i < sourceLines.size() - 1) {
                reconstructedSource += "\n";
            }
        }
        diagnostics.setSource(reconstructedSource, filepath);
        
        // Show tokens if requested
        if (opts.showTokens) {
            lexer.printTokens(tokens);
        }
        
        // Check for lexical errors
        if (lexer.hasErrors()) {
            if (opts.lexerOnly) {
                std::cout << RED << BOLD << "[Error] " << RESET 
                          << "Lexical analysis failed with errors.\n";
            }
            diagnostics.printSummary();
            return 1;
        }
        
        if (opts.verbose) {
            std::cout << GREEN << "[Bingo] " << RESET << " Lexical analysis completed.\n";
        }
        
        // Stop here if lexer-only mode
        if (opts.lexerOnly) {
            std::cout << GREEN << BOLD << "[Bingo] " << RESET 
                      << "Lexical analysis completed successfully.\n";
            return 0;
        }
        
        // PHASE 2: SYNTAX ANALYSIS & CODE GENERATION
        if (opts.verbose) {
            std::cout << BLUE << "[Phase 2]" << RESET << " Syntax Analysis & Code Generation...\n";
        }
        
        // Create parser with code generation
        ParserWithCodegen parser(tokens, &diagnostics);
        
        // Configure parser options
        ParserOptions parserOpts;
        parserOpts.showParseTree = opts.showAst;
        parserOpts.verbose = opts.verbose;
        parser.setOptions(parserOpts);
        
        // Perform parsing and code generation
        bool parseSuccess = parser.parse();
        
        // Check for parse errors
        if (!parseSuccess) {
            if (opts.parseOnly) {
                std::cout << RED << BOLD << "[Error] " << RESET 
                          << "Syntax analysis failed with errors.\n";
            }
            diagnostics.printSummary();
            return 1;
        }
        
        if (opts.verbose) {
            std::cout << GREEN << "[Error] " << RESET << " Syntax analysis completed.\n";
        }
        
        // Stop here if parse-only mode
        if (opts.parseOnly) {
            std::cout << GREEN << BOLD << "[Bingo] " << RESET 
                      << "Syntax analysis completed successfully.\n";
            return 0;
        }
        
        // PHASE 3: OUTPUT SYMBOL TABLE (if requested)
        if (opts.showSymbols) {
            parser.printSymbolTable();
        }
        
        // PHASE 4: OUTPUT GENERATED CODE (if requested)
        if (opts.showCode) {
            parser.printGeneratedCode();
        }
        
        // Compilation success message
        if (opts.verbose || opts.compileOnly) {
            std::cout << GREEN << BOLD << "✓ " << RESET 
                      << "Compilation completed successfully.\n";
        }
        
        // Stop here if not running
        if (!opts.runProgram) {
            diagnostics.printSummary();
            return 0;
        }
        
        // PHASE 5: PROGRAM EXECUTION
        if (opts.verbose) {
            std::cout << "\n" << BLUE << "[Phase 3]" << RESET << " Execution...\n";
            std::cout << "════════════════════════════════════════════════════════\n";
        } else {
            // Simple separator before program output
            std::cout << "\n";
        }
        
        // Create interpreter and load generated code
        Interpreter interpreter(opts.debugExecution);
        interpreter.loadCode(parser.getCode());
        
        // Execute the program
        interpreter.run();
        
        if (opts.verbose) {
            std::cout << GREEN << BOLD << "[Bingo] " << RESET 
                      << "Execution completed.\n";
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        // Handle any exceptions (file not found, etc.)
        std::cerr << RED << BOLD << "error: " << RESET << e.what() << "\n";
        return 1;
    }
}