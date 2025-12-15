#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <unordered_set>
#include <fstream>
#include <cstring>

// Forward declaration
class DiagnosticEngine;

enum class TokenType {
    // Keywords
    PROGRAM, CONST, VAR, PROCEDURE, BEGIN, END,
    IF, THEN, ELSE, WHILE, DO, CALL, READ, WRITE, ODD,
    
    // Identifiers and integers
    IDENTIFIER, INTEGER,
    
    // Operators
    PLUS, MINUS, MULTIPLY, DIVIDE,
    ASSIGN,        // :=
    EQ, NEQ, LT, LEQ, GT, GEQ,  // =, <>, <, <=, >, >=
    
    // Delimiters
    LPAREN, RPAREN, COMMA, SEMICOLON,
    
    // Special
    END_OF_FILE,
    ERROR
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    int length;  // Length of the token (for error highlighting)
    
    Token() : type(TokenType::ERROR), value(""), line(0), column(0), length(0) {}
    
    Token(TokenType t, const std::string& v, int l, int c)
        : type(t), value(v), line(l), column(c), length(static_cast<int>(v.length())) {}
    
    Token(TokenType t, const std::string& v, int l, int c, int len)
        : type(t), value(v), line(l), column(c), length(len) {}
};

// ============================================================
// Input Buffer Class - Double Buffer with Sentinels
// ============================================================
// 
// Implementation of the two-buffer scheme with sentinels
// (Reference: "Compilers: Principles, Techniques, and Tools" - Dragon Book)
// 
// Memory Layout:
// +------------------+---+------------------+---+
// |    Buffer 1      |EOF|    Buffer 2      |EOF|
// +------------------+---+------------------+---+
// |<-- BUFFER_SIZE -->|   |<-- BUFFER_SIZE -->|
// 
// Two Pointers:
// - lexemeBegin: points to the beginning of the current lexeme
// - forward: scans ahead to find the end of the lexeme
//
// Advantages:
// - Constant memory usage O(1) regardless of file size
// - Reduced system calls by reading in blocks
// - Sentinel eliminates separate end-of-buffer check
//
class InputBuffer {
public:
    // Buffer size (typically equals disk block size for optimal I/O)
    static const size_t BUFFER_SIZE = 4096;
    
    // Sentinel character (special value that cannot appear in source program)
    static const char SENTINEL = '\0';
    
private:
    std::istream* input;           // Input stream (can be file stream or string stream)
    bool ownsStream;               // Whether this object owns the input stream
    
    // Double buffer (each buffer has one sentinel position at the end)
    char buffer[2 * BUFFER_SIZE + 2];  // +2 for two sentinels
    
    // Pointers
    char* lexemeBegin;             // Lexeme begin pointer
    char* forward;                 // Forward scanning pointer
    
    // Buffer state
    char* buffer1;                 // Start position of first buffer
    char* buffer2;                 // Start position of second buffer
    char* sentinel1;               // Position of first sentinel
    char* sentinel2;               // Position of second sentinel
    
    bool eof1;                     // Whether buffer1 contains EOF
    bool eof2;                     // Whether buffer2 contains EOF
    bool inputExhausted;           // Whether input has been completely read
    
    // Position tracking
    int currentLine;
    int currentColumn;
    int lexemeStartLine;
    int lexemeStartColumn;
    
    // Source line cache for diagnostic system
    std::vector<std::string> sourceLines;
    std::string currentLineBuffer;
    
    // Helper methods
    void loadBuffer1();
    void loadBuffer2();
    void trackNewline(char c);
    
public:
    InputBuffer();
    ~InputBuffer();
    
    // Initialization methods
    void initFromFile(const std::string& filename);
    void initFromString(const std::string& source);
    void initFromStream(std::istream* stream, bool takeOwnership = false);
    
    // Core operations
    char currentChar();            // Get current character (pointed by forward)
    char peekChar(int offset = 1); // Look ahead by offset characters
    void advance();                // Move forward pointer one step ahead
    void retract();                // Move forward pointer one step back (for backtracking)
    
    // Lexeme operations
    void markLexemeStart();        // Mark the start position of current lexeme
    std::string getLexeme();       // Get current lexeme (from lexemeBegin to forward-1)
    void skipLexeme();             // Skip current lexeme, move lexemeBegin to forward
    
    // Position information
    int getLine() const { return currentLine; }
    int getColumn() const { return currentColumn; }
    int getLexemeStartLine() const { return lexemeStartLine; }
    int getLexemeStartColumn() const { return lexemeStartColumn; }
    
    // Check if end of file is reached
    bool isEOF() const;
    
    // Get source lines (for error reporting)
    const std::vector<std::string>& getSourceLines() const { return sourceLines; }
    std::string getSourceLine(int lineNum) const;
    
    // Reset buffer state
    void reset();
};

// ============================================================
// Lexer Class
// ============================================================
class Lexer {
private:
    InputBuffer buffer;            // Input buffer
    
    std::unordered_set<std::string> keywords;
    std::vector<Token> tokens;
    
    // Diagnostic engine reference
    DiagnosticEngine* diagnostics;
    bool hasErrorFlag;
    
    // Internal helper methods
    char currentChar();
    char peekChar();
    void advance();
    void skipWhitespace();
    
    Token readIdentifierOrKeyword();
    Token readNumber();
    Token readOperator();
    
    void reportError(const std::string& message);
    void reportError(const std::string& message, const std::string& suggestion);
    void reportErrorWithLength(const std::string& message, int length);
    
public:
    // Constructor
    Lexer(DiagnosticEngine* diag = nullptr);
    
    // Initialization methods (replace original constructor parameter)
    void initFromFile(const std::string& filename);
    void initFromString(const std::string& source);
    
    // Constructor compatible with old interface
    Lexer(const std::string& sourceCode, DiagnosticEngine* diag);
    
    std::vector<Token> tokenize();
    bool hasErrors() const { return hasErrorFlag; }
    
    // Get source lines (for diagnostic system)
    const std::vector<std::string>& getSourceLines() const { 
        return buffer.getSourceLines(); 
    }
    
    // Static utilities
    static std::string tokenTypeToString(TokenType type);
    static std::string tokenTypeToReadable(TokenType type);
    void printTokens(const std::vector<Token>& tokens) const;
    void printTokens() const { printTokens(tokens); }
};

#endif // LEXER_H