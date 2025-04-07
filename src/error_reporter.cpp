#include "../include/error_reporter.h"
#include <sstream>

ErrorReporter::ErrorReporter(const std::string& source) {
    splitSourceIntoLines(source);
}

void ErrorReporter::report(ErrorSeverity severity, int line, int column,
                           const std::string& message, const std::string& suggestion) {
    ErrorMessage error = {
        .severity = severity,
        .line = line,
        .column = column,
        .message = message,
        .suggestion = suggestion
    };

    errors.push_back(error);

    if (severity == ErrorSeverity::ERROR || severity == ErrorSeverity::FATAL) {
        errors_present = true;
    } else if (severity == ErrorSeverity::WARNING) {
        warnings_present = true;
    }
}

void ErrorReporter::displayErrors(std::ostream& out) {
    for (const auto& error : errors) {
        out << severityToString(error.severity) << " at line " << error.line
            << ", column " << error.column << ": " << error.message << std::endl;

        // Show source code context if available
        std::string context = getSourceLine(error.line);
        if (!context.empty()) {
            out << context << std::endl;
            out << std::string(error.column - 1, ' ') << "^" << std::endl;
        }

        // Show suggestion if available
        if (!error.suggestion.empty()) {
            out << "Suggestion: " << error.suggestion << std::endl;
        }
        out << std::endl;
    }
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
    if (line > 0 && line <= static_cast<int>(sourceLines.size())) {
        return sourceLines[line - 1];
    }
    return "";
}

void ErrorReporter::splitSourceIntoLines(const std::string& source) {
    std::istringstream stream(source);
    std::string line;
    while (std::getline(stream, line)) {
        sourceLines.push_back(line);
    }
}

std::string ErrorReporter::severityToString(ErrorSeverity severity) const {
    switch (severity) {
        case ErrorSeverity::INFO:
            return "Info";
        case ErrorSeverity::WARNING:
            return "Warning";
        case ErrorSeverity::ERROR:
            return "Error";
        case ErrorSeverity::FATAL:
            return "Fatal Error";
        default:
            return "Unknown";
    }
}

std::string ErrorReporter::highlightLocation(int line, int column) const {
    std::string sourceLine = getSourceLine(line);
    if (sourceLine.empty()) return "";

    std::string result = sourceLine + "\n";
    result += std::string(column - 1, ' ') + "^";
    return result;
}
