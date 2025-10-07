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

  if (match(TokenType::True)) {
    consume(TokenType::True);
    return std::make_unique<BooleanLiteral>(true);
  }

  if (match(TokenType::False)) {
    consume(TokenType::False);
    return std::make_unique<BooleanLiteral>(false);
  }

  if (match(TokenType::LParen)) {
    consume(TokenType::LParen);
    auto expr = parse_expression();
    consume(TokenType::RParen);
    return expr;
  }

  // Parse anonymous functions
  if (match(TokenType::Fn)) {
    return parse_anonymous_function();
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
    // Check if we're at the end of a statement (return without expression)
    // This happens when return is followed by statement terminators or end of input
    if (current_ >= tokens_.size() || 
        match(TokenType::EndOfFile) ||
        match(TokenType::RBrace) ||   // End of block
        match(TokenType::If) ||       // Next statement (if statement)
        match(TokenType::Loop) ||     // Next statement (loop statement)
        match(TokenType::Return) ||   // Next statement (another return)
        match(TokenType::Const)) {    // Next declaration (function declaration)
      // Return without expression for nil functions
      return std::make_unique<ReturnStatement>(nullptr);
    }
    auto expr = parse_expression();
    return std::make_unique<ReturnStatement>(std::move(expr));
  }
  
  if (match(TokenType::If)) {
    return parse_if_statement();
  }
  
  if (match(TokenType::Loop)) {
    return parse_loop_statement();
  }
  
  // Check for variable declaration with explicit type: identifier : type = value
  if (match(TokenType::Identifier) && current_ + 1 < tokens_.size() && 
      tokens_[current_ + 1].type == TokenType::Colon) {
    return parse_variable_declaration();
  }
  
  // Check for variable declaration with type inference: identifier := value
  if (match(TokenType::Identifier) && current_ + 1 < tokens_.size() && 
      tokens_[current_ + 1].type == TokenType::ColonEquals) {
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

  // Check for function call: identifier(...)
  if (match(TokenType::Identifier) && current_ + 1 < tokens_.size() && 
      tokens_[current_ + 1].type == TokenType::LParen) {
    return parse_expression();  // Parse as expression, function call can be a statement
  }

  throw std::runtime_error("Expected statement");
}

std::unique_ptr<VariableDeclaration> Parser::parse_variable_declaration() {
  std::string name = consume(TokenType::Identifier).value;
  
  std::string type;
  std::unique_ptr<ASTNode> value;
  
  if (match(TokenType::ColonEquals)) {
    // Type inference: name := value
    consume(TokenType::ColonEquals);
    value = parse_expression();
    type = infer_type(value.get());
  } else {
    // Explicit type: name: type = value
    consume(TokenType::Colon);
    type = parse_type();
    consume(TokenType::Equals);
    value = parse_expression();
  }
  
  // Add variable to symbol table
  variable_types_[name] = type;
  
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
  
  // Parse then body - check for 'do' or block syntax
  std::vector<std::unique_ptr<ASTNode>> then_body;
  if (match(TokenType::Do)) {
    consume(TokenType::Do);
    // Single expression after 'do'
    then_body.push_back(parse_statement());
  } else {
    consume(TokenType::LBrace);
    // Multiple statements in block
    while (!match(TokenType::RBrace)) {
      then_body.push_back(parse_statement());
    }
    consume(TokenType::RBrace);
  }
  
  // Parse optional else clause
  std::vector<std::unique_ptr<ASTNode>> else_body;
  if (match(TokenType::Else)) {
    consume(TokenType::Else);
    
    // Handle "else if" by recursively parsing another if statement
    if (match(TokenType::If)) {
      else_body.push_back(parse_if_statement());
    } else {
      // Handle regular else clause - check for 'do' or block syntax
      if (match(TokenType::Do)) {
        consume(TokenType::Do);
        // Single expression after 'do'
        else_body.push_back(parse_statement());
      } else {
        consume(TokenType::LBrace);
        while (!match(TokenType::RBrace)) {
          else_body.push_back(parse_statement());
        }
        consume(TokenType::RBrace);
      }
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
      std::string param_type = parse_type();
      parameters.push_back(std::make_unique<Parameter>(param_name, param_type));
    } while (match(TokenType::Comma) && (consume(TokenType::Comma), true));
  }

  consume(TokenType::RParen);
  
  std::string return_type;
  if (match(TokenType::Arrow)) {
    consume(TokenType::Arrow);
    if (match(TokenType::I32)) {
      return_type = consume(TokenType::I32).value;
    } else if (match(TokenType::Bool)) {
      return_type = consume(TokenType::Bool).value;
    } else if (match(TokenType::Nil)) {
      return_type = consume(TokenType::Nil).value;
    } else {
      throw std::runtime_error("Expected return type (i32, bool, or nil) after '->'");
    }
  } else {
    return_type = "nil";  // Default to nil if no return type specified
  }
  
  // Create function with return type
  auto func = std::make_unique<FunctionDeclaration>(name, return_type);

  // Add function to symbol table
  function_return_types_[name] = return_type;

  // Add all parameters
  for (auto& param : parameters) {
    func->add_parameter(std::move(param));
  }

  // Parse function body - check for 'do' or block syntax
  if (match(TokenType::Do)) {
    consume(TokenType::Do);
    // Single statement after 'do'
    func->add_statement(parse_statement());
  } else {
    consume(TokenType::LBrace);
    // Multiple statements in block
    while (!match(TokenType::RBrace)) {
      func->add_statement(parse_statement());
    }
    consume(TokenType::RBrace);
  }

  return func;
}

