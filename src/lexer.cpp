#include "../include/lexer.h"
#include <cctype>  //For isdigit, isalpha, etc.
#include <unordered_set> //For keywords

//Javascript Keywords
const std::unordered_set<std::string> keywords = {"let", "if", "else", "function", "return", "for", "while", "break", "continue", "true", "false", "null"};

Lexer::Lexer(const std::string& source) : source(source), position(0), line(1) {}

//Main function to tokenize input
std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;

  while (position < source.length()) {
    char current = peek();
    if (isspace(current)) {
      skipWhitespace();
      continue;
      }
      else if(isalpha(current) || current == '_'){
        tokens.push_back(identifier());
      }
      else if(isdigit(current)){
        tokens.push_back(number());
      }
      else if(current == '"'){
        tokens.push_back(string());
      }
      else if(ispunct(current)){
        tokens.push_back(symbol());
      }
      else if(current == '(' || current == ')' || current == '{' || current == '}' || current == '[' || current == ']'){
        tokens.push_back(symbol());
      }
      else if(current == ';'){
        advance();
      }
      else{
        tokens.push_back(errorToken("Unknown token"));
        advance();
      }
  }
  tokens.push_back(Token(END_OF_FILE, "EOF", line));
  return tokens;
 }

 //Look at the current character without advancing
char Lexer::peek(){
  return (position < source.length()) ? source[position] : '\0';
}

char Lexer::advance(){
 return (position < source.length()) ? source[position++] : '\0';
}

//Skip whitespace characters and track new lines
void Lexer::skipWhitespace(){
while (position <source.length() && isspace(peek())){
  if (source[position] == '\n') line++;
  position++;
}
}

//Handle strings
Token Lexer::string() {
    advance();
    size_t start = position;

    while (peek() != '"' && peek() != '\0') advance();
      if (peek() == '\0') return errorToken("Unterminated string");{

        std::string value = source.substr(start, position - start);
        advance();
        return Token(TokenType::STRING, value, line);
    }
  }


//Handle identifiers and keywords
Token Lexer::identifier(){
size_t start = position;
while (isalnum(peek()) || peek() == '_') advance();

std::string value = source.substr(start, position - start);
TokenType type = keywords.count(value) ? TokenType::KEYWORD : TokenType::IDENTIFIER;
return Token(type, value, line);
}

//Handle numbers (integers and floats)
Token Lexer::number(){
size_t start = position;
while (isdigit(peek())) advance();

//Handle decimal point
if (peek() == '.' && isdigit(source[position + 1])){
  advance();
  while (isdigit(peek())) advance();
}

return Token(TokenType::NUMBER, source.substr(start, position - start), line);
}

//Handle symbols and operators
Token Lexer::symbol(){
std::string value(1, advance());
return Token(TokenType::SYMBOL, value, line);

//Handle multi-character operators
if((value[0] == '=' || value[0] == '<' || value[0] == '>') && peek() == '='){
  value += advance();
}
return Token(TokenType::OPERATOR, value, line);
}

//Handle errors
Token Lexer::errorToken(const std::string& message){
return Token(TokenType::ERROR, message, line);
}