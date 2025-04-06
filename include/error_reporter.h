#ifndef ERROR_REPORTER_H
#define ERROR_REPORTER_H

#include <string>
#include <vector>
#include <iostream>

enum class ErrorSeverity {
    INFO,
    WARNING,
    ERROR,
    FATAL
};

struct ErrorMessage {
    ErrorSeverity severity;
    int line;
    int column;
    std::string message;
    std::string suggestion;
};

class ErrorReporter {
public:
    ErrorReporter(const std::string& source);
    
    void report(ErrorSeverity severity, int line, int column, 
                const std::string& message, const std::string& suggestion = "");
    
    void displayErrors(std::ostream& out = std::cerr);
    
    bool hasErrors() const;
    bool hasWarnings() const;
    
    void clear();
    
    // Get line of source code for context
    std::string getSourceLine(int line) const;
    void getErrorMessages();

private:
    std::vector<ErrorMessage> errors;
    std::vector<std::string> sourceLines;
    bool errors_present = false;
    bool warnings_present = false;
    
    // Helper to split source into lines for easier access
    void splitSourceIntoLines(const std::string& source);
    
    // Helpers for formatting error display
    std::string severityToString(ErrorSeverity severity) const;
    std::string highlightLocation(int line, int column) const;
};

#endif // ERROR_REPORTER_H
