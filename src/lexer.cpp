#include "../include/lexer.h"
#include <cctype>  // For isdigit, isalpha, etc.
#include <unordered_set> // For keywords
#include <unordered_map> // For common error suggestions

// Complete list of Javascript Keywords and Reserved Words
const std::unordered_set<std::string> keywords = {
    // Keywords
    "break", "case", "catch", "class", "const", "continue", "debugger",
    "default", "delete", "do", "else", "export", "extends", "finally",
    "for", "function", "if", "import", "in", "instanceof", "new",
    "return", "super", "switch", "this", "throw", "try", "typeof",
    "var", "void", "while", "with", "yield",

    // Future reserved words
    "enum", "implements", "interface", "let", "package", "private",
    "protected", "public", "static",

    // Literals
    "true", "false", "null", "undefined",

    // Special identifiers
    "arguments", "eval", "async", "await"
};

// Map of common errors to suggestions
const std::unordered_map<std::string, std::string> errorSuggestions = {
    {"Unterminated string", "Add matching quote to close the string"},
    {"Unterminated multi-line comment", "Add */ to close the comment"},
    {"Invalid number format", "Check decimal point usage and digit formatting"},
    {"Invalid identifier", "Identifiers must start with a letter, underscore, or dollar sign"}
};

Lexer::Lexer(const std::string& source) 
    : source(source), position(0), line(1), column(1), errorReporter(source) {}

// Check if character is valid for starting an identifier
bool isIdentifierStart(char c) {
    return isalpha(c) || c == '_' || c == '$';
}

// Check if character is valid inside an identifier
bool isIdentifierPart(char c) {
    return isalnum(c) || c == '_' || c == '$';
}

// Main function to tokenize input
std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;

  while (position < source.length()) {
    startToken(); // Track position at start of token
    char current = peek();
    
    if (isspace(current)) {
      skipWhitespace();
    } else if (isIdentifierStart(current)) {
      tokens.push_back(identifier());
    } else if (isdigit(current)) {
      tokens.push_back(number());
    } else if (current == '"' || current == '\'') {
      tokens.push_back(string());
    } else if (current == '/' && (peekNext() == '/' || peekNext() == '*')) {
      // Handle comments
      tokens.push_back(handleComment());
    } else if (current == '(' || current == ')' ||
               current == '{' || current == '}' ||
               current == '[' || current == ']' ||
               current == ',') {
      // Handle common grouping symbols - add explicit value
      advance();
      std::string symbolStr(1, current);
      tokens.push_back(Token(SYMBOL, symbolStr, line, symbolStr));
    } else if (current == ';') {
      advance();
      tokens.push_back(Token(SYMBOL, ";", line, ";"));
    } else if (current == '+' || current == '-' ||
               current == '*' || current == '/' ||
               current == '=' || current == '!' ||
               current == '<' || current == '>' ||
               current == '&' || current == '|') {
      tokens.push_back(symbol());
    } else {
      tokens.push_back(errorToken("Unexpected character: " + std::string(1, current)));
      advance();
    }
  }

  tokens.push_back(Token(END_OF_FILE, "EOF", line, "EOF"));
  return tokens;
}

// Track position at start of token for error reporting
void Lexer::startToken() {
    startLine = line;
    startColumn = column;
}

// Look at the current character without advancing
char Lexer::peek() {
  return (position < source.length()) ? source[position] : '\0';
}

// Look at the next character without advancing
char Lexer::peekNext() {
  return (position + 1 < source.length()) ? source[position + 1] : '\0';
}

// Move to next character
char Lexer::advance() {
  if (position < source.length() && source[position] == '\n') {
    line++;
    column = 1;
  } else {
    column++;
  }
  return (position < source.length()) ? source[position++] : '\0';
}

// Match current character with expected
bool Lexer::match(char expected) {
  if (peek() != expected) return false;
  advance();
  return true;
}

// Skip whitespace characters and track new lines
void Lexer::skipWhitespace() {
  while (position < source.length() && isspace(peek())) {
    if (peek() == '\n') {
      line++;
      column = 1;
    } else {
      column++;
    }
    position++;
  }
}

