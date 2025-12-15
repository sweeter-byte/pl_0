#include "diagnostics.h"
#include <algorithm>
#include <iomanip>

// Global diagnostic engine instance
DiagnosticEngine gDiagnostics;

DiagnosticEngine::DiagnosticEngine() 
    : filename("<input>"), useColors(true), errorCount(0), warningCount(0) {
}

void DiagnosticEngine::setSource(const std::string& source, const std::string& file) {
    filename = file;
    splitSource(source);
    reset();
}

void DiagnosticEngine::splitSource(const std::string& source) {
    sourceLines.clear();
    std::istringstream stream(source);
    std::string line;
    while (std::getline(stream, line)) {
        // Remove trailing \r if present (Windows line endings)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        sourceLines.push_back(line);
    }
    // Ensure at least one empty line
    if (sourceLines.empty()) {
        sourceLines.push_back("");
    }
}

std::string DiagnosticEngine::getLine(int lineNum) const {
    if (lineNum >= 1 && lineNum <= static_cast<int>(sourceLines.size())) {
        return sourceLines[lineNum - 1];
    }
    return "";
}

const std::string& DiagnosticEngine::getSourceLine(int lineNum) const {
    static const std::string empty;
    if (lineNum >= 1 && lineNum <= static_cast<int>(sourceLines.size())) {
        return sourceLines[lineNum - 1];
    }
    return empty;
}

std::string DiagnosticEngine::getLevelString(DiagnosticLevel level) const {
    switch (level) {
        case DiagnosticLevel::Error:   return "error";
        case DiagnosticLevel::Warning: return "warning";
        case DiagnosticLevel::Note:    return "note";
        default: return "unknown";
    }
}

std::string DiagnosticEngine::getLevelColor(DiagnosticLevel level) const {
    if (!useColors) return "";
    switch (level) {
        case DiagnosticLevel::Error:   return Color::BoldRed;
        case DiagnosticLevel::Warning: return Color::BoldYellow;
        case DiagnosticLevel::Note:    return Color::BoldCyan;
        default: return "";
    }
}

// Calculate display width up to a certain column (handles UTF-8)
int DiagnosticEngine::getDisplayWidth(const std::string& str, int maxCol) const {
    int width = 0;
    int col = 1;
    size_t i = 0;
    
    while (i < str.size() && col < maxCol) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        
        if (c == '\t') {
            // Tab expands to next multiple of 8
            width = ((width / 8) + 1) * 8;
            i++;
            col++;
        } else if ((c & 0x80) == 0) {
            // ASCII character
            width++;
            i++;
            col++;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte UTF-8
            width += 1;
            i += 2;
            col++;
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte UTF-8 (includes CJK)
            width += 2;  // CJK characters are typically double-width
            i += 3;
            col++;
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte UTF-8
            width += 2;
            i += 4;
            col++;
        } else {
            // Invalid UTF-8 or continuation byte, skip
            width++;
            i++;
            col++;
        }
    }
    
    return width;
}

std::string DiagnosticEngine::buildCaret(int column, int length) const {
    if (column < 1) column = 1;
    if (length < 1) length = 1;
    
    std::string caret;
    // Add spaces before the caret
    for (int i = 1; i < column; i++) {
        caret += ' ';
    }
    
    // Add the caret
    if (useColors) {
        caret += Color::BoldGreen;
    }
    caret += '^';
    
    // Add tildes for the rest of the range
    for (int i = 1; i < length; i++) {
        caret += '~';
    }
    
    if (useColors) {
        caret += Color::Reset;
    }
    
    return caret;
}

