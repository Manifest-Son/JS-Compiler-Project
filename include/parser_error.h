#ifndef PARSER_ERROR_H
#define PARSER_ERROR_H

#include <stdexcept>
#include <string>
#include "../include/token.h"

class ParserError : public std::runtime_error {
public:
    int line;
    int column;
    std::string suggestion;

    explicit ParserError(const std::string& message, int line = 0, int column = 0, 
                        const std::string& suggestion = "") 
        : std::runtime_error(message), line(line), column(column), suggestion(suggestion) {}

    ParserError(const Token& token, const std::string& message, 
                const std::string& suggestion = "")
        : std::runtime_error(message), line(token.line), column(0), suggestion(suggestion) {}
};

#endif //PARSER_ERROR_H
