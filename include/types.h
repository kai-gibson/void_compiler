#ifndef TYPES_H
#define TYPES_H

#include <memory>
#include <string>
#include <vector>

namespace void_compiler {
// Token types
enum class TokenType {
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
  Comma,
  Colon,
  Plus,
  Minus,
  Multiply,
  Divide,
  EndOfFile
};

struct Token {
  TokenType type;
  std::string value;
  int line;
  int column;
};

// AST Node types
class ASTNode {
 public:
  virtual ~ASTNode() = default;
};

class NumberLiteral : public ASTNode {
 public:
  explicit NumberLiteral(int value) : value_(value) {}
  int value() const { return value_; }

 private:
  int value_;
};

class VariableReference : public ASTNode {
 public:
  explicit VariableReference(const std::string& name) : name_(name) {}
  const std::string& name() const { return name_; }

 private:
  std::string name_;
};

class BinaryOperation : public ASTNode {
 public:
  BinaryOperation(std::unique_ptr<ASTNode> left, TokenType op,
                  std::unique_ptr<ASTNode> right)
      : left_(std::move(left)), operator_(op), right_(std::move(right)) {}

  const ASTNode* left() const { return left_.get(); }
  TokenType operator_type() const { return operator_; }
  const ASTNode* right() const { return right_.get(); }

 private:
  std::unique_ptr<ASTNode> left_;
  TokenType operator_;
  std::unique_ptr<ASTNode> right_;
};

class ReturnStatement : public ASTNode {
 public:
  explicit ReturnStatement(std::unique_ptr<ASTNode> expression)
      : expression_(std::move(expression)) {}
  const ASTNode* expression() const { return expression_.get(); }

 private:
  std::unique_ptr<ASTNode> expression_;
};

class FunctionCall : public ASTNode {
 public:
  FunctionCall(const std::string& name,
               std::vector<std::unique_ptr<ASTNode>> arguments)
      : function_name_(name), arguments_(std::move(arguments)) {}

  const std::string& function_name() const { return function_name_; }
  const std::vector<std::unique_ptr<ASTNode>>& arguments() const {
    return arguments_;
  }

 private:
  std::string function_name_;
  std::vector<std::unique_ptr<ASTNode>> arguments_;
};

class Parameter : public ASTNode {
 public:
  Parameter(const std::string& name, const std::string& type)
      : name_(name), type_(type) {}

  const std::string& name() const { return name_; }
  const std::string& type() const { return type_; }

 private:
  std::string name_;
  std::string type_;
};

class FunctionDeclaration : public ASTNode {
 public:
  FunctionDeclaration(const std::string& name, const std::string& return_type)
      : name_(name), return_type_(return_type) {}

  const std::string& name() const { return name_; }
  const std::string& return_type() const { return return_type_; }
  const std::vector<std::unique_ptr<ASTNode>>& body() const { return body_; }
  const std::vector<std::unique_ptr<Parameter>>& parameters() const {
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

class Program : public ASTNode {
 public:
  Program() = default;

  const std::vector<std::unique_ptr<FunctionDeclaration>>& functions() const {
    return functions_;
  }

  void add_function(std::unique_ptr<FunctionDeclaration> function) {
    functions_.push_back(std::move(function));
  }

 private:
  std::vector<std::unique_ptr<FunctionDeclaration>> functions_;
};
}  // namespace void_compiler

#endif  // TYPES_H
