#ifndef LEXER_H
#define LEXER_H

#include <string>

#include "types.h"

namespace void_compiler {

// Lexer
class Lexer {
 public:
  explicit Lexer(const std::string& source)
      : source_(source), position_(0), line_(1), column_(1) {}

  Token next_token();

 private:
  char current_char() const;
  void advance();
  void skip_whitespace();
  std::string read_identifier();
  std::string read_number();

  std::string source_;
  size_t position_;
  int line_;
  int column_;
};

}  // namespace void_compiler
#endif  // LEXER_H
