#include "parser_codegen.h"
#include "diagnostics.h"
#include <iostream>
#include <sstream>


// Constructor and Token Navigation
ParserWithCodegen::ParserWithCodegen(const std::vector<Token>& tokenList, DiagnosticEngine* diag) 
    : tokens(tokenList), position(0), indentLevel(0), diagnostics(diag), hasErrorFlag(false) {}

Token ParserWithCodegen::currentToken() {
    if (position >= tokens.size()) {
        return tokens.back();
    }
    return tokens[position];
}

Token ParserWithCodegen::peekToken(int offset) {
    size_t peekPos = position + offset;
    if (peekPos >= tokens.size()) {
        return tokens.back();
    }
    return tokens[peekPos];
}

// for better error report, I use one previous token
Token ParserWithCodegen::previousToken() {
    if (position > 0) {
        return tokens[position - 1];
    }
    return tokens[0];
}

void ParserWithCodegen::advance() {
    if (position < tokens.size() - 1) {
        position++;
    }
}

bool ParserWithCodegen::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool ParserWithCodegen::check(TokenType type) {
    return currentToken().type == type;
}

// Error Handling and Reporting
void ParserWithCodegen::reportError(const std::string& message) {
    hasErrorFlag = true;
    if (diagnostics) {
        Token tok = currentToken();
        diagnostics->error(tok.line, tok.column, message);
    }
}

void ParserWithCodegen::reportError(const std::string& message, const std::string& suggestion) {
    hasErrorFlag = true;
    if (diagnostics) {
        Token tok = currentToken();
        Diagnostic diag(DiagnosticLevel::Error, SourceLocation(tok.line, tok.column, tok.length), message);
        diag.withSuggestion(suggestion);
        diagnostics->report(diag);
    }
}

void ParserWithCodegen::reportErrorAt(const Token& token, const std::string& message) {
    hasErrorFlag = true;
    if (diagnostics) {
        Diagnostic diag(DiagnosticLevel::Error, SourceLocation(token.line, token.column, token.length), message);
        diagnostics->report(diag);
    }
}

void ParserWithCodegen::reportErrorAt(const Token& token, const std::string& message, const std::string& suggestion) {
    hasErrorFlag = true;
    if (diagnostics) {
        Diagnostic diag(DiagnosticLevel::Error, SourceLocation(token.line, token.column, token.length), message);
        diag.withSuggestion(suggestion);
        diagnostics->report(diag);
    }
}

void ParserWithCodegen::reportExpected(const std::string& expected) {
    hasErrorFlag = true;
    if (diagnostics) {
        Token tok = currentToken();
        std::string found = (tok.type == TokenType::END_OF_FILE)  ? "end of file" : "'" + tok.value + "'";
        std::string msg = "expected " + expected + ", found " + found;
        
        Diagnostic diag(DiagnosticLevel::Error, SourceLocation(tok.line, tok.column, tok.length), msg);
        
        // Add helpful suggestions for common mistakes
        if (expected == "';'") {
            if (tok.type == TokenType::BEGIN) {
                diag.withSuggestion("add ';' before 'begin'");
            } else if (tok.type == TokenType::IDENTIFIER) {
                diag.withSuggestion("statements must be separated by ';'");
            } else if (tok.type == TokenType::END) {
                diag.withSuggestion("add ';' after the last statement before 'end'");
            }
        } else if (expected == "':='") {
            if (tok.type == TokenType::EQ) {
                diag.withSuggestion("use ':=' for assignment, '=' is for comparison");
                diag.withFix(":=");
            }
        } else if (expected == "'then'") {
            diag.withSuggestion("'if' condition must be followed by 'then'");
        } else if (expected == "'do'") {
            diag.withSuggestion("'while' condition must be followed by 'do'");
        } else if (expected == "'end'") {
            diag.withSuggestion("'begin' must have a matching 'end'");
        } else if (expected == "')'") {
            diag.withSuggestion("missing closing parenthesis");
        } else if (expected == "'('") {
            diag.withSuggestion("missing opening parenthesis");
        }
        
        diagnostics->report(diag);
    }
}

