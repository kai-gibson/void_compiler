#include <iostream>
#include "../include/lexer.h"

int main() {
  void_compiler::Lexer lexer("loop i in 0..10");
  auto tokens = lexer.tokenize();
  
  std::cout << "Generated " << tokens.size() << " tokens:" << std::endl;
  for (size_t i = 0; i < tokens.size(); ++i) {
    std::cout << i << ": " << static_cast<int>(tokens[i].type) << " '" << tokens[i].value << "'" << std::endl;
  }
  
  return 0;
}