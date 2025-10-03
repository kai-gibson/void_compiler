#include "parser.h"

#include <string>

namespace void_compiler {

std::unique_ptr<Program> Parser::parse() {
  auto program = std::make_unique<Program>();

  while (!match(TokenType::EndOfFile)) {
    if (match(TokenType::Import)) {
      program->add_import(parse_import());
    } else if (match(TokenType::Const)) {
      program->add_function(parse_function());
    } else {
      throw std::runtime_error("Expected import or function declaration");
    }
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

std::unique_ptr<ASTNode> Parser::parse_expression() { return parse_logical_or(); }

std::unique_ptr<ASTNode> Parser::parse_logical_or() {
  auto left = parse_logical_and();

  while (match(TokenType::Or)) {
    TokenType op = peek().type;
    consume(op);
    auto right = parse_logical_and();
    left = std::make_unique<BinaryOperation>(std::move(left), op,
                                             std::move(right));
  }

  return left;
}

std::unique_ptr<ASTNode> Parser::parse_logical_and() {
  auto left = parse_comparison();

  while (match(TokenType::And)) {
    TokenType op = peek().type;
    consume(op);
    auto right = parse_comparison();
    left = std::make_unique<BinaryOperation>(std::move(left), op,
                                             std::move(right));
  }

  return left;
}

std::unique_ptr<ASTNode> Parser::parse_comparison() {
  // Handle 'not' at comparison level for proper precedence
  if (match(TokenType::Not)) {
    TokenType op = consume(TokenType::Not).type;
    auto operand = parse_comparison(); // Parse the comparison after 'not'
    return std::make_unique<UnaryOperation>(op, std::move(operand));
  }

  auto left = parse_additive();

  while (match(TokenType::GreaterThan) || match(TokenType::LessThan) ||
         match(TokenType::GreaterEqual) || match(TokenType::LessEqual) ||
         match(TokenType::EqualEqual) || match(TokenType::NotEqual)) {
    TokenType op = peek().type;
    consume(op);
    auto right = parse_additive();
    left = std::make_unique<BinaryOperation>(std::move(left), op,
                                             std::move(right));
  }

  return left;
}

std::unique_ptr<ASTNode> Parser::parse_additive() {
  auto left = parse_multiplicative();

  while (match(TokenType::Plus) || match(TokenType::Minus)) {
    TokenType op = peek().type;
    consume(op);
    auto right = parse_multiplicative();
    left = std::make_unique<BinaryOperation>(std::move(left), op,
                                             std::move(right));
  }

  return left;
}

std::unique_ptr<ASTNode> Parser::parse_multiplicative() {
  auto left = parse_unary();

  while (match(TokenType::Multiply) || match(TokenType::Divide)) {
    TokenType op = peek().type;
    consume(op);
    auto right = parse_unary();
    left = std::make_unique<BinaryOperation>(std::move(left), op,
                                             std::move(right));
  }

  return left;
}

std::unique_ptr<ASTNode> Parser::parse_unary() {
  // Currently no unary operators at this level
  return parse_primary();
}

std::unique_ptr<ASTNode> Parser::parse_primary() {
  if (match(TokenType::Number)) {
    int value = std::stoi(consume(TokenType::Number).value);
    return std::make_unique<NumberLiteral>(value);
  }

  if (match(TokenType::StringLiteral)) {
    std::string value = consume(TokenType::StringLiteral).value;
    return std::make_unique<StringLiteral>(value);
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
    
    // Check for member access (e.g., fmt.println)
    if (match(TokenType::Dot)) {
      consume(TokenType::Dot);
      std::string member_name = consume(TokenType::Identifier).value;
      
      // Member function call
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
        return std::make_unique<MemberAccess>(name, member_name, std::move(arguments));
      }
      
      throw std::runtime_error("Expected function call after member access");
    }
    
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
  
  if (match(TokenType::If)) {
    return parse_if_statement();
  }
  
  if (match(TokenType::Loop)) {
    return parse_loop_statement();
  }
  
  // Check for variable declaration: identifier : type = value
  if (match(TokenType::Identifier) && current_ + 1 < tokens_.size() && 
      tokens_[current_ + 1].type == TokenType::Colon) {
    return parse_variable_declaration();
  }
  
  // Check for variable assignment: identifier = value
  if (match(TokenType::Identifier) && current_ + 1 < tokens_.size() && 
      tokens_[current_ + 1].type == TokenType::Equals) {
    return parse_variable_assignment();
  }
  
  // Check for member access: identifier . member(...)
  if (match(TokenType::Identifier) && current_ + 1 < tokens_.size() && 
      tokens_[current_ + 1].type == TokenType::Dot) {
    return parse_expression();  // Parse as expression, it will be handled as MemberAccess
  }

  throw std::runtime_error("Expected statement");
}

std::unique_ptr<VariableDeclaration> Parser::parse_variable_declaration() {
  std::string name = consume(TokenType::Identifier).value;
  consume(TokenType::Colon);
  std::string type = consume(TokenType::I32).value;  // For now, only support i32
  consume(TokenType::Equals);
  auto value = parse_expression();
  return std::make_unique<VariableDeclaration>(std::move(name), std::move(type), std::move(value));
}

std::unique_ptr<VariableAssignment> Parser::parse_variable_assignment() {
  std::string name = consume(TokenType::Identifier).value;
  consume(TokenType::Equals);
  auto value = parse_expression();
  return std::make_unique<VariableAssignment>(std::move(name), std::move(value));
}

std::unique_ptr<IfStatement> Parser::parse_if_statement() {
  consume(TokenType::If);
  auto condition = parse_expression();
  consume(TokenType::LBrace);
  
  // Parse then body
  std::vector<std::unique_ptr<ASTNode>> then_body;
  while (!match(TokenType::RBrace)) {
    then_body.push_back(parse_statement());
  }
  consume(TokenType::RBrace);
  
  // Parse optional else clause
  std::vector<std::unique_ptr<ASTNode>> else_body;
  if (match(TokenType::Else)) {
    consume(TokenType::Else);
    
    // Handle "else if" by recursively parsing another if statement
    if (match(TokenType::If)) {
      else_body.push_back(parse_if_statement());
    } else {
      // Handle regular else clause
      consume(TokenType::LBrace);
      while (!match(TokenType::RBrace)) {
        else_body.push_back(parse_statement());
      }
      consume(TokenType::RBrace);
    }
  }
  
  return std::make_unique<IfStatement>(std::move(condition), std::move(then_body), std::move(else_body));
}

std::unique_ptr<ImportStatement> Parser::parse_import() {
  consume(TokenType::Import);
  std::string module_name = consume(TokenType::Identifier).value;
  return std::make_unique<ImportStatement>(std::move(module_name));
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
  
  std::string return_type;
  if (match(TokenType::Arrow)) {
    consume(TokenType::Arrow);
    return_type = consume(TokenType::I32).value;
  } else {
    return_type = "void";  // Default to void if no return type specified
  }
  
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

std::unique_ptr<LoopStatement> Parser::parse_loop_statement() {
  consume(TokenType::Loop);
  
  // Check if it's a conditional loop: loop if condition { ... }
  if (match(TokenType::If)) {
    consume(TokenType::If);
    auto condition = parse_expression();
    consume(TokenType::LBrace);
    
    std::vector<std::unique_ptr<ASTNode>> body;
    while (!match(TokenType::RBrace)) {
      body.push_back(parse_statement());
    }
    consume(TokenType::RBrace);
    
    return std::make_unique<LoopStatement>(std::move(condition), std::move(body));
  }
  
  // Otherwise it's a range loop: loop i in 0..10 { ... }
  std::string variable_name = consume(TokenType::Identifier).value;
  consume(TokenType::In);
  auto range = parse_range_expression();
  consume(TokenType::LBrace);
  
  std::vector<std::unique_ptr<ASTNode>> body;
  while (!match(TokenType::RBrace)) {
    body.push_back(parse_statement());
  }
  consume(TokenType::RBrace);
  
  return std::make_unique<LoopStatement>(variable_name, std::move(range), std::move(body));
}

std::unique_ptr<RangeExpression> Parser::parse_range_expression() {
  auto start = parse_additive();  // Parse the start expression
  consume(TokenType::DotDot);
  auto end = parse_additive();    // Parse the end expression
  
  return std::make_unique<RangeExpression>(std::move(start), std::move(end));
}

}  // namespace void_compiler
