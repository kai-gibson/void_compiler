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

class FunctionDeclaration : public ASTNode {
 public:
  FunctionDeclaration(const std::string& name, const std::string& return_type)
      : name_(name), return_type_(return_type) {}

  const std::string& name() const { return name_; }
  const std::string& return_type() const { return return_type_; }
  const std::vector<std::unique_ptr<ASTNode>>& body() const { return body_; }

  void add_statement(std::unique_ptr<ASTNode> statement) {
    body_.push_back(std::move(statement));
  }

 private:
  std::string name_;
  std::string return_type_;
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