void ParserWithCodegen::reportExpectedAt(const Token& token, const std::string& expected) {
    hasErrorFlag = true;
    if (diagnostics) {
        std::string found = (token.type == TokenType::END_OF_FILE) ? "end of file" : "'" + token.value + "'";
        std::string msg = "expected " + expected + ", found " + found;
        diagnostics->error(token.line, token.column, msg);
    }
}

void ParserWithCodegen::expect(TokenType type, const std::string& message) {
    if (check(type)) {
        advance();
    } else {
        reportExpected(message);
        synchronize();
    }
}

void ParserWithCodegen::expectSemicolon() {
    if (check(TokenType::SEMICOLON)) {
        advance();
    } else {
        // Provide context-aware error message
        Token tok = currentToken();
        Token prev = previousToken();
        
        hasErrorFlag = true;
        if (diagnostics) {
            std::string msg = "expected ';'";
            std::string suggestion;
            
            if (tok.type == TokenType::IDENTIFIER || tok.type == TokenType::BEGIN ||
                tok.type == TokenType::IF || tok.type == TokenType::WHILE ||
                tok.type == TokenType::CALL || tok.type == TokenType::READ ||
                tok.type == TokenType::WRITE) {
                // Likely forgot semicolon between statements
                suggestion = "add ';' after '" + prev.value + "'";
                // Point to end of previous token
                Diagnostic diag(DiagnosticLevel::Error, SourceLocation(prev.line, prev.column + prev.length, 1), msg);
                diag.withSuggestion(suggestion);
                diagnostics->report(diag);
            } else {
                Diagnostic diag(DiagnosticLevel::Error, SourceLocation(tok.line, tok.column, tok.length), msg + ", found '" + tok.value + "'");
                diagnostics->report(diag);
            }
        }
        synchronize();
    }
}

void ParserWithCodegen::synchronize() {
    while (currentToken().type != TokenType::END_OF_FILE) {
        // Stop at statement boundaries
        if (currentToken().type == TokenType::SEMICOLON) {
            advance();  // Consume the semicolon
            return;
        }
        
        // Stop at block boundaries
        if (currentToken().type == TokenType::BEGIN ||
            currentToken().type == TokenType::END) {
            return;
        }
        
        // Stop at declaration keywords
        if (currentToken().type == TokenType::CONST ||
            currentToken().type == TokenType::VAR ||
            currentToken().type == TokenType::PROCEDURE) {
            return;
        }
        
        advance();
    }
}

void ParserWithCodegen::synchronizeTo(std::initializer_list<TokenType> types) {
    while (currentToken().type != TokenType::END_OF_FILE) {
        for (TokenType t : types) {
            if (currentToken().type == t) {
                return;
            }
        }
        advance();
    }
}

// Parse Tree Output (controlled by options)

void ParserWithCodegen::printIndent() {
    if (!options.showParseTree) return;
    for (int i = 0; i < indentLevel; i++) {
        std::cout << "  ";
    }
}

void ParserWithCodegen::parseLog(const std::string& message) {
    if (!options.showParseTree) return;
    printIndent();
    std::cout << message << "\n";
}

void ParserWithCodegen::parseLogEnter(const std::string& rule) {
    if (!options.showParseTree) return;
    printIndent();
    std::cout << "├─ " << rule << "\n";
    indentLevel++;
}

void ParserWithCodegen::parseLogExit(const std::string& rule) {
    if (!options.showParseTree) return;
    indentLevel--;
}

// Grammar Rules Implementation