// Handle strings with improved error handling
Token Lexer::string() {
  char quote = advance(); // Skip the opening quote (can be ' or ")
  size_t start = position;

  while (peek() != quote && peek() != '\0') {
    if (peek() == '\n') {
      line++;
      column = 1;
    } else if (peek() == '\\' && peekNext() == quote) {
      // Handle escaped quotes
      advance(); // Skip the backslash
      advance(); // Skip the quote
    }

    if (peek() != quote && peek() != '\0') {
      advance();
    }
  }

  if (peek() == '\0') {
    std::string message = "Unterminated string";
    errorReporter.report(ErrorSeverity::ERROR, startLine, startColumn, 
                         message, errorSuggestions.at("Unterminated string"));
    return errorToken(message);
  }

  std::string value = source.substr(start, position - start);
  advance(); // Skip the closing quote
  return Token(TokenType::STRING, value, startLine, value);
}

// Handle identifiers and keywords
Token Lexer::identifier() {
  size_t start = position;
  while (isIdentifierPart(peek())) advance();

  std::string value = source.substr(start, position - start);
  TokenType type = keywords.count(value) ? TokenType::KEYWORD : TokenType::IDENTIFIER;

  // Set bool value for true/false literals
  if (value == "true") {
    return Token(type, value, line, true);
  } else if (value == "false") {
    return Token(type, value, line, false);
  } else if (value == "null") {
    return Token(type, value, line, std::monostate{});
  } else if (value == "undefined") {
    return Token(type, value, line, std::monostate{});
  }

  // Always provide the value for identifiers
  return Token(type, value, line, value);
}

// Handle numbers (integers and floats)
Token Lexer::number() {
  size_t start = position;
  while (isdigit(peek())) advance();

  // Handle decimal point
  if (peek() == '.' && isdigit(peekNext())) {
    advance(); // Consume the '.'
    while (isdigit(peek())) advance();
  }

  std::string lexeme = source.substr(start, position - start);
  double value = std::stod(lexeme);
  return Token(TokenType::NUMBER, lexeme, line, value);
}

// Handle symbols and operators
Token Lexer::symbol() {
  char current = advance();
  std::string value(1, current);

  // Handle multi-character operators
  if (current == '=' && peek() == '=') {
    advance();
    value += "=";
  } else if (current == '!' && peek() == '=') {
    advance();
    value += "=";
  } else if (current == '<' && peek() == '=') {
    advance();
    value += "=";
  } else if (current == '>' && peek() == '=') {
    advance();
    value += "=";
  } else if (current == '&' && peek() == '&') {
    advance();
    value += "&";
  } else if (current == '|' && peek() == '|') {
    advance();
    value += "|";
  } else if (current == '+' && peek() == '+') {
    advance();
    value += "+";
  } else if (current == '-' && peek() == '-') {
    advance();
    value += "-";
  } else if (current == '+' && peek() == '=') {
    advance();
    value += "=";
  } else if (current == '-' && peek() == '=') {
    advance();
    value += "=";
  } else if (current == '*' && peek() == '=') {
    advance();
    value += "=";
  } else if (current == '/' && peek() == '=') {
    advance();
    value += "=";
  }

  return Token(TokenType::OPERATOR, value, line, value);
}

// Handle comments with improved error handling
Token Lexer::handleComment() {
  advance(); // Skip the first '/'

  if (peek() == '/') {
    // Single-line comment
    advance(); // Skip the second '/'
    size_t start = position;

    while (peek() != '\n' && peek() != '\0') advance();

    std::string comment = source.substr(start, position - start);
    return Token(TokenType::COMMENT, comment, startLine, comment);
  } else if (peek() == '*') {
    // Multi-line comment
    advance(); // Skip the '*'
    size_t start = position;
    int startLine = line;

    while (!(peek() == '*' && peekNext() == '/') && peek() != '\0') {
      advance();
    }

    if (peek() == '\0') {
      std::string message = "Unterminated multi-line comment";
      errorReporter.report(ErrorSeverity::ERROR, this->startLine, startColumn, 
                           message, errorSuggestions.at("Unterminated multi-line comment"));
      return errorToken(message);
    }

    std::string comment = source.substr(start, position - start);
    advance(); // Skip the '*'
    advance(); // Skip the '/'

    return Token(TokenType::COMMENT, comment, startLine, comment);
  }

  // If we get here, it's just a division operator
  return Token(TokenType::OPERATOR, "/", startLine, "/");
}

// Handle errors with better reporting
Token Lexer::errorToken(const std::string& message) {
  // If not already reported by specific handlers
  if (message.find("Unterminated") == std::string::npos) {
    std::string suggestion = "";
    for (const auto& [error, fix] : errorSuggestions) {
      if (message.find(error) != std::string::npos) {
        suggestion = fix;
        break;
      }
    }
    errorReporter.report(ErrorSeverity::ERROR, startLine, startColumn, message, suggestion);
  }
  
  return Token(TokenType::ERROR, message, startLine);
}