std::unique_ptr<LoopStatement> Parser::parse_loop_statement() {
  consume(TokenType::Loop);
  
  // Check if it's a conditional loop: loop if condition { ... }
  if (match(TokenType::If)) {
    consume(TokenType::If);
    auto condition = parse_expression();
    
    std::vector<std::unique_ptr<ASTNode>> body;
    if (match(TokenType::Do)) {
      consume(TokenType::Do);
      // Single statement after 'do'
      body.push_back(parse_statement());
    } else {
      consume(TokenType::LBrace);
      while (!match(TokenType::RBrace)) {
        body.push_back(parse_statement());
      }
      consume(TokenType::RBrace);
    }
    
    return std::make_unique<LoopStatement>(std::move(condition), std::move(body));
  }
  
  // Otherwise it's a range loop: loop i in 0..10 { ... }
  std::string variable_name = consume(TokenType::Identifier).value;
  consume(TokenType::In);
  auto range = parse_range_expression();
  
  std::vector<std::unique_ptr<ASTNode>> body;
  if (match(TokenType::Do)) {
    consume(TokenType::Do);
    // Single statement after 'do'
    body.push_back(parse_statement());
  } else {
    consume(TokenType::LBrace);
    while (!match(TokenType::RBrace)) {
      body.push_back(parse_statement());
    }
    consume(TokenType::RBrace);
  }
  
  return std::make_unique<LoopStatement>(variable_name, std::move(range), std::move(body));
}

std::unique_ptr<RangeExpression> Parser::parse_range_expression() {
  auto start = parse_additive();  // Parse the start expression
  consume(TokenType::DotDot);
  auto end = parse_additive();    // Parse the end expression
  
  return std::make_unique<RangeExpression>(std::move(start), std::move(end));
}

std::string Parser::parse_type() {
  if (tokens_[current_].type == TokenType::I32) {
    current_++;
    return "i32";
  } else if (tokens_[current_].type == TokenType::Bool) {
    current_++;
    return "bool";
  } else if (tokens_[current_].type == TokenType::Const) {
    current_++; // consume 'const'
    if (tokens_[current_].type == TokenType::String) {
      current_++; // consume 'string'
      return "const string";
    } else {
      throw std::runtime_error("Expected 'string' after 'const' in type");
    }
  } else if (tokens_[current_].type == TokenType::String) {
    current_++;
    return "string";
  } else if (tokens_[current_].type == TokenType::Fn) {
    // Parse function pointer type: fn(param_types) -> return_type
    current_++; // consume 'fn'
    consume(TokenType::LParen);
    
    std::vector<std::string> param_types;
    if (!match(TokenType::RParen)) {
      do {
        param_types.push_back(parse_type());
      } while (match(TokenType::Comma) && (current_++, true));
    }
    
    consume(TokenType::RParen);
    consume(TokenType::Arrow);
    std::string return_type = parse_type();
    
    // Create a FunctionType and return its string representation
    FunctionType func_type(std::move(param_types), std::move(return_type));
    return func_type.to_string();
  } else {
    throw std::runtime_error("Unexpected token in type: " + tokens_[current_].value);
  }
}

std::unique_ptr<AnonymousFunction> Parser::parse_anonymous_function() {
  consume(TokenType::Fn);
  consume(TokenType::LParen);

  // Parse parameters first, store them temporarily
  std::vector<std::unique_ptr<Parameter>> parameters;
  if (!match(TokenType::RParen)) {
    do {
      std::string param_name = consume(TokenType::Identifier).value;
      consume(TokenType::Colon);
      std::string param_type = parse_type();
      parameters.push_back(std::make_unique<Parameter>(param_name, param_type));
    } while (match(TokenType::Comma) && (consume(TokenType::Comma), true));
  }

  consume(TokenType::RParen);
  
  std::string return_type;
  if (match(TokenType::Arrow)) {
    consume(TokenType::Arrow);
    return_type = parse_type();
  } else {
    return_type = "nil";  // Default to nil if no return type specified
  }
  
  // Create anonymous function with return type
  auto func = std::make_unique<AnonymousFunction>(return_type);

  // Add all parameters
  for (auto& param : parameters) {
    func->add_parameter(std::move(param));
  }

  // Parse function body - check for 'do' or block syntax
  if (match(TokenType::Do)) {
    consume(TokenType::Do);
    // Single statement after 'do'
    func->add_statement(parse_statement());
  } else {
    consume(TokenType::LBrace);
    // Multiple statements in block
    while (!match(TokenType::RBrace)) {
      func->add_statement(parse_statement());
    }
    consume(TokenType::RBrace);
  }

  return func;
}

