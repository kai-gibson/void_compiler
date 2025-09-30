#include <gtest/gtest.h>

#include "lexer.h"
#include "parser.h"

namespace void_compiler {
namespace {

class ParserTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}

  std::unique_ptr<Program> ParseSource(const std::string& source) {
    Lexer lexer(source);
    std::vector<Token> tokens;
    Token token;
    do {
      token = lexer.next_token();
      tokens.push_back(token);
    } while (token.type != TokenType::EndOfFile);

    Parser parser(std::move(tokens));
    return parser.parse();
  }
};

TEST_F(ParserTest, ParsesSimpleFunction) {
  const std::string source = R"(
const test = fn() -> i32 {
  return 42
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);

  const auto& func = program->functions()[0];
  EXPECT_EQ(func->name(), "test");
  EXPECT_EQ(func->return_type(), "i32");
  EXPECT_EQ(func->parameters().size(), 0);
  EXPECT_EQ(func->body().size(), 1);

  // Check the return statement
  const auto* ret_stmt = dynamic_cast<const ReturnStatement*>(func->body()[0].get());
  ASSERT_NE(ret_stmt, nullptr);
  
  const auto* num_literal = dynamic_cast<const NumberLiteral*>(ret_stmt->expression());
  ASSERT_NE(num_literal, nullptr);
  EXPECT_EQ(num_literal->value(), 42);
}

TEST_F(ParserTest, ParsesFunctionWithParameters) {
  const std::string source = R"(
const add = fn(x: i32, y: i32) -> i32 {
  return x + y
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);

  const auto& func = program->functions()[0];
  EXPECT_EQ(func->name(), "add");
  EXPECT_EQ(func->return_type(), "i32");
  ASSERT_EQ(func->parameters().size(), 2);

  EXPECT_EQ(func->parameters()[0]->name(), "x");
  EXPECT_EQ(func->parameters()[0]->type(), "i32");
  EXPECT_EQ(func->parameters()[1]->name(), "y");
  EXPECT_EQ(func->parameters()[1]->type(), "i32");
}

TEST_F(ParserTest, ParsesMultipleFunctions) {
  const std::string source = R"(
const first = fn() -> i32 {
  return 1
}

const second = fn(a: i32) -> i32 {
  return a
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 2);

  EXPECT_EQ(program->functions()[0]->name(), "first");
  EXPECT_EQ(program->functions()[1]->name(), "second");
  EXPECT_EQ(program->functions()[1]->parameters().size(), 1);
}

TEST_F(ParserTest, ParsesArithmeticExpressions) {
  const std::string source = R"(
const calc = fn(x: i32, y: i32) -> i32 {
  return x + y * 2 - x / y
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);

  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 1);

  const auto* ret_stmt = dynamic_cast<const ReturnStatement*>(func->body()[0].get());
  ASSERT_NE(ret_stmt, nullptr);

  // The expression should be a binary operation (subtraction at the top level)
  const auto* binop = dynamic_cast<const BinaryOperation*>(ret_stmt->expression());
  ASSERT_NE(binop, nullptr);
  EXPECT_EQ(binop->operator_type(), TokenType::Minus);
}

TEST_F(ParserTest, ParsesFunctionCalls) {
  const std::string source = R"(
const helper = fn() -> i32 {
  return 42
}

const main = fn() -> i32 {
  return helper()
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 2);

  const auto& main_func = program->functions()[1];
  ASSERT_EQ(main_func->body().size(), 1);

  const auto* ret_stmt = dynamic_cast<const ReturnStatement*>(main_func->body()[0].get());
  ASSERT_NE(ret_stmt, nullptr);

  const auto* func_call = dynamic_cast<const FunctionCall*>(ret_stmt->expression());
  ASSERT_NE(func_call, nullptr);
  EXPECT_EQ(func_call->function_name(), "helper");
  EXPECT_EQ(func_call->arguments().size(), 0);
}

TEST_F(ParserTest, ParsesFunctionCallsWithArguments) {
  const std::string source = R"(
const add = fn(x: i32, y: i32) -> i32 {
  return x + y
}

const main = fn() -> i32 {
  return add(5, 10)
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 2);

  const auto& main_func = program->functions()[1];
  const auto* ret_stmt = dynamic_cast<const ReturnStatement*>(main_func->body()[0].get());
  ASSERT_NE(ret_stmt, nullptr);

  const auto* func_call = dynamic_cast<const FunctionCall*>(ret_stmt->expression());
  ASSERT_NE(func_call, nullptr);
  EXPECT_EQ(func_call->function_name(), "add");
  ASSERT_EQ(func_call->arguments().size(), 2);

  // Check arguments
  const auto* arg1 = dynamic_cast<const NumberLiteral*>(func_call->arguments()[0].get());
  const auto* arg2 = dynamic_cast<const NumberLiteral*>(func_call->arguments()[1].get());
  ASSERT_NE(arg1, nullptr);
  ASSERT_NE(arg2, nullptr);
  EXPECT_EQ(arg1->value(), 5);
  EXPECT_EQ(arg2->value(), 10);
}

TEST_F(ParserTest, ParsesVariableReferences) {
  const std::string source = R"(
const test = fn(x: i32) -> i32 {
  return x
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);

  const auto& func = program->functions()[0];
  const auto* ret_stmt = dynamic_cast<const ReturnStatement*>(func->body()[0].get());
  ASSERT_NE(ret_stmt, nullptr);

  const auto* var_ref = dynamic_cast<const VariableReference*>(ret_stmt->expression());
  ASSERT_NE(var_ref, nullptr);
  EXPECT_EQ(var_ref->name(), "x");
}

TEST_F(ParserTest, ParsesParenthesizedExpressions) {
  const std::string source = R"(
const calc = fn(x: i32, y: i32) -> i32 {
  return (x + y) * 2
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);

  const auto& func = program->functions()[0];
  const auto* ret_stmt = dynamic_cast<const ReturnStatement*>(func->body()[0].get());
  ASSERT_NE(ret_stmt, nullptr);

  // Should be multiplication at the top level due to parentheses
  const auto* binop = dynamic_cast<const BinaryOperation*>(ret_stmt->expression());
  ASSERT_NE(binop, nullptr);
  EXPECT_EQ(binop->operator_type(), TokenType::Multiply);
}

}  // namespace
}  // namespace void_compiler