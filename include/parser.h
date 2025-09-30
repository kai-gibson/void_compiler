#ifndef PARSER_H
#define PARSER_H
#include <vector>

#include "types.h"

namespace void_compiler {
// Parser
class Parser {
 public:
  explicit Parser(std::vector<Token> tokens)
      : tokens_(std::move(tokens)), current_(0) {}

  std::unique_ptr<Program> parse();

 private:
  Token& peek();
  Token consume(TokenType expected);
  bool match(TokenType type) const;
  std::unique_ptr<ASTNode> parse_expression();
  std::unique_ptr<ASTNode> parse_additive();
  std::unique_ptr<ASTNode> parse_multiplicative();
  std::unique_ptr<ASTNode> parse_primary();
  std::unique_ptr<ASTNode> parse_statement();
  std::unique_ptr<FunctionDeclaration> parse_function();

  std::vector<Token> tokens_;
  size_t current_;
};

}  // namespace void_compiler
#endif  // PARSER_H
