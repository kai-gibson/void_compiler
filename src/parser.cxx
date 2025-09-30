#include "parser.h"

#include <string>

namespace void_compiler {

std::unique_ptr<Program> Parser::parse() {
  auto program = std::make_unique<Program>();

  while (!match(TokenType::EndOfFile)) {
    program->add_function(parse_function());
  }

  return program;
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
  return parse_additive();
}

std::unique_ptr<ASTNode> Parser::parse_additive() {
  auto left = parse_multiplicative();

  while (match(TokenType::Plus) || match(TokenType::Minus)) {
    TokenType op = peek().type;
    consume(op);
    auto right = parse_multiplicative();
    left = std::make_unique<BinaryOperation>(std::move(left), op, std::move(right));
  }

  return left;
}

std::unique_ptr<ASTNode> Parser::parse_multiplicative() {
  auto left = parse_primary();

  while (match(TokenType::Multiply) || match(TokenType::Divide)) {
    TokenType op = peek().type;
    consume(op);
    auto right = parse_primary();
    left = std::make_unique<BinaryOperation>(std::move(left), op, std::move(right));
  }

  return left;
}

std::unique_ptr<ASTNode> Parser::parse_primary() {
  if (match(TokenType::Number)) {
    int value = std::stoi(consume(TokenType::Number).value);
    return std::make_unique<NumberLiteral>(value);
  }

  if (match(TokenType::LParen)) {
    consume(TokenType::LParen);
    auto expr = parse_expression();
    consume(TokenType::RParen);
    return expr;
  }

  // Parse function calls and variable references
  if (match(TokenType::Identifier)) {
    std::string name = consume(TokenType::Identifier).value;
    if (match(TokenType::LParen)) {
      consume(TokenType::LParen);
      std::vector<std::unique_ptr<ASTNode>> arguments;

      // Parse arguments
      if (!match(TokenType::RParen)) {
        do {
          arguments.push_back(parse_expression());
        } while (match(TokenType::Comma) && (consume(TokenType::Comma), true));
      }

      consume(TokenType::RParen);
      return std::make_unique<FunctionCall>(name, std::move(arguments));
    }
    // It's a variable reference
    return std::make_unique<VariableReference>(name);
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

  // Parse parameters first, store them temporarily
  std::vector<std::unique_ptr<Parameter>> parameters;
  if (!match(TokenType::RParen)) {
    do {
      std::string param_name = consume(TokenType::Identifier).value;
      consume(TokenType::Colon);
      std::string param_type = consume(TokenType::I32).value;
      parameters.push_back(std::make_unique<Parameter>(param_name, param_type));
    } while (match(TokenType::Comma) && (consume(TokenType::Comma), true));
  }

  consume(TokenType::RParen);
  consume(TokenType::Arrow);
  std::string return_type = consume(TokenType::I32).value;
  consume(TokenType::LBrace);

  // Create function with return type
  auto func = std::make_unique<FunctionDeclaration>(name, return_type);
  
  // Add all parameters
  for (auto& param : parameters) {
    func->add_parameter(std::move(param));
  }

  // Parse function body
  while (!match(TokenType::RBrace)) {
    func->add_statement(parse_statement());
  }

  consume(TokenType::RBrace);
  return func;
}

}  // namespace void_compiler
