#include "../include/lexer.h"
#include <cctype>  // For isdigit, isalpha, etc.
#include <unordered_set> // For keywords

// Javascript Keywords
const std::unordered_set<std::string> keywords = {
    "let", "if", "else", "function", "return", "for", "while", 
    "break", "continue", "true", "false", "null", "const", "var"
};

Lexer::Lexer(const std::string& source) : source(source), position(0), line(1) {}

// Main function to tokenize input
std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;

  while (position < source.length()) {
    char current = peek();
    
    if (isspace(current)) {
      skipWhitespace();
    } else if (isalpha(current) || current == '_') {
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
      // Handle common grouping symbols
      advance();
      tokens.push_back(Token(SYMBOL, std::string(1, current), line));
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

  tokens.push_back(Token(END_OF_FILE, "EOF", line));
  return tokens;
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
    if (peek() == '\n') line++;
    position++;
  }
}

// Handle strings
Token Lexer::string() {
  char quote = advance(); // Skip the opening quote (can be ' or ")
  size_t start = position;

  while (peek() != quote && peek() != '\0') {
    if (peek() == '\n') {
      line++;
    } else if (peek() == '\\' && peekNext() == quote) {
      // Handle escaped quotes
      advance(); // Skip the backslash
      advance(); // Skip the quote
    }

    if (peek() != quote && peek() != '\0') {
      advance();
    }
  }

  if (peek() == '\0') return errorToken("Unterminated string");

  std::string value = source.substr(start, position - start);
  advance(); // Skip the closing quote
  return Token(TokenType::STRING, value, line, value);
}

// Handle identifiers and keywords
Token Lexer::identifier() {
  size_t start = position;
  while (isalnum(peek()) || peek() == '_') advance();

  std::string value = source.substr(start, position - start);
  TokenType type = keywords.count(value) ? TokenType::KEYWORD : TokenType::IDENTIFIER;

  // Set bool value for true/false literals
  if (value == "true") {
    return Token(type, value, line, true);
  } else if (value == "false") {
    return Token(type, value, line, false);
  } else if (value == "null") {
    return Token(type, value, line, std::monostate{});
  }

  return Token(type, value, line);
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

// Handle symbols and operators - renamed from symbol() to operator()
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

  return Token(TokenType::OPERATOR, value, line);
}

// Handle comments (both single-line and multi-line)
Token Lexer::handleComment() {
  advance(); // Skip the first '/'

  if (peek() == '/') {
    // Single-line comment
    advance(); // Skip the second '/'
    size_t start = position;

    while (peek() != '\n' && peek() != '\0') advance();

    std::string comment = source.substr(start, position - start);
    return Token(TokenType::COMMENT, comment, line);
  } else if (peek() == '*') {
    // Multi-line comment
    advance(); // Skip the '*'
    size_t start = position;
    int startLine = line;

    while (!(peek() == '*' && peekNext() == '/') && peek() != '\0') {
      advance();
    }

    if (peek() == '\0') return errorToken("Unterminated multi-line comment");

    std::string comment = source.substr(start, position - start);
    advance(); // Skip the '*'
    advance(); // Skip the '/'

    return Token(TokenType::COMMENT, comment, startLine);
  }

  // If we get here, it's just a division operator
  return Token(TokenType::OPERATOR, "/", line);
}

// Handle errors
Token Lexer::errorToken(const std::string& message) {
  return Token(TokenType::ERROR, message, line);
}

