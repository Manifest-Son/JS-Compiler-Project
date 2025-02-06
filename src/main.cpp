#include <iostream>
#include "../include/lexer.h"

int main() {
  const std::string source_code = R"(
    let x = 10;
    let y = 20;
    let sum = x + y;
    function add(a, b){
      return a + b;
    }
    let result = add(x, y);
    print(result);
  )";

  Lexer lexer(source_code);
  std::vector<Token> tokens = lexer.tokenize();

  for (const Token& token : tokens) {
    std::cout << "Token: " << token.value << " Line: " << token.line << std::endl;
  }
  return 0;
}