// <prog> -> program <id>; <block>
void ParserWithCodegen::parseProgram() {
    parseLogEnter("<program>");
    
    expect(TokenType::PROGRAM, "'program'");
    
    if (check(TokenType::IDENTIFIER)) {
        parseLog("Program name: " + currentToken().value);
        advance();
    } else {
        reportExpected("program name (identifier)");
    }
    
    expect(TokenType::SEMICOLON, "';'");
    
    parseBlock();
    
    // Generate program end code: OPR 0 0
    codeGen.emit(OpCode::OPR, 0, static_cast<int>(OprType::RET));
    
    if (!check(TokenType::END_OF_FILE)) {
        Token tok = currentToken();
        reportErrorAt(tok, "unexpected token after end of program", "program should end after the main block");
    }
    
    parseLogExit("<program>");
}

// <block> -> [<condecl>][<vardecl>][<proc>]<body>
void ParserWithCodegen::parseBlock() {
    parseLogEnter("<block>");
    
    int savedAddress = symbolTable.getCurrentAddress();
    int jmpAddr = codeGen.emit(OpCode::JMP, 0, 0);  // Jump over declarations
    
    // Constant declarations
    if (check(TokenType::CONST)) {
        parseCondecl();
    }
    
    // Variable declarations
    if (check(TokenType::VAR)) {
        parseVardecl();
    }
    
    // Procedure declarations
    while (check(TokenType::PROCEDURE)) {
        parseProc();
    }
    
    // Backpatch jump address
    codeGen.backpatch(jmpAddr, codeGen.getNextAddress());
    
    // Allocate data space for above procdure
    int dataSize = symbolTable.getCurrentAddress();
    codeGen.emit(OpCode::INT, 0, dataSize);
    
    // Body
    parseBody();
    
    parseLogExit("<block>");
}

// expanded upon the original version to support the definition of negative numbers.
// <condecl> -> const <const>{,<const>};
// [old] <const> -> id := <integer>
// [now] <const> -> id := [+|-]<integer>
void ParserWithCodegen::parseCondecl() {
    parseLogEnter("<const-declaration>");
    
    expect(TokenType::CONST, "'const'");
    
    do {
        if (check(TokenType::IDENTIFIER)) {
            Token nameToken = currentToken();
            std::string constName = nameToken.value;
            advance();
            
            // Expect ':=' for constant definition
            if (check(TokenType::EQ)) {
                // Common mistake: using '=' instead of ':='
                reportErrorAt(currentToken(), "use ':=' for constant definition, not '='", "PL/0 uses ':=' for both assignment and constant definition");
                advance();  // Skip the '=' and try to continue
            } else {
                expect(TokenType::ASSIGN, "':='");
            }
            
            bool negative = false;
            if (check(TokenType::MINUS)) {
                negative = true;
                advance();
            } else if (check(TokenType::PLUS)) {
                advance();
            }
            
            if (check(TokenType::INTEGER)) {
                int value = std::stoi(currentToken().value); // transfer string to integer
                if (negative) value = -value;
                
                // Check for redeclaration
                if (symbolTable.lookupCurrent(constName)) {
                    reportErrorAt(nameToken, "redefinition of '" + constName + "'", "'" + constName + "' is already declared in this scope");
                } else {
                    symbolTable.addSymbol(constName, SymbolType::CONST, value);
                    parseLog("Constant: " + constName + " = " + std::to_string(value));
                }
                
                advance();
            } else {
                reportExpected("integer value");
            }
        } else {
            reportExpected("identifier");
            break;
        }
    } while (match(TokenType::COMMA));
    
    expectSemicolon();
    
    parseLogExit("<const-declaration>");
}

// <vardecl> -> var <id>{,<id>};
void ParserWithCodegen::parseVardecl() {
    parseLogEnter("<var-declaration>");
    
    expect(TokenType::VAR, "'var'");
    
    do {
        if (check(TokenType::IDENTIFIER)) {
            Token nameToken = currentToken();
            std::string varName = nameToken.value;
            
            // Check for redeclaration
            if (symbolTable.lookupCurrent(varName)) {
                reportErrorAt(nameToken, "redefinition of '" + varName + "'","'" + varName + "' is already declared in this scope");
            } else {
                symbolTable.addSymbol(varName, SymbolType::VAR, 0);
                parseLog("Variable: " + varName);
            }
            
            advance();
        } else {
            reportExpected("identifier");
            break;
        }
    } while (match(TokenType::COMMA));
    
    expectSemicolon();
    
    parseLogExit("<var-declaration>");
}

