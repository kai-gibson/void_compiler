#include "lexer.h"

namespace void_compiler {

Token Lexer::next_token() {
  skip_whitespace();

  if (current_char() == '\0') return make_token(TokenType::EndOfFile, "");

  if (std::isdigit(current_char())) {
    return make_token(TokenType::Number, read_number());
  }

  if (current_char() == '"') {
    return make_token(TokenType::StringLiteral, read_string());
  }

  if (std::isalpha(current_char()) || current_char() == '_') {
    std::string identifier = read_identifier();
    return map_identifier(identifier);
  }

  // handle symbol tokens
  char ch = current_char();
  int start_column = column_;
  advance();

  switch (ch) {
    case '=':
      if (current_char() == '=') {
        advance();
        return Token{.type = TokenType::EqualEqual,
                     .value = "==",
                     .line = line_,
                     .column = start_column};
      }
      return Token{.type = TokenType::Equals,
                   .value = "=",
                   .line = line_,
                   .column = start_column};
    case '>':
      if (current_char() == '=') {
        advance();
        return Token{.type = TokenType::GreaterEqual,
                     .value = ">=",
                     .line = line_,
                     .column = start_column};
      }
      return Token{.type = TokenType::GreaterThan,
                   .value = ">",
                   .line = line_,
                   .column = start_column};
    case '<':
      if (current_char() == '=') {
        advance();
        return Token{.type = TokenType::LessEqual,
                     .value = "<=",
                     .line = line_,
                     .column = start_column};
      }
      return Token{.type = TokenType::LessThan,
                   .value = "<",
                   .line = line_,
                   .column = start_column};
    case '!':
      if (current_char() == '=') {
        advance();
        return Token{.type = TokenType::NotEqual,
                     .value = "!=",
                     .line = line_,
                     .column = start_column};
      }
      throw std::runtime_error("Unknown character: !");
    case '(':
      return Token{.type = TokenType::LParen,
                   .value = "(",
                   .line = line_,
                   .column = start_column};
    case ')':
      return Token{.type = TokenType::RParen,
                   .value = ")",
                   .line = line_,
                   .column = start_column};
    case '{':
      return Token{.type = TokenType::LBrace,
                   .value = "{",
                   .line = line_,
                   .column = start_column};
    case '}':
      return Token{.type = TokenType::RBrace,
                   .value = "}",
                   .line = line_,
                   .column = start_column};
    case '[':
      return Token{.type = TokenType::LBracket,
                   .value = "[",
                   .line = line_,
                   .column = start_column};
    case ']':
      return Token{.type = TokenType::RBracket,
                   .value = "]",
                   .line = line_,
                   .column = start_column};
    case ',':
      return Token{.type = TokenType::Comma,
                   .value = ",",
                   .line = line_,
                   .column = start_column};
    case ':':
      if (current_char() == '=') {
        advance();  // consume the '='
        return Token{.type = TokenType::ColonEquals,
                     .value = ":=",
                     .line = line_,
                     .column = start_column};
      }
      return Token{.type = TokenType::Colon,
                   .value = ":",
                   .line = line_,
                   .column = start_column};
    case '+':
      return Token{.type = TokenType::Plus,
                   .value = "+",
                   .line = line_,
                   .column = start_column};
    case '*':
      return Token{.type = TokenType::Asterisk,
                   .value = "*",
                   .line = line_,
                   .column = start_column};
    case '/':
      return Token{.type = TokenType::Divide,
                   .value = "/",
                   .line = line_,
                   .column = start_column};
    case '.':
      if (current_char() == '.') {
        advance();
        return Token{.type = TokenType::DotDot,
                     .value = "..",
                     .line = line_,
                     .column = start_column};
      }
      if (current_char() == '*') {
        advance();
        return Token{.type = TokenType::DotStar,
                     .value = ".*",
                     .line = line_,
                     .column = start_column};
      }
      return Token{.type = TokenType::Dot,
                   .value = ".",
                   .line = line_,
                   .column = start_column};
    case '-':
      if (current_char() == '>') {
        advance();
        return Token{.type = TokenType::Arrow,
                     .value = "->",
                     .line = line_,
                     .column = start_column};
      }
      return Token{.type = TokenType::Minus,
                   .value = "-",
                   .line = line_,
                   .column = start_column};
    case '&':
      return Token{.type = TokenType::Borrow,
                   .value = "&",
                   .line = line_,
                   .column = start_column};
    default:
      throw std::runtime_error("Unknown character: " + std::string(1, ch));
  }

  throw std::runtime_error("Unknown character: " + std::string(1, ch));
}

char Lexer::current_char() const {
  if (position_ >= source_.size()) {
    return '\0';
  }
  return source_[position_];
}

char Lexer::peek_char() const {
  if (position_ + 1 >= source_.size()) {
    return '\0';
  }
  return source_[position_ + 1];
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
  while (true) {
    // Skip regular whitespace
    while (current_char() == ' ' || current_char() == '\t' ||
           current_char() == '\n' || current_char() == '\r') {
      advance();
    }

    // Skip single-line comments
    if (current_char() == '/' && peek_char() == '/') {
      // Skip until end of line
      while (current_char() != '\n' && current_char() != '\0') {
        advance();
      }
      // Continue the loop to skip any whitespace after the comment
    } else {
      break;  // No more whitespace or comments to skip
    }
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

std::string Lexer::read_string() {
  std::string result;
  advance();  // Skip opening quote

  while (current_char() != '"' && current_char() != '\0') {
    if (current_char() == '\\') {
      advance();  // Skip escape character
      switch (current_char()) {
        case 'n':
          result += '\n';
          break;
        case 't':
          result += '\t';
          break;
        case 'r':
          result += '\r';
          break;
        case '\\':
          result += '\\';
          break;
        case '"':
          result += '"';
          break;
        default:
          result += current_char();
          break;
      }
    } else {
      result += current_char();
    }
    advance();
  }

  if (current_char() == '"') {
    advance();  // Skip closing quote
  } else {
    throw std::runtime_error("Unterminated string literal");
  }

  return result;
}

inline Token Lexer::make_token(TokenType token_type, std::string value) {
  return Token{.type = token_type,
               .value = std::move(value),
               .line = line_,
               .column = column_};
}

Token Lexer::map_identifier(const std::string& identifier) {
  // default to identifier if no keyword matches
  TokenType found_type = TokenType::Identifier;

  // Keywords
  if (identifier == "const") {
    found_type = TokenType::Const;
  } else if (identifier == "fn") {
    found_type = TokenType::Fn;
  } else if (identifier == "return") {
    found_type = TokenType::Return;
  } else if (identifier == "i8") {
    found_type = TokenType::I8;
  } else if (identifier == "i16") {
    found_type = TokenType::I16;
  } else if (identifier == "i32") {
    found_type = TokenType::I32;
  } else if (identifier == "i64") {
    found_type = TokenType::I64;
  } else if (identifier == "u8") {
    found_type = TokenType::U8;
  } else if (identifier == "u16") {
    found_type = TokenType::U16;
  } else if (identifier == "u32") {
    found_type = TokenType::U32;
  } else if (identifier == "u64") {
    found_type = TokenType::U64;
  } else if (identifier == "bool") {
    found_type = TokenType::Bool;
  } else if (identifier == "true") {
    found_type = TokenType::True;
  } else if (identifier == "false") {
    found_type = TokenType::False;
  } else if (identifier == "import") {
    found_type = TokenType::Import;
  } else if (identifier == "if") {
    found_type = TokenType::If;
  } else if (identifier == "else") {
    found_type = TokenType::Else;
  } else if (identifier == "and") {
    found_type = TokenType::And;
  } else if (identifier == "or") {
    found_type = TokenType::Or;
  } else if (identifier == "not") {
    found_type = TokenType::Not;
  } else if (identifier == "loop") {
    found_type = TokenType::Loop;
  } else if (identifier == "in") {
    found_type = TokenType::In;
  } else if (identifier == "do") {
    found_type = TokenType::Do;
  } else if (identifier == "void") {
    found_type = TokenType::Void;
  } else if (identifier == "string") {
    found_type = TokenType::String;
  } else if (identifier == "nil") {
    found_type = TokenType::Nil;
  }

  return make_token(found_type, identifier);
}

}  // namespace void_compiler
