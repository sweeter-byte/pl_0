#include "lexer.h"
#include "diagnostics.h"
#include <cctype>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <stdexcept>

// ============================================================
// InputBuffer Implementation
// ============================================================

InputBuffer::InputBuffer() 
    : input(nullptr), ownsStream(false),
      lexemeBegin(nullptr), forward(nullptr),
      buffer1(nullptr), buffer2(nullptr),
      sentinel1(nullptr), sentinel2(nullptr),
      eof1(false), eof2(false), inputExhausted(false),
      currentLine(1), currentColumn(1),
      lexemeStartLine(1), lexemeStartColumn(1) {
    
    // Initialize buffer layout
    // +------------------+---+------------------+---+
    // |    Buffer 1      |EOF|    Buffer 2      |EOF|
    // +------------------+---+------------------+---+
    buffer1 = buffer;
    sentinel1 = buffer + BUFFER_SIZE;
    buffer2 = buffer + BUFFER_SIZE + 1;
    sentinel2 = buffer + 2 * BUFFER_SIZE + 1;
    
    // Initialize sentinels
    *sentinel1 = SENTINEL;
    *sentinel2 = SENTINEL;
    
    // Clear buffer
    std::memset(buffer, 0, sizeof(buffer));
}

InputBuffer::~InputBuffer() {
    if (ownsStream && input) {
        delete input;
    }
}