// <proc> -> procedure <id>([<id>{,<id>}]);<block>{;<proc>}
void ParserWithCodegen::parseProc() {
    parseLogEnter("<procedure>");
    
    expect(TokenType::PROCEDURE, "'procedure'");
    
    Token procNameToken;
    std::string procName;
    
    if (check(TokenType::IDENTIFIER)) {
        procNameToken = currentToken();
        procName = procNameToken.value;
        
        // Add procedure symbol
        if (symbolTable.lookupCurrent(procName)) {
            reportErrorAt(procNameToken, "redefinition of procedure '" + procName + "'");
        } else {
            symbolTable.addSymbol(procName, SymbolType::PROCEDURE, codeGen.getNextAddress());
            parseLog("Procedure: " + procName);
        }
        
        advance();
    } else {
        reportExpected("procedure name");
    }
    
    expect(TokenType::LPAREN, "'('");
    
    // Enter new scope for procedure
    symbolTable.enterScope();
    
    // Parameter handling
    if (check(TokenType::IDENTIFIER)) {
        parseLog("Parameters:");
        do {
            if (check(TokenType::IDENTIFIER)) {
                std::string paramName = currentToken().value;
                symbolTable.addSymbol(paramName, SymbolType::VAR, 0); // formal parameters, use 0 to occupy the place
                parseLog("  - " + paramName);
                advance();
            }
        } while (match(TokenType::COMMA));
    }
    
    expect(TokenType::RPAREN, "')'");
    expectSemicolon();
    
    // Parse procedure body
    parseBlock();
    
    // Generate return instruction
    codeGen.emit(OpCode::OPR, 0, static_cast<int>(OprType::RET));
    
    // Exit scope
    symbolTable.exitScope();
    
    expectSemicolon();
    
    parseLogExit("<procedure>");
}

// <body> -> begin <statement>{;<statement>}end
void ParserWithCodegen::parseBody() {
    parseLogEnter("<body>");
    
    Token beginToken = currentToken();
    expect(TokenType::BEGIN, "'begin'");
    
    parseStatement();
    
    while (match(TokenType::SEMICOLON)) {
        if (check(TokenType::END)) {
            // Allow trailing semicolon before 'end'
            break;
        }
        parseStatement();
    }
    
    if (!check(TokenType::END)) {
        Token tok = currentToken();
        if (tok.type == TokenType::IDENTIFIER || tok.type == TokenType::IF ||
            tok.type == TokenType::WHILE || tok.type == TokenType::CALL ||
            tok.type == TokenType::READ || tok.type == TokenType::WRITE ||
            tok.type == TokenType::BEGIN) {
            // Likely missing semicolon
            reportError("expected ';' between statements", "statements must be separated by ';'");
        } else {
            reportExpected("'end'");
        }
    }
    
    expect(TokenType::END, "'end'");
    
    parseLogExit("<body>");
}

