//
// Created by YVONNE on 3/27/2025.
//

#ifndef PARSER_ERROR_H
#define PARSER_ERROR_H

#include <stdexcept>
#include <string>

class ParserError : public std::runtime_error {
public:
    explicit ParserError(const std::string& message) : std::runtime_error(message) {}
};

#endif //PARSER_ERROR_H
