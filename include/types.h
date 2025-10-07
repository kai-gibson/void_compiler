#ifndef TYPES_H
#define TYPES_H

#include <memory>
#include <string>
#include <vector>

namespace void_compiler {
// Token types
enum class TokenType : uint8_t {
  Const,
  Identifier,
  Equals,
  Fn,
  LParen,
  RParen,
  LBrace,
  RBrace,
  Arrow,
  Return,
  Number,
  I32,
  Bool,          // bool type
  True,          // true literal
  False,         // false literal
  Comma,
  Colon,
  ColonEquals,  // := for type inference
  Plus,
  Minus,
  Multiply,
  Divide,
  Import,
  Dot,
  StringLiteral,
  If,
  Else,
  GreaterThan,
  LessThan,
  GreaterEqual,
  LessEqual,
  EqualEqual,
  NotEqual,
  And,
  Or,
  Not,
  Loop,
  In,
  DotDot,
  Do,
  Nil,
  String,
  EndOfFile
};

struct Token {
  TokenType type;
  std::string value;
  int line;
  int column;
};

// Function pointer type representation
class FunctionType {
 public:
  FunctionType(std::vector<std::string> param_types, std::string return_type)
      : param_types_(std::move(param_types)), return_type_(std::move(return_type)) {}

  [[nodiscard]] const std::vector<std::string>& param_types() const { return param_types_; }
  [[nodiscard]] const std::string& return_type() const { return return_type_; }
  
  // Generate a string representation for type checking
  [[nodiscard]] std::string to_string() const {
    std::string result = "fn(";
    for (size_t i = 0; i < param_types_.size(); ++i) {
      if (i > 0) result += ", ";
      result += param_types_[i];
    }
    result += ") -> " + return_type_;
    return result;
  }
  
  // Check if two function types are compatible
  [[nodiscard]] bool is_compatible(const FunctionType& other) const {
    return param_types_ == other.param_types_ && return_type_ == other.return_type_;
  }

 private:
  std::vector<std::string> param_types_;
  std::string return_type_;
};

// AST Node types
class ASTNode {
 public:
  virtual ~ASTNode() = default;
};

class StringLiteral : public ASTNode {
 public:
  explicit StringLiteral(std::string value) : value_(std::move(value)) {}
  [[nodiscard]] const std::string& value() const { return value_; }

 private:
  std::string value_;
};

class ImportStatement : public ASTNode {
 public:
  explicit ImportStatement(std::string module_name) : module_name_(std::move(module_name)) {}
  [[nodiscard]] const std::string& module_name() const { return module_name_; }

 private:
  std::string module_name_;
};

class MemberAccess : public ASTNode {
 public:
  MemberAccess(std::string object_name, std::string member_name,
               std::vector<std::unique_ptr<ASTNode>> arguments)
      : object_name_(std::move(object_name)), member_name_(std::move(member_name)), 
        arguments_(std::move(arguments)) {}

  [[nodiscard]] const std::string& object_name() const { return object_name_; }
  [[nodiscard]] const std::string& member_name() const { return member_name_; }
  [[nodiscard]] const std::vector<std::unique_ptr<ASTNode>>& arguments() const {
    return arguments_;
  }

 private:
  std::string object_name_;
  std::string member_name_;
  std::vector<std::unique_ptr<ASTNode>> arguments_;
};

class NumberLiteral : public ASTNode {
 public:
  explicit NumberLiteral(int value) : value_(value) {}
  [[nodiscard]] int value() const { return value_; }

 private:
  int value_;
};

class BooleanLiteral : public ASTNode {
 public:
  explicit BooleanLiteral(bool value) : value_(value) {}
  [[nodiscard]] bool value() const { return value_; }

 private:
  bool value_;
};

class VariableReference : public ASTNode {
 public:
  explicit VariableReference(std::string name) : name_(std::move(name)) {}
  [[nodiscard]] const std::string& name() const { return name_; }

 private:
  std::string name_;
};

class BinaryOperation : public ASTNode {
 public:
  BinaryOperation(std::unique_ptr<ASTNode> left, TokenType op,
                  std::unique_ptr<ASTNode> right)
      : left_(std::move(left)), operator_(op), right_(std::move(right)) {}

  [[nodiscard]] const ASTNode* left() const { return left_.get(); }
  [[nodiscard]] TokenType operator_type() const { return operator_; }
  [[nodiscard]] const ASTNode* right() const { return right_.get(); }

