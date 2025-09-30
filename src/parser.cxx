#include "parser.h"

#include <string>

namespace void_compiler {

std::unique_ptr<FunctionDeclaration> Parser::parse() {
  return parse_function();
}

Token& Parser::peek() {
  if (current_ >= tokens_.size()) {
    throw std::runtime_error("Unexpected end of input");
  }
  return tokens_[current_];
}

Token Parser::consume(TokenType expected) {
  if (peek().type != expected) {
    throw std::runtime_error("Expected token type, got: " + peek().value);
  }
  return tokens_[current_++];
}

bool Parser::match(TokenType type) const {
  if (current_ >= tokens_.size()) return false;
  return tokens_[current_].type == type;
}

std::unique_ptr<ASTNode> Parser::parse_expression() {
  if (match(TokenType::Number)) {
    int value = std::stoi(consume(TokenType::Number).value);
    return std::make_unique<NumberLiteral>(value);
  }

  throw std::runtime_error("Expected expression");
}

std::unique_ptr<ASTNode> Parser::parse_statement() {
  if (match(TokenType::Return)) {
    consume(TokenType::Return);
    auto expr = parse_expression();
    return std::make_unique<ReturnStatement>(std::move(expr));
  }

  throw std::runtime_error("Expected statement");
}

std::unique_ptr<FunctionDeclaration> Parser::parse_function() {
  consume(TokenType::Const);
  std::string name = consume(TokenType::Identifier).value;
  consume(TokenType::Equals);
  consume(TokenType::Fn);
  consume(TokenType::LParen);
  consume(TokenType::RParen);
  consume(TokenType::Arrow);
  std::string return_type = consume(TokenType::I32).value;
  consume(TokenType::LBrace);

  auto func = std::make_unique<FunctionDeclaration>(name, return_type);

  while (!match(TokenType::RBrace)) {
    func->add_statement(parse_statement());
  }

  consume(TokenType::RBrace);
  return func;
}

}  // namespace void_compiler
