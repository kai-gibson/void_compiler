#ifndef PARSER_H
#define PARSER_H
#include <unordered_map>
#include <vector>

#include "types.h"

namespace void_compiler {
// Parser
class Parser {
 public:
  explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

  std::unique_ptr<Program> parse();

 private:
  Token& peek();
  Token consume(TokenType expected);
  bool match(TokenType type) const;
  std::unique_ptr<ASTNode> parse_expression();
  std::unique_ptr<ASTNode> parse_logical_or();
  std::unique_ptr<ASTNode> parse_logical_and();
  std::unique_ptr<ASTNode> parse_comparison();
  std::unique_ptr<ASTNode> parse_additive();
  std::unique_ptr<ASTNode> parse_multiplicative();
  std::unique_ptr<ASTNode> parse_unary();
  std::unique_ptr<ASTNode> parse_primary();
  std::unique_ptr<ASTNode> parse_statement();
  std::unique_ptr<IfStatement> parse_if_statement();
  std::unique_ptr<LoopStatement> parse_loop_statement();
  std::unique_ptr<RangeExpression> parse_range_expression();
  std::unique_ptr<ImportStatement> parse_import();
  std::unique_ptr<FunctionDeclaration> parse_function();
  std::unique_ptr<AnonymousFunction> parse_anonymous_function();
  std::unique_ptr<VariableDeclaration> parse_variable_declaration();
  std::unique_ptr<VariableAssignment> parse_variable_assignment();
  std::string parse_type();  // Helper to parse type tokens
  std::string infer_type(
      const ASTNode* node);  // Helper to infer types from expressions
  std::unique_ptr<ASTNode> parse_slice_expression();

  std::vector<Token> tokens_;
  size_t current_ = 0;

  // Symbol table for type tracking
  std::unordered_map<std::string, std::string> variable_types_;
  std::unordered_map<std::string, std::string> function_return_types_;
};

}  // namespace void_compiler
#endif  // PARSER_H
