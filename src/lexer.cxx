#include "lexer.h"

namespace void_compiler {

Token Lexer::next_token() {
  skip_whitespace();

  if (current_char() == '\0') {
    return Token{.type = TokenType::EndOfFile,
                 .value = "",
                 .line = line_,
                 .column = column_};
  }

  if (std::isdigit(current_char())) {
    return Token{.type = TokenType::Number,
                 .value = read_number(),
                 .line = line_,
                 .column = column_};
  }

  if (current_char() == '"') {
    return Token{.type = TokenType::StringLiteral,
                 .value = read_string(),
                 .line = line_,
                 .column = column_};
  }

  if (std::isalpha(current_char()) || current_char() == '_') {
    std::string identifier = read_identifier();

    // Keywords
    if (identifier == "const") {
      return Token{.type = TokenType::Const,
                   .value = identifier,
                   .line = line_,
                   .column = column_};
    }
    if (identifier == "fn") {
      return Token{.type = TokenType::Fn,
                   .value = identifier,
                   .line = line_,
                   .column = column_};
    }
    if (identifier == "return") {
      return Token{.type = TokenType::Return,
                   .value = identifier,
                   .line = line_,
                   .column = column_};
    }
    if (identifier == "i32") {
      return Token{.type = TokenType::I32,
                   .value = identifier,
                   .line = line_,
                   .column = column_};
    }
    if (identifier == "import") {
      return Token{.type = TokenType::Import,
                   .value = identifier,
                   .line = line_,
                   .column = column_};
    }
    if (identifier == "if") {
      return Token{.type = TokenType::If,
                   .value = identifier,
                   .line = line_,
                   .column = column_};
    }
    if (identifier == "else") {
      return Token{.type = TokenType::Else,
                   .value = identifier,
                   .line = line_,
                   .column = column_};
    }
    if (identifier == "and") {
      return Token{.type = TokenType::And,
                   .value = identifier,
                   .line = line_,
                   .column = column_};
    }
    if (identifier == "or") {
      return Token{.type = TokenType::Or,
                   .value = identifier,
                   .line = line_,
                   .column = column_};
    }
    if (identifier == "not") {
      return Token{.type = TokenType::Not,
                   .value = identifier,
                   .line = line_,
                   .column = column_};
    }
    if (identifier == "loop") {
      return Token{.type = TokenType::Loop,
                   .value = identifier,
                   .line = line_,
                   .column = column_};
    }
    if (identifier == "in") {
      return Token{.type = TokenType::In,
                   .value = identifier,
                   .line = line_,
                   .column = column_};
    }
    if (identifier == "do") {
      return Token{.type = TokenType::Do,
                   .value = identifier,
                   .line = line_,
                   .column = column_};
    }
    if (identifier == "nil") {
      return Token{.type = TokenType::Nil,
                   .value = identifier,
                   .line = line_,
                   .column = column_};
    }

    return Token{.type = TokenType::Identifier,
                 .value = identifier,
                 .line = line_,
                 .column = column_};
  }

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
    case ',':
      return Token{.type = TokenType::Comma,
                   .value = ",",
                   .line = line_,
                   .column = start_column};
    case ':':
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
      return Token{.type = TokenType::Multiply,
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
    default:
      throw std::runtime_error("Unknown character: " + std::string(1, ch));
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

std::string Lexer::read_string() {
  std::string result;
  advance(); // Skip opening quote
  
  while (current_char() != '"' && current_char() != '\0') {
    if (current_char() == '\\') {
      advance(); // Skip escape character
      switch (current_char()) {
        case 'n': result += '\n'; break;
        case 't': result += '\t'; break;
        case 'r': result += '\r'; break;
        case '\\': result += '\\'; break;
        case '"': result += '"'; break;
        default: result += current_char(); break;
      }
    } else {
      result += current_char();
    }
    advance();
  }
  
  if (current_char() == '"') {
    advance(); // Skip closing quote
  } else {
    throw std::runtime_error("Unterminated string literal");
  }
  
  return result;
}

}  // namespace void_compiler