void DiagnosticEngine::report(const Diagnostic& diag) {
    // Update counts
    if (diag.level == DiagnosticLevel::Error) {
        errorCount++;
    } else if (diag.level == DiagnosticLevel::Warning) {
        warningCount++;
    }
    
    std::ostream& out = std::cerr;
    
    // Line 1: filename:line:column: level: message
    out << color(Color::BoldWhite) << filename << ":"
        << diag.location.line << ":" << diag.location.column << ": "
        << color(Color::Reset);
    
    out << getLevelColor(diag.level)
        << getLevelString(diag.level) << ": "
        << color(Color::Reset);
    
    out << color(Color::BoldWhite) << diag.message << color(Color::Reset) << "\n";
    
    // Line 2: Source code line
    std::string sourceLine = getLine(diag.location.line);
    if (!sourceLine.empty() || diag.location.line <= static_cast<int>(sourceLines.size())) {
        // Print line number with padding
        int lineNumWidth = 5;
        out << color(Color::Blue)
            << std::setw(lineNumWidth) << diag.location.line << " | "
            << color(Color::Reset);
        
        // Print the source line, replacing tabs with spaces for alignment
        for (char c : sourceLine) {
            if (c == '\t') {
                out << "    ";  // 4 spaces for tab
            } else {
                out << c;
            }
        }
        out << "\n";
        
        // Line 3: Caret pointer
        out << std::string(lineNumWidth, ' ') << color(Color::Blue) << " | " << color(Color::Reset);
        
        // Calculate position accounting for tabs
        int visualColumn = 0;
        for (int i = 0; i < diag.location.column - 1 && i < static_cast<int>(sourceLine.size()); i++) {
            if (sourceLine[i] == '\t') {
                visualColumn += 4;
            } else {
                visualColumn++;
            }
        }
        
        // Print spaces and caret
        out << std::string(visualColumn, ' ');
        out << color(Color::BoldGreen) << "^";
        
        // Print tildes for length > 1
        for (int i = 1; i < diag.location.length; i++) {
            out << "~";
        }
        out << color(Color::Reset) << "\n";
    }
    
    // Line 4: Suggestion (if any)
    if (!diag.suggestion.empty()) {
        out << std::string(5, ' ') << color(Color::Blue) << " | " << color(Color::Reset);
        out << color(Color::BoldGreen) << "help: " << color(Color::Reset)
            << diag.suggestion << "\n";
    }
    
    // Line 5: Suggested fix code (if any)
    if (!diag.suggestionCode.empty()) {
        out << std::string(5, ' ') << color(Color::Blue) << " | " << color(Color::Reset);
        out << color(Color::Cyan) << "try: " << color(Color::Reset)
            << color(Color::Bold) << diag.suggestionCode << color(Color::Reset) << "\n";
    }
    
    out << "\n";
}

void DiagnosticEngine::error(const SourceLocation& loc, const std::string& message) {
    report(Diagnostic(DiagnosticLevel::Error, loc, message));
}

void DiagnosticEngine::error(int line, int col, const std::string& message) {
    error(SourceLocation(line, col), message);
}

void DiagnosticEngine::warning(const SourceLocation& loc, const std::string& message) {
    report(Diagnostic(DiagnosticLevel::Warning, loc, message));
}

void DiagnosticEngine::warning(int line, int col, const std::string& message) {
    warning(SourceLocation(line, col), message);
}

void DiagnosticEngine::note(const SourceLocation& loc, const std::string& message) {
    report(Diagnostic(DiagnosticLevel::Note, loc, message));
}

void DiagnosticEngine::note(int line, int col, const std::string& message) {
    note(SourceLocation(line, col), message);
}

void DiagnosticEngine::errorExpected(int line, int col, const std::string& expected, const std::string& found) {
    std::string msg = "expected " + expected;
    if (!found.empty()) {
        msg += ", found '" + found + "'";
    }
    
    Diagnostic diag(DiagnosticLevel::Error, SourceLocation(line, col), msg);
    
    // Add helpful suggestions for common cases
    if (expected == "';'" && found == "begin") {
        diag.withSuggestion("add ';' before 'begin'");
    } else if (expected == "'end'" && found == "EOF") {
        diag.withSuggestion("missing 'end' to close the block");
    } else if (expected == "':='") {
        diag.withSuggestion("use ':=' for assignment in PL/0");
    }
    
    report(diag);
}

void DiagnosticEngine::errorUndeclared(int line, int col, const std::string& name, const std::string& kind) {
    std::string msg = "use of undeclared " + kind + " '" + name + "'";
    Diagnostic diag(DiagnosticLevel::Error, SourceLocation(line, col, static_cast<int>(name.length())), msg);
    diag.withSuggestion("declare '" + name + "' before use with 'var' or 'const'");
    report(diag);
}

void DiagnosticEngine::errorRedeclared(int line, int col, const std::string& name) {
    std::string msg = "redeclaration of '" + name + "'";
    Diagnostic diag(DiagnosticLevel::Error, SourceLocation(line, col, static_cast<int>(name.length())), msg);
    diag.withSuggestion("'" + name + "' was already declared in this scope");
    report(diag);
}

void DiagnosticEngine::errorTypeMismatch(int line, int col, const std::string& name, 
                                          const std::string& expected, const std::string& found) {
    std::string msg = "'" + name + "' is a " + found + ", not a " + expected;
    Diagnostic diag(DiagnosticLevel::Error, SourceLocation(line, col, static_cast<int>(name.length())), msg);
    report(diag);
}

void DiagnosticEngine::reset() {
    errorCount = 0;
    warningCount = 0;
}

void DiagnosticEngine::printSummary() const {
    std::ostream& out = std::cerr;
    
    if (errorCount == 0 && warningCount == 0) {
        return;
    }
    
    if (errorCount > 0) {
        out << color(Color::BoldRed) << errorCount << " error" 
            << (errorCount > 1 ? "s" : "") << color(Color::Reset);
    }
    
    if (errorCount > 0 && warningCount > 0) {
        out << " and ";
    }
    
    if (warningCount > 0) {
        out << color(Color::BoldYellow) << warningCount << " warning"
            << (warningCount > 1 ? "s" : "") << color(Color::Reset);
    }
    
    out << " generated.\n";
}
