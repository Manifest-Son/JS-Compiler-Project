#include "../include/error_reporter.h"
#include <sstream>
#include <algorithm>
#include <iomanip>

ErrorReporter::ErrorReporter(const std::string& source) {
    splitSourceIntoLines(source);
}

void ErrorReporter::splitSourceIntoLines(const std::string& source) {
    std::istringstream stream(source);
    std::string line;
    while (std::getline(stream, line)) {
        sourceLines.push_back(line);
    }
}

void ErrorReporter::report(ErrorSeverity severity, int line, int column, 
                          const std::string& message, const std::string& suggestion) {
    errors.push_back({severity, line, column, message, suggestion});
    
    if (severity == ErrorSeverity::ERROR || severity == ErrorSeverity::FATAL) {
        errors_present = true;
    } else if (severity == ErrorSeverity::WARNING) {
        warnings_present = true;
    }
}

void ErrorReporter::displayErrors(std::ostream& out) {
    if (errors.empty()) return;
    
    // Sort errors by line and column
    std::sort(errors.begin(), errors.end(), [](const ErrorMessage& a, const ErrorMessage& b) {
        return (a.line < b.line) || (a.line == b.line && a.column < b.column);
    });
    
    for (const auto& error : errors) {
        // Display severity, location and message
        out << severityToString(error.severity) << " at line " << error.line 
            << ", column " << error.column << ": " << error.message << "\n";
        
        // Display source context with highlighting
        out << highlightLocation(error.line, error.column);
        
        // Display suggestion if available
        if (!error.suggestion.empty()) {
            out << "Suggestion: " << error.suggestion << "\n";
        }
        
        out << "\n";
    }
}

std::string ErrorReporter::severityToString(ErrorSeverity severity) const {
    switch (severity) {
        case ErrorSeverity::INFO:    return "\033[36mInfo\033[0m";      // Cyan
        case ErrorSeverity::WARNING: return "\033[33mWarning\033[0m";   // Yellow
        case ErrorSeverity::ERROR:   return "\033[31mError\033[0m";     // Red
        case ErrorSeverity::FATAL:   return "\033[1;31mFatal\033[0m";   // Bold Red
        default: return "Unknown";
    }
}

std::string ErrorReporter::highlightLocation(int line, int column) const {
    if (line <= 0 || line > static_cast<int>(sourceLines.size())) {
        return "  <source line not available>\n";
    }
    
    // Get the source line (adjust for 0-based index)
    std::string sourceLine = sourceLines[line - 1];
    
    std::stringstream result;
    result << "  " << sourceLine << "\n  ";
    
    // Create the pointer marking the error position
    for (int i = 0; i < column - 1; i++) {
        result << " ";
    }
    result << "\033[32m^\033[0m\n";  // Green pointer
    
    return result.str();
}

bool ErrorReporter::hasErrors() const {
    return errors_present;
}

bool ErrorReporter::hasWarnings() const {
    return warnings_present;
}

void ErrorReporter::clear() {
    errors.clear();
    errors_present = false;
    warnings_present = false;
}

std::string ErrorReporter::getSourceLine(int line) const {
    if (line <= 0 || line > static_cast<int>(sourceLines.size())) {
        return "";
    }
    return sourceLines[line - 1];
}
