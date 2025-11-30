#ifndef LEXER_H
#define LEXER_H

#include <string>

#include "types.h"

namespace void_compiler {

// Lexer
class Lexer {
 public:
  explicit Lexer(std::string source) : source_(std::move(source)) {}

  Token next_token();

 private:
  [[nodiscard]] char current_char() const;
  [[nodiscard]] char peek_char() const;
  void advance();
  void skip_whitespace();
  std::string read_identifier();
  std::string read_number();
  std::string read_string();
  inline Token make_token(TokenType token_type, std::string value);
  Token map_identifier(const std::string& identifier);

  std::string source_;
  size_t position_{0};
  int line_{1};
  int column_{1};
};

}  // namespace void_compiler
#endif  // LEXER_H
