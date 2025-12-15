#ifndef PARSER_CODEGEN_H
#define PARSER_CODEGEN_H

#include "lexer.h"
#include "codegen.h"
#include <vector>
#include <string>
#include <memory>

// Forward declaration
class DiagnosticEngine;

// Parser output options
struct ParserOptions {
    bool showParseTree;    // Show parsing process
    bool verbose;          // Extra verbose output
    
    ParserOptions() : showParseTree(false), verbose(false) {}
};

class ParserWithCodegen {
private:
    std::vector<Token> tokens;
    size_t position;
    int indentLevel;  // For parse tree indentation
    
    // Symbol table and code generator
    SymbolTable symbolTable;
    CodeGenerator codeGen;
    
    // Diagnostics
    DiagnosticEngine* diagnostics;
    bool hasErrorFlag;
    
    // Options
    ParserOptions options;
    
    // Token navigation
    Token currentToken();
    Token peekToken(int offset = 1);
    Token previousToken();
    void advance();
    bool match(TokenType type);
    bool check(TokenType type);
    void expect(TokenType type, const std::string& message);
    void expectSemicolon();  // Common case with better error message
    
    // Error handling
    void reportError(const std::string& message);
    void reportError(const std::string& message, const std::string& suggestion);
    void reportErrorAt(const Token& token, const std::string& message);
    void reportErrorAt(const Token& token, const std::string& message, const std::string& suggestion);
    void reportExpected(const std::string& expected);
    void reportExpectedAt(const Token& token, const std::string& expected);
    void synchronize();
    void synchronizeTo(std::initializer_list<TokenType> types);
    
    // Parse tree output
    void printIndent();
    void parseLog(const std::string& message);
    void parseLogEnter(const std::string& rule);
    void parseLogExit(const std::string& rule);
    
    // Grammar rules with code generation
    void parseProgram();
    void parseBlock();
    void parseCondecl();
    void parseVardecl();
    void parseProc();
    void parseBody();
    void parseStatement();
    void parseLexp();
    void parseExp();
    void parseTerm();
    void parseFactor();
    
public:
    ParserWithCodegen(const std::vector<Token>& tokenList, DiagnosticEngine* diag = nullptr);
    
    // Set options
    void setOptions(const ParserOptions& opts) { options = opts; }
    void setShowParseTree(bool show) { options.showParseTree = show; }
    void setVerbose(bool v) { options.verbose = v; }
    
    // Parse
    bool parse();
    bool hasErrors() const { return hasErrorFlag; }
    
    // Output
    void printSymbolTable() const { symbolTable.printSymbolTable(); }
    void printGeneratedCode() const { codeGen.printCode(); }
    const std::vector<Instruction>& getCode() const { return codeGen.getCode(); }
    const SymbolTable& getSymbolTable() const { return symbolTable; }
};

#endif // PARSER_CODEGEN_H