// <statement> -> <id> := <exp>
//              | if <lexp> then <statement> [else <statement>]
//              | while <lexp> do <statement>
//              | call <id>([<exp>{,<exp>}])
//              | <body>
//              | read(<id>{,<id>})
//              | write(<exp>{,<exp>})
void ParserWithCodegen::parseStatement() {
    parseLogEnter("<statement>");
    
    if (check(TokenType::IDENTIFIER)) {
        // Assignment statement
        Token varToken = currentToken();
        std::string varName = varToken.value;
        Symbol* sym = symbolTable.lookup(varName);
        
        parseLog("Assignment to: " + varName);
        
        if (!sym) {
            reportErrorAt(varToken, "use of undeclared identifier '" + varName + "'", "declare '" + varName + "' with 'var' before use");
            advance();
            // Try to recover by parsing the rest
            if (check(TokenType::ASSIGN) || check(TokenType::EQ)) {
                advance();
                parseExp();
            }
            parseLogExit("<statement>");
            return;
        }
        
        if (sym->type == SymbolType::CONST) {
            reportErrorAt(varToken, "cannot assign to constant '" + varName + "'", "'" + varName + "' was declared as 'const'");
            advance();
            if (check(TokenType::ASSIGN) || check(TokenType::EQ)) {
                advance();
                parseExp();
            }
            parseLogExit("<statement>");
            return;
        }
        
        if (sym->type == SymbolType::PROCEDURE) {
            reportErrorAt(varToken, "cannot assign to procedure '" + varName + "'", "did you mean 'call " + varName + "(...)'?");
            advance();
            if (check(TokenType::ASSIGN) || check(TokenType::EQ)) {
                advance();
                parseExp();
            }
            parseLogExit("<statement>");
            return;
        }
        
        advance();
        
        // Check for '=' instead of ':='
        if (check(TokenType::EQ)) {
            reportError("use ':=' for assignment, not '='", "'=' is for comparison, ':=' is for assignment");
            // Treat '=' as ':=' and continue
            advance();
        } else {
            expect(TokenType::ASSIGN, "':='");
        }
        
        parseExp();
        
        // Generate store instruction
        int levelDiff = symbolTable.getCurrentLevel() - sym->level;
        codeGen.emit(OpCode::STO, levelDiff, sym->address);
        
    } else if (match(TokenType::IF)) {
        parseLog("IF statement");
        
        parseLexp();
        
        expect(TokenType::THEN, "'then'");
        
        int jpcAddr = codeGen.emit(OpCode::JPC, 0, 0);  // false
        
        parseStatement();
        
        if (match(TokenType::ELSE)) {
            parseLog("ELSE clause");
            int jmpAddr = codeGen.emit(OpCode::JMP, 0, 0); // true
            codeGen.backpatch(jpcAddr, codeGen.getNextAddress()); 
            parseStatement();
            codeGen.backpatch(jmpAddr, codeGen.getNextAddress());
        } else {
            codeGen.backpatch(jpcAddr, codeGen.getNextAddress());
        }
        
    } else if (match(TokenType::WHILE)) {
        parseLog("WHILE loop");
        
        int loopAddr = codeGen.getNextAddress();
        
        parseLexp();
        
        expect(TokenType::DO, "'do'");
        
        int jpcAddr = codeGen.emit(OpCode::JPC, 0, 0);
        
        parseStatement();
        
        codeGen.emit(OpCode::JMP, 0, loopAddr);
        codeGen.backpatch(jpcAddr, codeGen.getNextAddress());
        
    } else if (match(TokenType::CALL)) {
        parseLog("CALL statement");
        
        if (check(TokenType::IDENTIFIER)) {
            Token procToken = currentToken();
            std::string procName = procToken.value;
            Symbol* sym = symbolTable.lookup(procName);
            
            parseLog("Calling: " + procName);
            
            if (!sym) {
                reportErrorAt(procToken, "call to undeclared procedure '" + procName + "'", "declare procedure before calling it");
            } else if (sym->type != SymbolType::PROCEDURE) {
                std::string typeStr = (sym->type == SymbolType::CONST) ? "constant" : "variable";
                reportErrorAt(procToken, "'" + procName + "' is a " + typeStr + ", not a procedure", "only procedures can be called");
            } else {
                int levelDiff = symbolTable.getCurrentLevel() - sym->level;
                codeGen.emit(OpCode::CAL, levelDiff, sym->address);
            }
            
            advance();
        } else {
            reportExpected("procedure name");
        }
        
        expect(TokenType::LPAREN, "'('");
        
        // Parse arguments
        if (!check(TokenType::RPAREN)) {
            do {
                parseExp();
            } while (match(TokenType::COMMA));
        }
        
        expect(TokenType::RPAREN, "')'");
        
    } else if (check(TokenType::BEGIN)) {
        parseBody();
        
    } else if (match(TokenType::READ)) {
        parseLog("READ statement");
        
        expect(TokenType::LPAREN, "'('");
        
        do {
            if (check(TokenType::IDENTIFIER)) {
                Token varToken = currentToken();
                std::string varName = varToken.value;
                Symbol* sym = symbolTable.lookup(varName);
                
                parseLog("Reading into: " + varName);
                
                if (!sym) {
                    reportErrorAt(varToken, "use of undeclared identifier '" + varName + "'");
                } else if (sym->type == SymbolType::CONST) {
                    reportErrorAt(varToken, "cannot read into constant '" + varName + "'", "'" + varName + "' was declared as 'const'");
                } else if (sym->type == SymbolType::PROCEDURE) {
                    reportErrorAt(varToken, "cannot read into procedure '" + varName + "'");
                } else {
                    int levelDiff = symbolTable.getCurrentLevel() - sym->level;
                    codeGen.emit(OpCode::RED, levelDiff, sym->address);
                }
                
                advance();
            } else {
                reportExpected("identifier");
                break;
            }
        } while (match(TokenType::COMMA));
        
        expect(TokenType::RPAREN, "')'");
        
    } else if (match(TokenType::WRITE)) {
        parseLog("WRITE statement");
        
        expect(TokenType::LPAREN, "'('");
        
        do {
            parseExp();
            codeGen.emit(OpCode::WRT, 0, 0); //output the top of stack
        } while (match(TokenType::COMMA));
        
        expect(TokenType::RPAREN, "')'");
        
    } else {
        // Empty statement is allowed (or error)
        if (!check(TokenType::SEMICOLON) && !check(TokenType::END) && 
            !check(TokenType::ELSE) && !check(TokenType::END_OF_FILE)) {
            Token tok = currentToken();
            reportErrorAt(tok, "unexpected token in statement", "expected statement starting with identifier, 'if', 'while', 'call', 'begin', 'read', or 'write'");
        }
    }
    
    parseLogExit("<statement>");
}