 private:
  std::unique_ptr<ASTNode> left_;
  TokenType operator_;
  std::unique_ptr<ASTNode> right_;
};

class UnaryOperation : public ASTNode {
 public:
  UnaryOperation(TokenType op, std::unique_ptr<ASTNode> operand)
      : operator_(op), operand_(std::move(operand)) {}

  [[nodiscard]] TokenType operator_type() const { return operator_; }
  [[nodiscard]] const ASTNode* operand() const { return operand_.get(); }

 private:
  TokenType operator_;
  std::unique_ptr<ASTNode> operand_;
};

class VariableDeclaration : public ASTNode {
 public:
  VariableDeclaration(std::string name, std::string type, std::unique_ptr<ASTNode> value)
      : name_(std::move(name)), type_(std::move(type)), value_(std::move(value)) {}

  [[nodiscard]] const std::string& name() const { return name_; }
  [[nodiscard]] const std::string& type() const { return type_; }
  [[nodiscard]] const ASTNode* value() const { return value_.get(); }

 private:
  std::string name_;
  std::string type_;
  std::unique_ptr<ASTNode> value_;
};

class VariableAssignment : public ASTNode {
 public:
  VariableAssignment(std::string name, std::unique_ptr<ASTNode> value)
      : name_(std::move(name)), value_(std::move(value)) {}

  [[nodiscard]] const std::string& name() const { return name_; }
  [[nodiscard]] const ASTNode* value() const { return value_.get(); }

 private:
  std::string name_;
  std::unique_ptr<ASTNode> value_;
};

class ReturnStatement : public ASTNode {
 public:
  explicit ReturnStatement(std::unique_ptr<ASTNode> expression)
      : expression_(std::move(expression)) {}

  [[nodiscard]] const ASTNode* expression() const { return expression_.get(); }

 private:
  std::unique_ptr<ASTNode> expression_;
};

class IfStatement : public ASTNode {
 public:
  IfStatement(std::unique_ptr<ASTNode> condition,
              std::vector<std::unique_ptr<ASTNode>> then_body,
              std::vector<std::unique_ptr<ASTNode>> else_body = {})
      : condition_(std::move(condition)), 
        then_body_(std::move(then_body)), 
        else_body_(std::move(else_body)) {}

  [[nodiscard]] const ASTNode* condition() const { return condition_.get(); }
  [[nodiscard]] const std::vector<std::unique_ptr<ASTNode>>& then_body() const {
    return then_body_;
  }
  [[nodiscard]] const std::vector<std::unique_ptr<ASTNode>>& else_body() const {
    return else_body_;
  }

 private:
  std::unique_ptr<ASTNode> condition_;
  std::vector<std::unique_ptr<ASTNode>> then_body_;
  std::vector<std::unique_ptr<ASTNode>> else_body_;
};

class RangeExpression : public ASTNode {
 public:
  RangeExpression(std::unique_ptr<ASTNode> start, std::unique_ptr<ASTNode> end)
      : start_(std::move(start)), end_(std::move(end)) {}

  [[nodiscard]] const ASTNode* start() const { return start_.get(); }
  [[nodiscard]] const ASTNode* end() const { return end_.get(); }

 private:
  std::unique_ptr<ASTNode> start_;
  std::unique_ptr<ASTNode> end_;
};

class LoopStatement : public ASTNode {
 public:
  // Range-based loop: loop i in 0..10 { ... }
  LoopStatement(std::string variable_name, std::unique_ptr<ASTNode> range,
                std::vector<std::unique_ptr<ASTNode>> body)
      : variable_name_(std::move(variable_name)), 
        range_(std::move(range)), 
        condition_(nullptr),
        body_(std::move(body)), 
        is_range_loop_(true) {}

  // Conditional loop: loop if condition { ... }
  LoopStatement(std::unique_ptr<ASTNode> condition,
                std::vector<std::unique_ptr<ASTNode>> body)
      : range_(nullptr),
        condition_(std::move(condition)), 
        body_(std::move(body)), 
        is_range_loop_(false) {}

  [[nodiscard]] bool is_range_loop() const { return is_range_loop_; }
  [[nodiscard]] const std::string& variable_name() const { return variable_name_; }
  [[nodiscard]] const ASTNode* range() const { return range_.get(); }
  [[nodiscard]] const ASTNode* condition() const { return condition_.get(); }
  [[nodiscard]] const std::vector<std::unique_ptr<ASTNode>>& body() const {
    return body_;
  }

