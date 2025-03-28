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

  for (std::vector<Token> tokens = lexer.tokenize(); const Token& token : tokens) {
    std::cout << "Token: ";
      std::visit([](const auto& v) {
    if constexpr (!std::is_same_v<std::decay_t<decltype(v)>, std::monostate>) {
        std::cout << v;
    } else {
        std::cout << "null";
    }
}, token.value);

      std::cout << "Line: " << token.line << std::endl;
  }
  return 0;
}