// <lexp> -> <exp> <lop> <exp> | odd <exp>
void ParserWithCodegen::parseLexp() {
    parseLogEnter("<condition>");
    
    if (match(TokenType::ODD)) {
        parseLog("ODD operator");
        parseExp();
        codeGen.emit(OpCode::OPR, 0, static_cast<int>(OprType::ODD));
    } else {
        parseExp();
        
        Token opToken = currentToken();
        TokenType relOp = opToken.type;
        
        if (check(TokenType::EQ) || check(TokenType::NEQ) ||
            check(TokenType::LT) || check(TokenType::LEQ) ||
            check(TokenType::GT) || check(TokenType::GEQ)) {
            
            parseLog("Relational operator: " + opToken.value);
            
            advance();
            parseExp();
            
            // Generate comparison instruction
            switch (relOp) {
                case TokenType::EQ:
                    codeGen.emit(OpCode::OPR, 0, static_cast<int>(OprType::EQ));
                    break;
                case TokenType::NEQ:
                    codeGen.emit(OpCode::OPR, 0, static_cast<int>(OprType::NEQ));
                    break;
                case TokenType::LT:
                    codeGen.emit(OpCode::OPR, 0, static_cast<int>(OprType::LT));
                    break;
                case TokenType::LEQ:
                    codeGen.emit(OpCode::OPR, 0, static_cast<int>(OprType::LEQ));
                    break;
                case TokenType::GT:
                    codeGen.emit(OpCode::OPR, 0, static_cast<int>(OprType::GT));
                    break;
                case TokenType::GEQ:
                    codeGen.emit(OpCode::OPR, 0, static_cast<int>(OprType::GEQ));
                    break;
                default:
                    break;
            }
        } else {
            reportError("expected relational operator (=, <>, <, <=, >, >=)", "conditions require a comparison");
        }
    }
    
    parseLogExit("<condition>");
}