 private:
  std::string variable_name_;  // For range loops
  std::unique_ptr<ASTNode> range_;  // For range loops  
  std::unique_ptr<ASTNode> condition_;  // For conditional loops
  std::vector<std::unique_ptr<ASTNode>> body_;
  bool is_range_loop_;
};

class FunctionCall : public ASTNode {
 public:
  FunctionCall(std::string name,
               std::vector<std::unique_ptr<ASTNode>> arguments)
      : function_name_(std::move(name)), arguments_(std::move(arguments)) {}

  [[nodiscard]] const std::string& function_name() const {
    return function_name_;
  }
  [[nodiscard]] const std::vector<std::unique_ptr<ASTNode>>& arguments() const {
    return arguments_;
  }

 private:
  std::string function_name_;
  std::vector<std::unique_ptr<ASTNode>> arguments_;
};

class Parameter : public ASTNode {
 public:
  Parameter(std::string name, std::string type)
      : name_(std::move(name)), type_(std::move(type)) {}

  [[nodiscard]] const std::string& name() const { return name_; }
  [[nodiscard]] const std::string& type() const { return type_; }

 private:
  std::string name_;
  std::string type_;
};

class FunctionDeclaration : public ASTNode {
 public:
  FunctionDeclaration(std::string name, std::string return_type)
      : name_(std::move(name)), return_type_(std::move(return_type)) {}

  [[nodiscard]] const std::string& name() const { return name_; }
  [[nodiscard]] const std::string& return_type() const { return return_type_; }
  [[nodiscard]] const std::vector<std::unique_ptr<ASTNode>>& body() const {
    return body_;
  }
  [[nodiscard]] const std::vector<std::unique_ptr<Parameter>>& parameters()
      const {
    return parameters_;
  }

  void add_statement(std::unique_ptr<ASTNode> statement) {
    body_.push_back(std::move(statement));
  }

  void add_parameter(std::unique_ptr<Parameter> parameter) {
    parameters_.push_back(std::move(parameter));
  }

 private:
  std::string name_;
  std::string return_type_;
  std::vector<std::unique_ptr<Parameter>> parameters_;
  std::vector<std::unique_ptr<ASTNode>> body_;
};

class AnonymousFunction : public ASTNode {
 public:
  explicit AnonymousFunction(std::string return_type)
      : return_type_(std::move(return_type)) {}

  [[nodiscard]] const std::string& return_type() const { return return_type_; }
  [[nodiscard]] const std::vector<std::unique_ptr<ASTNode>>& body() const {
    return body_;
  }
  [[nodiscard]] const std::vector<std::unique_ptr<Parameter>>& parameters()
      const {
    return parameters_;
  }

  void add_statement(std::unique_ptr<ASTNode> statement) {
    body_.push_back(std::move(statement));
  }

  void add_parameter(std::unique_ptr<Parameter> parameter) {
    parameters_.push_back(std::move(parameter));
  }

 private:
  std::string return_type_;
  std::vector<std::unique_ptr<Parameter>> parameters_;
  std::vector<std::unique_ptr<ASTNode>> body_;
};

class Program : public ASTNode {
 public:
  Program() = default;

  [[nodiscard]] const std::vector<std::unique_ptr<ImportStatement>>&
  imports() const {
    return imports_;
  }

  [[nodiscard]] const std::vector<std::unique_ptr<FunctionDeclaration>>&
  functions() const {
    return functions_;
  }

  [[nodiscard]] const std::vector<std::unique_ptr<VariableDeclaration>>&
  variables() const {
    return variables_;
  }

  void add_import(std::unique_ptr<ImportStatement> import) {
    imports_.push_back(std::move(import));
  }

  void add_function(std::unique_ptr<FunctionDeclaration> function) {
    functions_.push_back(std::move(function));
  }

  void add_variable(std::unique_ptr<VariableDeclaration> variable) {
    variables_.push_back(std::move(variable));
  }

 private:
  std::vector<std::unique_ptr<ImportStatement>> imports_;
  std::vector<std::unique_ptr<FunctionDeclaration>> functions_;
  std::vector<std::unique_ptr<VariableDeclaration>> variables_;
};
}  // namespace void_compiler

#endif  // TYPES_H