void InputBuffer::initFromFile(const std::string& filename) {
    // Clean up old stream
    if (ownsStream && input) {
        delete input;
    }
    
    // Create file stream
    std::ifstream* fileStream = new std::ifstream(filename, std::ios::binary);
    if (!fileStream->is_open()) {
        delete fileStream;
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    input = fileStream;
    ownsStream = true;
    
    // Reset state
    reset();
    
    // Load first buffer
    loadBuffer1();
    
    // Initialize pointers
    lexemeBegin = buffer1;
    forward = buffer1;
}

void InputBuffer::initFromString(const std::string& source) {
    // Clean up old stream
    if (ownsStream && input) {
        delete input;
    }
    
    // Create string stream
    std::istringstream* stringStream = new std::istringstream(source);
    input = stringStream;
    ownsStream = true;
    
    // Cache source lines for error reporting
    sourceLines.clear();
    std::istringstream lineStream(source);
    std::string line;
    while (std::getline(lineStream, line)) {
        // Remove trailing \r (Windows line endings)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        sourceLines.push_back(line);
    }
    if (sourceLines.empty()) {
        sourceLines.push_back("");
    }
    
    // Reset state
    reset();
    
    // Load first buffer
    loadBuffer1();
    
    // Initialize pointers
    lexemeBegin = buffer1;
    forward = buffer1;
}

void InputBuffer::initFromStream(std::istream* stream, bool takeOwnership) {
    // Clean up old stream
    if (ownsStream && input) {
        delete input;
    }
    
    input = stream;
    ownsStream = takeOwnership;
    
    // Reset state
    reset();
    
    // Load first buffer
    loadBuffer1();
    
    // Initialize pointers
    lexemeBegin = buffer1;
    forward = buffer1;
}

void InputBuffer::reset() {
    currentLine = 1;
    currentColumn = 1;
    lexemeStartLine = 1;
    lexemeStartColumn = 1;
    eof1 = false;
    eof2 = false;
    inputExhausted = false;
    currentLineBuffer.clear();
    
    // Reset sentinels
    *sentinel1 = SENTINEL;
    *sentinel2 = SENTINEL;
}

void InputBuffer::loadBuffer1() {
    if (inputExhausted) {
        eof1 = true;
        buffer1[0] = SENTINEL;
        return;
    }
    
    // Read BUFFER_SIZE characters into buffer1
    input->read(buffer1, BUFFER_SIZE);
    std::streamsize bytesRead = input->gcount();
    
    if (bytesRead < static_cast<std::streamsize>(BUFFER_SIZE)) {
        // Input is less than one buffer, mark EOF
        buffer1[bytesRead] = SENTINEL;
        eof1 = true;
        inputExhausted = true;
    } else {
        // Buffer is full, restore sentinel
        *sentinel1 = SENTINEL;
        eof1 = false;
    }
}

void InputBuffer::loadBuffer2() {
    if (inputExhausted) {
        eof2 = true;
        buffer2[0] = SENTINEL;
        return;
    }
    
    // Read BUFFER_SIZE characters into buffer2
    input->read(buffer2, BUFFER_SIZE);
    std::streamsize bytesRead = input->gcount();
    
    if (bytesRead < static_cast<std::streamsize>(BUFFER_SIZE)) {
        // Input is less than one buffer, mark EOF
        buffer2[bytesRead] = SENTINEL;
        eof2 = true;
        inputExhausted = true;
    } else {
        // Buffer is full, restore sentinel
        *sentinel2 = SENTINEL;
        eof2 = false;
    }
}

void InputBuffer::trackNewline(char c) {
    if (c == '\n') {
        // Save current line to source line cache (for error reporting when reading from file)
        if (sourceLines.size() < static_cast<size_t>(currentLine)) {
            sourceLines.push_back(currentLineBuffer);
        }
        currentLineBuffer.clear();
        currentLine++;
        currentColumn = 1;
    } else if (c != '\r') {
        currentLineBuffer += c;
        currentColumn++;
    }
}

char InputBuffer::currentChar() {
    if (forward == nullptr) return SENTINEL;
    
    char c = *forward;
    
    // Check if we hit a sentinel
    if (c == SENTINEL) {
        if (forward == sentinel1) {
            // Reached end of buffer1
            if (eof1) {
                return SENTINEL;  // This is the real EOF
            }
            // Load buffer2 and switch to it
            loadBuffer2();
            forward = buffer2;
            return *forward;
        } else if (forward == sentinel2) {
            // Reached end of buffer2
            if (eof2) {
                return SENTINEL;  // This is the real EOF
            }
            // Load buffer1 and switch to it
            loadBuffer1();
            forward = buffer1;
            return *forward;
        } else {
            // Hit sentinel in the middle of buffer, this is the real EOF
            return SENTINEL;
        }
    }
    
    return c;
}

char InputBuffer::peekChar(int offset) {
    // Save current state
    char* savedForward = forward;
    int savedLine = currentLine;
    int savedColumn = currentColumn;
    
    // Move forward by offset steps
    char result = SENTINEL;
    for (int i = 0; i < offset; i++) {
        advance();
        if (isEOF()) {
            result = SENTINEL;
            break;
        }
        if (i == offset - 1) {
            result = currentChar();
        }
    }
    
    // Restore state
    forward = savedForward;
    currentLine = savedLine;
    currentColumn = savedColumn;
    
    return result;
}

void InputBuffer::advance() {
    if (forward == nullptr) return;
    
    char c = *forward;
    
    // Track position
    trackNewline(c);
    
    // Move forward pointer
    forward++;
    
    // Check if we hit a sentinel (need to switch buffer)
    if (*forward == SENTINEL) {
        if (forward == sentinel1) {
            if (!eof1) {
                loadBuffer2();
                forward = buffer2;
            }
            // If eof1 is true, stay at sentinel position
        } else if (forward == sentinel2) {
            if (!eof2) {
                loadBuffer1();
                forward = buffer1;
            }
            // If eof2 is true, stay at sentinel position
        }
        // Other cases: hit sentinel in the middle of buffer, it's the real EOF
    }
}

void InputBuffer::retract() {
    if (forward == nullptr) return;
    
    // Retract forward pointer
    if (forward == buffer1) {
        // At the beginning of buffer1, need to go back to end of buffer2
        // Note: this situation rarely happens in normal usage
        forward = sentinel2 - 1;
    } else if (forward == buffer2) {
        // At the beginning of buffer2, need to go back to end of buffer1
        forward = sentinel1 - 1;
    } else {
        forward--;
    }
    
    // Retract column number (simplified, does not handle newline retraction)
    if (currentColumn > 1) {
        currentColumn--;
    }
}

void InputBuffer::markLexemeStart() {
    lexemeBegin = forward;
    lexemeStartLine = currentLine;
    lexemeStartColumn = currentColumn;
}

std::string InputBuffer::getLexeme() {
    std::string lexeme;
    
    char* p = lexemeBegin;
    while (p != forward) {
        if (*p != SENTINEL && *p != '\0') {
            lexeme += *p;
        }
        
        // Move pointer, handle buffer boundary
        p++;
        if (p == sentinel1 + 1) {
            p = buffer2;
        } else if (p == sentinel2 + 1) {
            p = buffer1;
        }
    }
    
    return lexeme;
}

void InputBuffer::skipLexeme() {
    lexemeBegin = forward;
    lexemeStartLine = currentLine;
    lexemeStartColumn = currentColumn;
}

bool InputBuffer::isEOF() const {
    if (forward == nullptr) return true;
    
    char c = *forward;
    if (c == SENTINEL) {
        // Check if it's the real EOF or just buffer boundary
        if (forward == sentinel1 && eof1) return true;
        if (forward == sentinel2 && eof2) return true;
        if (forward != sentinel1 && forward != sentinel2) return true;
    }
    return false;
}

std::string InputBuffer::getSourceLine(int lineNum) const {
    if (lineNum >= 1 && lineNum <= static_cast<int>(sourceLines.size())) {
        return sourceLines[lineNum - 1];
    }
    return "";
}

// ============================================================
// Lexer Implementation
// ============================================================

Lexer::Lexer(DiagnosticEngine* diag) 
    : diagnostics(diag), hasErrorFlag(false) {
    // Initialize keywords
    keywords = {
        "program", "const", "var", "procedure", "begin", "end",
        "if", "then", "else", "while", "do", "call", "read", "write", "odd"
    };
}

// Constructor compatible with old interface
Lexer::Lexer(const std::string& sourceCode, DiagnosticEngine* diag) 
    : diagnostics(diag), hasErrorFlag(false) {
    // Initialize keywords
    keywords = {
        "program", "const", "var", "procedure", "begin", "end",
        "if", "then", "else", "while", "do", "call", "read", "write", "odd"
    };
    
    // Initialize buffer from string
    buffer.initFromString(sourceCode);
}

void Lexer::initFromFile(const std::string& filename) {
    buffer.initFromFile(filename);
}

void Lexer::initFromString(const std::string& source) {
    buffer.initFromString(source);
}

char Lexer::currentChar() {
    return buffer.currentChar();
}

char Lexer::peekChar() {
    return buffer.peekChar(1);
}

void Lexer::advance() {
    buffer.advance();
}

void Lexer::skipWhitespace() {
    while (std::isspace(currentChar())) {
        advance();
    }
}

void Lexer::reportError(const std::string& message) {
    hasErrorFlag = true;
    if (diagnostics) {
        diagnostics->error(buffer.getLexemeStartLine(), 
                          buffer.getLexemeStartColumn(), 
                          message);
    }
}

void Lexer::reportError(const std::string& message, const std::string& suggestion) {
    hasErrorFlag = true;
    if (diagnostics) {
        Diagnostic diag(DiagnosticLevel::Error, 
                       SourceLocation(buffer.getLexemeStartLine(), 
                                     buffer.getLexemeStartColumn()), 
                       message);
        diag.withSuggestion(suggestion);
        diagnostics->report(diag);
    }
}

void Lexer::reportErrorWithLength(const std::string& message, int length) {
    hasErrorFlag = true;
    if (diagnostics) {
        Diagnostic diag(DiagnosticLevel::Error,
                       SourceLocation(buffer.getLexemeStartLine(), 
                                     buffer.getLexemeStartColumn(), 
                                     length),
                       message);
        diagnostics->report(diag);
    }
}

Token Lexer::readIdentifierOrKeyword() {
    buffer.markLexemeStart();
    int startLine = buffer.getLexemeStartLine();
    int startCol = buffer.getLexemeStartColumn();
    
    std::string value;
    
    // <id> -> l{l|d} (letter followed by letters or digits)
    while (std::isalnum(currentChar()) || currentChar() == '_') {
        value += currentChar();
        advance();
    }
    
    // Check for underscore at start (invalid in standard PL/0)
    if (!value.empty() && value[0] == '_') {
        reportError("identifier cannot start with underscore", 
                   "identifiers must start with a letter");
        return Token(TokenType::ERROR, value, startLine, startCol);
    }
    
    // Convert to lowercase for keyword matching
    std::string lowerValue = value;
    for (char& c : lowerValue) {
        c = std::tolower(c);
    }
    
    TokenType type = TokenType::IDENTIFIER;
    
    // Check if it's a keyword
    if (keywords.count(lowerValue)) {
        if (lowerValue == "program") type = TokenType::PROGRAM;
        else if (lowerValue == "const") type = TokenType::CONST;
        else if (lowerValue == "var") type = TokenType::VAR;
        else if (lowerValue == "procedure") type = TokenType::PROCEDURE;
        else if (lowerValue == "begin") type = TokenType::BEGIN;
        else if (lowerValue == "end") type = TokenType::END;
        else if (lowerValue == "if") type = TokenType::IF;
        else if (lowerValue == "then") type = TokenType::THEN;
        else if (lowerValue == "else") type = TokenType::ELSE;
        else if (lowerValue == "while") type = TokenType::WHILE;
        else if (lowerValue == "do") type = TokenType::DO;
        else if (lowerValue == "call") type = TokenType::CALL;
        else if (lowerValue == "read") type = TokenType::READ;
        else if (lowerValue == "write") type = TokenType::WRITE;
        else if (lowerValue == "odd") type = TokenType::ODD;
    }
    
    return Token(type, value, startLine, startCol);
}

Token Lexer::readNumber() {
    buffer.markLexemeStart();
    int startLine = buffer.getLexemeStartLine();
    int startCol = buffer.getLexemeStartColumn();
    
    std::string value;
    
    // <integer> -> d{d} (digit followed by digits)
    while (std::isdigit(currentChar())) {
        value += currentChar();
        advance();
    }
    
    // Check for invalid identifier (number followed by letter)
    if (std::isalpha(currentChar()) || currentChar() == '_') {
        std::string invalidToken = value;
        while (std::isalnum(currentChar()) || currentChar() == '_') {
            invalidToken += currentChar();
            advance();
        }
        
        Diagnostic diag(DiagnosticLevel::Error,
                       SourceLocation(startLine, startCol, 
                                     static_cast<int>(invalidToken.length())),
                       "invalid identifier '" + invalidToken + "'");
        diag.withSuggestion("identifiers cannot start with a digit");
        if (diagnostics) {
            diagnostics->report(diag);
        }
        hasErrorFlag = true;
        
        return Token(TokenType::ERROR, invalidToken, startLine, startCol);
    }
    
    // Check for number overflow
    try {
        long long num = std::stoll(value);
        if (num > 2147483647LL || num < -2147483648LL) {
            Diagnostic diag(DiagnosticLevel::Warning,
                           SourceLocation(startLine, startCol, 
                                         static_cast<int>(value.length())),
                           "integer literal is too large");
            diag.withSuggestion("maximum value is 2147483647");
            if (diagnostics) {
                diagnostics->report(diag);
            }
        }
    } catch (...) {
        Diagnostic diag(DiagnosticLevel::Error,
                       SourceLocation(startLine, startCol, 
                                     static_cast<int>(value.length())),
                       "integer literal overflow");
        if (diagnostics) {
            diagnostics->report(diag);
        }
        hasErrorFlag = true;
    }
    
    return Token(TokenType::INTEGER, value, startLine, startCol);
}

// Helper function to get the length of a UTF-8 character
static int getUTF8CharLength(unsigned char firstByte) {
    if ((firstByte & 0x80) == 0) return 1;      // 0xxxxxxx - ASCII
    if ((firstByte & 0xE0) == 0xC0) return 2;   // 110xxxxx - 2 bytes
    if ((firstByte & 0xF0) == 0xE0) return 3;   // 1110xxxx - 3 bytes (Chinese, etc.)
    if ((firstByte & 0xF8) == 0xF0) return 4;   // 11110xxx - 4 bytes
    return 1; // Invalid, treat as single byte
}

// Check if current character can start a valid token
static bool isValidTokenStart(char ch) {
    return std::isalpha(ch) || std::isdigit(ch) ||
           ch == '+' || ch == '-' || ch == '*' || ch == '/' ||
           ch == '(' || ch == ')' || ch == ',' || ch == ';' ||
           ch == '=' || ch == '<' || ch == '>' || ch == ':';
}

Token Lexer::readOperator() {
    buffer.markLexemeStart();
    int startLine = buffer.getLexemeStartLine();
    int startCol = buffer.getLexemeStartColumn();
    
    char ch = currentChar();
    
    switch (ch) {
        case '+':
            advance();
            return Token(TokenType::PLUS, "+", startLine, startCol);
        case '-':
            advance();
            return Token(TokenType::MINUS, "-", startLine, startCol);
        case '*':
            advance();
            return Token(TokenType::MULTIPLY, "*", startLine, startCol);
        case '/':
            advance();
            return Token(TokenType::DIVIDE, "/", startLine, startCol);
        case '(':
            advance();
            return Token(TokenType::LPAREN, "(", startLine, startCol);
        case ')':
            advance();
            return Token(TokenType::RPAREN, ")", startLine, startCol);
        case ',':
            advance();
            return Token(TokenType::COMMA, ",", startLine, startCol);
        case ';':
            advance();
            return Token(TokenType::SEMICOLON, ";", startLine, startCol);
        case '=':
            advance();
            return Token(TokenType::EQ, "=", startLine, startCol);
        case '<':
            advance();
            if (currentChar() == '=') {
                advance();
                return Token(TokenType::LEQ, "<=", startLine, startCol, 2);
            } else if (currentChar() == '>') {
                advance();
                return Token(TokenType::NEQ, "<>", startLine, startCol, 2);
            }
            return Token(TokenType::LT, "<", startLine, startCol);
        case '>':
            advance();
            if (currentChar() == '=') {
                advance();
                return Token(TokenType::GEQ, ">=", startLine, startCol, 2);
            }
            return Token(TokenType::GT, ">", startLine, startCol);
        case ':':
            advance();
            if (currentChar() == '=') {
                advance();
                return Token(TokenType::ASSIGN, ":=", startLine, startCol, 2);
            }
            // Error: lone colon
            {
                Diagnostic diag(DiagnosticLevel::Error,
                               SourceLocation(startLine, startCol),
                               "unexpected ':' - did you mean ':='?");
                diag.withSuggestion("use ':=' for assignment");
                diag.withFix(":=");
                if (diagnostics) {
                    diagnostics->report(diag);
                }
                hasErrorFlag = true;
            }
            return Token(TokenType::ERROR, ":", startLine, startCol);
            
        case '!':
            // Common mistake: != instead of <>
            advance();
            if (currentChar() == '=') {
                advance();
                Diagnostic diag(DiagnosticLevel::Error,
                               SourceLocation(startLine, startCol, 2),
                               "'!=' is not valid in PL/0");
                diag.withSuggestion("use '<>' for not-equal comparison");
                diag.withFix("<>");
                if (diagnostics) {
                    diagnostics->report(diag);
                }
                hasErrorFlag = true;
                return Token(TokenType::ERROR, "!=", startLine, startCol, 2);
            }
            // Lone !
            {
                Diagnostic diag(DiagnosticLevel::Error,
                               SourceLocation(startLine, startCol),
                               "unexpected character '!'");
                if (diagnostics) {
                    diagnostics->report(diag);
                }
                hasErrorFlag = true;
            }
            return Token(TokenType::ERROR, "!", startLine, startCol);
            
        case '&':
        case '|':
            // Common mistake from C/C++
            {
                char op = ch;
                advance();
                std::string opStr(1, op);
                if (currentChar() == op) {
                    advance();
                    opStr += op;
                }
                Diagnostic diag(DiagnosticLevel::Error,
                               SourceLocation(startLine, startCol, 
                                             static_cast<int>(opStr.length())),
                               "'" + opStr + "' is not valid in PL/0");
                diag.withSuggestion("PL/0 does not have logical operators");
                if (diagnostics) {
                    diagnostics->report(diag);
                }
                hasErrorFlag = true;
                return Token(TokenType::ERROR, opStr, startLine, startCol);
            }
            
        default:
            // Handle unknown characters
            std::string invalidChars;
            
            // Check if this is a UTF-8 multi-byte character
            unsigned char byte = static_cast<unsigned char>(ch);
            if (byte >= 0x80) {
                // UTF-8 multi-byte character
                int charLen = getUTF8CharLength(byte);
                for (int i = 0; i < charLen; i++) {
                    invalidChars += currentChar();
                    advance();
                }
                
                // Collect consecutive invalid UTF-8 characters
                while (!buffer.isEOF() && !std::isspace(currentChar())) {
                    byte = static_cast<unsigned char>(currentChar());
                    
                    if (byte < 0x80 && isValidTokenStart(currentChar())) {
                        break;
                    }
                    
                    if (byte >= 0x80) {
                        charLen = getUTF8CharLength(byte);
                        for (int i = 0; i < charLen && !buffer.isEOF(); i++) {
                            invalidChars += currentChar();
                            advance();
                        }
                    } else {
                        invalidChars += currentChar();
                        advance();
                    }
                }
                
                Diagnostic diag(DiagnosticLevel::Error,
                               SourceLocation(startLine, startCol, 
                                             static_cast<int>(invalidChars.length())),
                               "invalid character(s) '" + invalidChars + "'");
                diag.withSuggestion("PL/0 only supports ASCII characters");
                if (diagnostics) {
                    diagnostics->report(diag);
                }
                hasErrorFlag = true;
                
                return Token(TokenType::ERROR, invalidChars, startLine, startCol);
            } else {
                // ASCII invalid character
                invalidChars += ch;
                advance();
                
                // Collect consecutive invalid ASCII characters
                while (!buffer.isEOF() && !std::isspace(currentChar())) {
                    byte = static_cast<unsigned char>(currentChar());
                    
                    if (isValidTokenStart(currentChar())) {
                        break;
                    }
                    
                    if (byte >= 0x80) {
                        break;
                    }
                    
                    invalidChars += currentChar();
                    advance();
                }
                
                Diagnostic diag(DiagnosticLevel::Error,
                               SourceLocation(startLine, startCol, 
                                             static_cast<int>(invalidChars.length())),
                               "unexpected character '" + invalidChars + "'");
                
                // Provide helpful suggestions for common mistakes
                if (invalidChars == "{" || invalidChars == "}") {
                    diag.withSuggestion("use 'begin' and 'end' for blocks in PL/0");
                } else if (invalidChars == "[" || invalidChars == "]") {
                    diag.withSuggestion("PL/0 does not support arrays");
                } else if (invalidChars == "\"" || invalidChars == "'") {
                    diag.withSuggestion("PL/0 does not support string literals");
                }
                
                if (diagnostics) {
                    diagnostics->report(diag);
                }
                hasErrorFlag = true;
                
                return Token(TokenType::ERROR, invalidChars, startLine, startCol);
            }
    }
}

std::vector<Token> Lexer::tokenize() {
    tokens.clear();
    hasErrorFlag = false;
    
    while (!buffer.isEOF()) {
        skipWhitespace();
        
        if (buffer.isEOF()) break;
        
        char ch = currentChar();
        
        if (std::isalpha(ch) || ch == '_') {
            tokens.push_back(readIdentifierOrKeyword());
        } else if (std::isdigit(ch)) {
            tokens.push_back(readNumber());
        } else {
            tokens.push_back(readOperator());
        }
    }
    
    // Add EOF token
    tokens.push_back(Token(TokenType::END_OF_FILE, "", 
                          buffer.getLine(), buffer.getColumn(), 0));
    return tokens;
}

std::string Lexer::tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::PROGRAM: return "PROGRAM";
        case TokenType::CONST: return "CONST";
        case TokenType::VAR: return "VAR";
        case TokenType::PROCEDURE: return "PROCEDURE";
        case TokenType::BEGIN: return "BEGIN";
        case TokenType::END: return "END";
        case TokenType::IF: return "IF";
        case TokenType::THEN: return "THEN";
        case TokenType::ELSE: return "ELSE";
        case TokenType::WHILE: return "WHILE";
        case TokenType::DO: return "DO";
        case TokenType::CALL: return "CALL";
        case TokenType::READ: return "READ";
        case TokenType::WRITE: return "WRITE";
        case TokenType::ODD: return "ODD";
        case TokenType::IDENTIFIER: return "IDENT";
        case TokenType::INTEGER: return "NUMBER";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::MULTIPLY: return "TIMES";
        case TokenType::DIVIDE: return "SLASH";
        case TokenType::ASSIGN: return "BECOMES";
        case TokenType::EQ: return "EQL";
        case TokenType::NEQ: return "NEQ";
        case TokenType::LT: return "LSS";
        case TokenType::LEQ: return "LEQ";
        case TokenType::GT: return "GTR";
        case TokenType::GEQ: return "GEQ";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::COMMA: return "COMMA";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::END_OF_FILE: return "EOF";
        case TokenType::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string Lexer::tokenTypeToReadable(TokenType type) {
    switch (type) {
        case TokenType::PROGRAM: return "'program'";
        case TokenType::CONST: return "'const'";
        case TokenType::VAR: return "'var'";
        case TokenType::PROCEDURE: return "'procedure'";
        case TokenType::BEGIN: return "'begin'";
        case TokenType::END: return "'end'";
        case TokenType::IF: return "'if'";
        case TokenType::THEN: return "'then'";
        case TokenType::ELSE: return "'else'";
        case TokenType::WHILE: return "'while'";
        case TokenType::DO: return "'do'";
        case TokenType::CALL: return "'call'";
        case TokenType::READ: return "'read'";
        case TokenType::WRITE: return "'write'";
        case TokenType::ODD: return "'odd'";
        case TokenType::IDENTIFIER: return "identifier";
        case TokenType::INTEGER: return "integer";
        case TokenType::PLUS: return "'+'";
        case TokenType::MINUS: return "'-'";
        case TokenType::MULTIPLY: return "'*'";
        case TokenType::DIVIDE: return "'/'";
        case TokenType::ASSIGN: return "':='";
        case TokenType::EQ: return "'='";
        case TokenType::NEQ: return "'<>'";
        case TokenType::LT: return "'<'";
        case TokenType::LEQ: return "'<='";
        case TokenType::GT: return "'>'";
        case TokenType::GEQ: return "'>='";
        case TokenType::LPAREN: return "'('";
        case TokenType::RPAREN: return "')'";
        case TokenType::COMMA: return "','";
        case TokenType::SEMICOLON: return "';'";
        case TokenType::END_OF_FILE: return "end of file";
        case TokenType::ERROR: return "error";
        default: return "unknown token";
    }
}

void Lexer::printTokens(const std::vector<Token>& tokenList) const {
    // Check if colors should be used
    bool useColor = diagnostics && diagnostics->hasColors();
    
    const char* RESET = useColor ? "\033[0m" : "";
    const char* BOLD = useColor ? "\033[1m" : "";
    const char* CYAN = useColor ? "\033[36m" : "";
    const char* GREEN = useColor ? "\033[32m" : "";
    const char* YELLOW = useColor ? "\033[33m" : "";
    const char* BLUE = useColor ? "\033[34m" : "";
    
    // Print table header
    std::cout << "\n" << BOLD << CYAN 
              << "╔══════════════════════════════════════════════════════╗" << RESET << "\n";
    std::cout << BOLD << CYAN << "║" << RESET << "                   " << BOLD 
              << "TOKEN LIST" << RESET << "                         " << CYAN << "║" << RESET << "\n";
    std::cout << BOLD << CYAN 
              << "╠══════════════════════════════════════════════════════╣" << RESET << "\n";
    std::cout << BOLD << CYAN << "║" << RESET 
              << BOLD << std::left 
              << std::setw(8) << " Line" 
              << std::setw(8) << "Col" 
              << std::setw(14) << "Type" 
              << "Value" << RESET
              << std::string(15, ' ') << CYAN << "║" << RESET << "\n";
    std::cout << BOLD << CYAN 
              << "╠══════════════════════════════════════════════════════╣" << RESET << "\n";
    
    // Print each token
    for (const auto& token : tokenList) {
        std::cout << CYAN << "║" << RESET << " ";
        
        // Line number
        std::cout << BLUE << std::setw(6) << token.line << RESET << " ";
        
        // Column number
        std::cout << std::setw(7) << token.column << " ";
        
        // Token type (with color coding)
        std::string typeStr = tokenTypeToString(token.type);
        if (token.type == TokenType::ERROR) {
            std::cout << "\033[31m" << std::setw(13) << typeStr << RESET << " ";
        } else if (token.type >= TokenType::PROGRAM && token.type <= TokenType::ODD) {
            std::cout << GREEN << std::setw(13) << typeStr << RESET << " ";
        } else if (token.type == TokenType::IDENTIFIER || token.type == TokenType::INTEGER) {
            std::cout << YELLOW << std::setw(13) << typeStr << RESET << " ";
        } else {
            std::cout << std::setw(13) << typeStr << " ";
        }
        
        // Token value
        if (token.type == TokenType::END_OF_FILE) {
            std::cout << std::setw(19) << "" << CYAN << "║" << RESET << "\n";
        } else {
            std::string val = token.value;
            if (val.length() > 18) {
                val = val.substr(0, 15) + "...";
            }
            std::cout << std::left << std::setw(19) << val << CYAN << "║" << RESET << "\n";
        }
    }
    
    // Print table footer
    std::cout << BOLD << CYAN 
              << "╚══════════════════════════════════════════════════════╝" << RESET << "\n\n";
    
    // Print summary
    int tokenCount = static_cast<int>(tokenList.size()) - 1;  // Exclude EOF token
    std::cout << "Total tokens: " << BOLD << tokenCount << RESET << "\n\n";
}