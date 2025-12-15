#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

// ANSI Color codes
namespace Color {
    constexpr const char* Reset     = "\033[0m";
    constexpr const char* Bold      = "\033[1m";
    constexpr const char* Red       = "\033[31m";
    constexpr const char* Green     = "\033[32m";
    constexpr const char* Yellow    = "\033[33m";
    constexpr const char* Blue      = "\033[34m";
    constexpr const char* Magenta   = "\033[35m";
    constexpr const char* Cyan      = "\033[36m";
    constexpr const char* White     = "\033[37m";
    constexpr const char* BoldRed   = "\033[1;31m";
    constexpr const char* BoldGreen = "\033[1;32m";
    constexpr const char* BoldYellow= "\033[1;33m";
    constexpr const char* BoldCyan  = "\033[1;36m";
    constexpr const char* BoldWhite = "\033[1;37m";
}

// Diagnostic severity levels
enum class DiagnosticLevel {
    Error,
    Warning,
    Note
};

// Source location
struct SourceLocation {
    int line;
    int column;
    int length;  // Length of the highlighted region (for underlines)
    
    SourceLocation() : line(0), column(0), length(1) {}
    SourceLocation(int l, int c, int len = 1) : line(l), column(c), length(len) {}
};

// A single diagnostic message
struct Diagnostic {
    DiagnosticLevel level;
    SourceLocation location;
    std::string message;
    std::string suggestion;      // Optional suggestion/fix-it hint
    std::string suggestionCode;  // Optional code to show as fix
    
    Diagnostic(DiagnosticLevel lvl, const SourceLocation& loc, const std::string& msg)
        : level(lvl), location(loc), message(msg) {}
    
    Diagnostic& withSuggestion(const std::string& sug) {
        suggestion = sug;
        return *this;
    }
    
    Diagnostic& withFix(const std::string& code) {
        suggestionCode = code;
        return *this;
    }
};

// Diagnostic Engine - manages all diagnostics
class DiagnosticEngine {
private:
    std::vector<std::string> sourceLines;  // Source code split by lines
    std::string filename;
    bool useColors;
    int errorCount;
    int warningCount;
    
    // Helper methods
    void splitSource(const std::string& source);
    std::string getLine(int lineNum) const;
    std::string getLevelString(DiagnosticLevel level) const;
    std::string getLevelColor(DiagnosticLevel level) const;
    std::string buildCaret(int column, int length) const;
    int getDisplayWidth(const std::string& str, int maxCol) const;
    
public:
    DiagnosticEngine();
    
    // Initialize with source code
    void setSource(const std::string& source, const std::string& file = "<input>");
    
    // Enable/disable colors
    void setColors(bool enable) { useColors = enable; }
    bool hasColors() const { return useColors; }
    
    // Report diagnostics
    void report(const Diagnostic& diag);
    void error(const SourceLocation& loc, const std::string& message);
    void error(int line, int col, const std::string& message);
    void warning(const SourceLocation& loc, const std::string& message);
    void warning(int line, int col, const std::string& message);
    void note(const SourceLocation& loc, const std::string& message);
    void note(int line, int col, const std::string& message);
    
    // Convenience methods for common errors
    void errorExpected(int line, int col, const std::string& expected, const std::string& found);
    void errorUndeclared(int line, int col, const std::string& name, const std::string& kind = "identifier");
    void errorRedeclared(int line, int col, const std::string& name);
    void errorTypeMismatch(int line, int col, const std::string& name, const std::string& expected, const std::string& found);
    
    // Statistics
    int getErrorCount() const { return errorCount; }
    int getWarningCount() const { return warningCount; }
    bool hasErrors() const { return errorCount > 0; }
    void reset();
    
    // Print summary
    void printSummary() const;
    
    // Get a specific source line (1-indexed)
    const std::string& getSourceLine(int lineNum) const;
    int getLineCount() const { return static_cast<int>(sourceLines.size()); }
    
    // Color helper (returns empty string if colors disabled)
    std::string color(const char* c) const { return useColors ? c : ""; }
};

// Global diagnostic engine (optional, for convenience)
extern DiagnosticEngine gDiagnostics;

#endif // DIAGNOSTICS_H