std::string Parser::infer_type(const ASTNode* node) {
  // Infer type from literals
  if (dynamic_cast<const NumberLiteral*>(node)) {
    return "i32";
  }
  
  if (dynamic_cast<const StringLiteral*>(node)) {
    return "const string";
  }
  
  if (dynamic_cast<const BooleanLiteral*>(node)) {
    return "bool";
  }
  
  // Infer type from anonymous functions
  if (const auto* anon_func = dynamic_cast<const AnonymousFunction*>(node)) {
    // Build function type string: fn(param_types) -> return_type
    std::string func_type = "fn(";
    for (size_t i = 0; i < anon_func->parameters().size(); ++i) {
      if (i > 0) func_type += ", ";
      // For now, assume all parameters are i32 (can be enhanced with parameter type info)
      func_type += "i32";
    }
    func_type += ") -> " + anon_func->return_type();
    return func_type;
  }
  
  // Infer type from variable references
  if (const auto* var_ref = dynamic_cast<const VariableReference*>(node)) {
    auto it = variable_types_.find(var_ref->name());
    if (it != variable_types_.end()) {
      return it->second;
    }
    throw std::runtime_error("Cannot infer type from undeclared variable '" + var_ref->name() + "'");
  }
  
  // Infer type from binary operations
  if (const auto* bin_op = dynamic_cast<const BinaryOperation*>(node)) {
    std::string left_type = infer_type(bin_op->left());
    std::string right_type = infer_type(bin_op->right());
    
    // Type rules for binary operations
    TokenType op = bin_op->operator_type();
    
    // Arithmetic operations (i32 + i32 = i32)
    if (op == TokenType::Plus || op == TokenType::Minus || 
        op == TokenType::Multiply || op == TokenType::Divide) {
      if (left_type == "i32" && right_type == "i32") {
        return "i32";
      }
      // String concatenation (const string + const string = const string)
      if (op == TokenType::Plus && left_type == "const string" && right_type == "const string") {
        return "const string";
      }
      throw std::runtime_error("Type mismatch in arithmetic operation: " + left_type + " " + 
                               (op == TokenType::Plus ? "+" : 
                                op == TokenType::Minus ? "-" : 
                                op == TokenType::Multiply ? "*" : "/") + " " + right_type);
    }
    
    // Comparison operations always return bool
    if (op == TokenType::EqualEqual || op == TokenType::NotEqual ||
        op == TokenType::LessThan || op == TokenType::LessEqual ||
        op == TokenType::GreaterThan || op == TokenType::GreaterEqual) {
      if (left_type == right_type) {
        return "bool";  // Boolean result
      }
      throw std::runtime_error("Cannot compare different types: " + left_type + " and " + right_type);
    }
    
    // Logical operations (bool && bool = bool)
    if (op == TokenType::And || op == TokenType::Or) {
      if (left_type == "bool" && right_type == "bool") {
        return "bool";
      }
      throw std::runtime_error("Logical operations require boolean operands");
    }
  }
  
  // Infer type from function calls
  if (const auto* func_call = dynamic_cast<const FunctionCall*>(node)) {
    auto it = function_return_types_.find(func_call->function_name());
    if (it != function_return_types_.end()) {
      return it->second;
    }
    
    // Check if it's a function pointer variable
    auto var_it = variable_types_.find(func_call->function_name());
    if (var_it != variable_types_.end()) {
      const std::string& func_type = var_it->second;
      // Extract return type from function pointer type: "fn(params) -> return_type"
      auto arrow_pos = func_type.find(" -> ");
      if (arrow_pos != std::string::npos) {
        return func_type.substr(arrow_pos + 4);  // Return the part after " -> "
      }
    }
    
    // Handle built-in functions or member access functions
    if (func_call->function_name() == "fmt.println") {
      return "nil";  // fmt.println returns nothing
    }
    throw std::runtime_error("Cannot infer return type from undeclared function '" + func_call->function_name() + "'");
  }
  
  // For other expression types, we can't infer the type yet
  throw std::runtime_error("Cannot infer type from this expression - use explicit type annotation");
}

}  // namespace void_compiler