// <exp> -> [+|-]<term>{<aop><term>}
void ParserWithCodegen::parseExp() {
    parseLogEnter("<expression>");
    
    bool negative = false;
    
    if (match(TokenType::PLUS)) {
        parseLog("Unary +");
    } else if (match(TokenType::MINUS)) {
        parseLog("Unary -");
        negative = true;
    }
    
    parseTerm();
    
    if (negative) {
        codeGen.emit(OpCode::OPR, 0, static_cast<int>(OprType::NEG));
    }
    
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        TokenType op = currentToken().type;
        parseLog("Operator: " + currentToken().value);
        advance();
        parseTerm();
        
        if (op == TokenType::PLUS) {
            codeGen.emit(OpCode::OPR, 0, static_cast<int>(OprType::ADD));
        } else {
            codeGen.emit(OpCode::OPR, 0, static_cast<int>(OprType::SUB));
        }
    }
    
    parseLogExit("<expression>");
}

// <term> -> <factor>{<mop><factor>}
void ParserWithCodegen::parseTerm() {
    parseLogEnter("<term>");
    
    parseFactor();
    
    while (check(TokenType::MULTIPLY) || check(TokenType::DIVIDE)) {
        TokenType op = currentToken().type;
        parseLog("Operator: " + currentToken().value);
        advance();
        parseFactor();
        
        if (op == TokenType::MULTIPLY) {
            codeGen.emit(OpCode::OPR, 0, static_cast<int>(OprType::MUL));
        } else {
            codeGen.emit(OpCode::OPR, 0, static_cast<int>(OprType::DIV));
        }
    }
    
    parseLogExit("<term>");
}

// <factor> -> <id> | <integer> | (<exp>)
void ParserWithCodegen::parseFactor() {
    parseLogEnter("<factor>");
    
    if (check(TokenType::IDENTIFIER)) {
        Token idToken = currentToken();
        std::string name = idToken.value;
        Symbol* sym = symbolTable.lookup(name);
        
        parseLog("Identifier: " + name);
        
        if (!sym) {
            reportErrorAt(idToken, "use of undeclared identifier '" + name + "'", "declare '" + name + "' before use");
        } else {
            int levelDiff = symbolTable.getCurrentLevel() - sym->level;
            
            if (sym->type == SymbolType::CONST) {
                codeGen.emit(OpCode::LIT, 0, sym->address);  // Constant value
            } else if (sym->type == SymbolType::VAR) {
                codeGen.emit(OpCode::LOD, levelDiff, sym->address);  // Variable address
            } else {
                reportErrorAt(idToken, "procedure '" + name + "' cannot be used as a value", "procedures cannot appear in expressions");
            }
        }
        
        advance();
        
    } else if (check(TokenType::INTEGER)) {
        int value = std::stoi(currentToken().value);
        parseLog("Integer: " + std::to_string(value));
        codeGen.emit(OpCode::LIT, 0, value);
        advance();
        
    } else if (match(TokenType::LPAREN)) {
        parseLog("( expression )");
        parseExp();
        expect(TokenType::RPAREN, "')'");
        
    } else {
        Token tok = currentToken();
        if (tok.type == TokenType::END_OF_FILE) {
            reportError("unexpected end of file in expression", "expression is incomplete");
        } else {
            reportErrorAt(tok, "expected expression (identifier, number, or '(')", "found '" + tok.value + "' which cannot start an expression");
        }
    }
    
    parseLogExit("<factor>");
}

// Main Parse Entry Point

bool ParserWithCodegen::parse() {
    if (options.showParseTree) {
        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "                    PARSE TREE\n";
        std::cout << std::string(50, '=') << "\n\n";
    }
    
    parseProgram();
    
    if (options.showParseTree) {
        std::cout << "\n" << std::string(50, '=') << "\n";
    }
    
    return !hasErrorFlag;
}
