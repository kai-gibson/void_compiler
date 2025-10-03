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
  Comma,
  Colon,
  Plus,
  Minus,
  Multiply,
  Divide,
  Import,
  Dot,
  StringLiteral,
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
