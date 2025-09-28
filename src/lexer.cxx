#include "lexer.h"

namespace void_compiler {

Token Lexer::next_token() {
  skip_whitespace();

  if (current_char() == '\0') {
    return {TokenType::EndOfFile, "", line_, column_};
  }

  if (std::isdigit(current_char())) {
    return {TokenType::Number, read_number(), line_, column_};
  }

  if (std::isalpha(current_char()) || current_char() == '_') {
    std::string identifier = read_identifier();

    // Keywords
    if (identifier == "const") {
      return {TokenType::Const, identifier, line_, column_};
    }
    if (identifier == "fn") {
      return {TokenType::Fn, identifier, line_, column_};
    }
    if (identifier == "return") {
      return {TokenType::Return, identifier, line_, column_};
    }
    if (identifier == "i32") {
      return {TokenType::I32, identifier, line_, column_};
    }

    return {TokenType::Identifier, identifier, line_, column_};
  }

  char ch = current_char();
  int start_column = column_;
  advance();

  switch (ch) {
    case '=':
      return {TokenType::Equals, "=", line_, start_column};
    case '(':
      return {TokenType::LParen, "(", line_, start_column};
    case ')':
      return {TokenType::RParen, ")", line_, start_column};
    case '{':
      return {TokenType::LBrace, "{", line_, start_column};
    case '}':
      return {TokenType::RBrace, "}", line_, start_column};
    case '-':
      if (current_char() == '>') {
        advance();
        return {TokenType::Arrow, "->", line_, start_column};
      }
      break;
  }

  throw std::runtime_error("Unknown character: " + std::string(1, ch));
}

char Lexer::current_char() const {
  if (position_ >= source_.length()) return '\0';
  return source_[position_];
}

void Lexer::advance() {
  if (current_char() == '\n') {
    line_++;
    column_ = 1;
  } else {
    column_++;
  }
  position_++;
}

void Lexer::skip_whitespace() {
  while (current_char() == ' ' || current_char() == '\t' ||
         current_char() == '\n' || current_char() == '\r') {
    advance();
  }
}

std::string Lexer::read_identifier() {
  std::string result;
  while (std::isalnum(current_char()) || current_char() == '_') {
    result += current_char();
    advance();
  }
  return result;
}

std::string Lexer::read_number() {
  std::string result;
  while (std::isdigit(current_char())) {
    result += current_char();
    advance();
  }
  return result;
}

}  // namespace void_compiler
