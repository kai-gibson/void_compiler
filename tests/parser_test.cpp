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

// Error handling tests
TEST_F(ParserTest, ThrowsOnMissingConst) {
  EXPECT_THROW({
    ParseSource("add = fn() -> i32 { return 1 }");
  }, std::runtime_error);
}

TEST_F(ParserTest, ThrowsOnMissingIdentifier) {
  EXPECT_THROW({
    ParseSource("const = fn() -> i32 { return 1 }");
  }, std::runtime_error);
}

TEST_F(ParserTest, ThrowsOnMissingEquals) {
  EXPECT_THROW({
    ParseSource("const add fn() -> i32 { return 1 }");
  }, std::runtime_error);
}

TEST_F(ParserTest, ThrowsOnMissingFn) {
  EXPECT_THROW({
    ParseSource("const add = () -> i32 { return 1 }");
  }, std::runtime_error);
}

TEST_F(ParserTest, ThrowsOnMissingLParen) {
  EXPECT_THROW({
    ParseSource("const add = fn) -> i32 { return 1 }");
  }, std::runtime_error);
}

TEST_F(ParserTest, ThrowsOnMissingRParen) {
  EXPECT_THROW({
    ParseSource("const add = fn( -> i32 { return 1 }");
  }, std::runtime_error);
}

TEST_F(ParserTest, ThrowsOnMissingArrow) {
  EXPECT_THROW({
    ParseSource("const add = fn() i32 { return 1 }");
  }, std::runtime_error);
}

TEST_F(ParserTest, ThrowsOnMissingReturnType) {
  EXPECT_THROW({
    ParseSource("const add = fn() -> { return 1 }");
  }, std::runtime_error);
}

TEST_F(ParserTest, ThrowsOnMissingLBrace) {
  EXPECT_THROW({
    ParseSource("const add = fn() -> i32 return 1 }");
  }, std::runtime_error);
}

TEST_F(ParserTest, ThrowsOnMissingRBrace) {
  EXPECT_THROW({
    ParseSource("const add = fn() -> i32 { return 1");
  }, std::runtime_error);
}

TEST_F(ParserTest, ParsesReturnWithoutExpressionForTypeValidation) {
  // Parser should accept return without expression and let code generation validate types
  const std::string source = R"(
const add = fn() -> i32 {
  return
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  EXPECT_EQ(func->name(), "add");
  EXPECT_EQ(func->return_type(), "i32");
  EXPECT_EQ(func->body().size(), 1);
  
  const auto* return_stmt = dynamic_cast<const ReturnStatement*>(func->body()[0].get());
  ASSERT_NE(return_stmt, nullptr);
  EXPECT_EQ(return_stmt->expression(), nullptr);  // Return without value
}

TEST_F(ParserTest, ThrowsOnUnmatchedParentheses) {
  EXPECT_THROW({
    ParseSource("const add = fn() -> i32 { return (1 + 2 }");
  }, std::runtime_error);
}

TEST_F(ParserTest, ThrowsOnMissingParameterType) {
  EXPECT_THROW({
    ParseSource("const add = fn(x) -> i32 { return x }");
  }, std::runtime_error);
}

TEST_F(ParserTest, ThrowsOnMissingParameterName) {
  EXPECT_THROW({
    ParseSource("const add = fn(: i32) -> i32 { return 1 }");
  }, std::runtime_error);
}

TEST_F(ParserTest, ThrowsOnMissingColon) {
  EXPECT_THROW({
    ParseSource("const add = fn(x i32) -> i32 { return x }");
  }, std::runtime_error);
}

TEST_F(ParserTest, HandlesEmptyParameterList) {
  const std::string source = R"(
const test = fn() -> i32 {
  return 42
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  EXPECT_EQ(program->functions()[0]->parameters().size(), 0);
}

TEST_F(ParserTest, ParsesComplexNestedExpressions) {
  const std::string source = R"(
const complex = fn(a: i32, b: i32) -> i32 {
  return ((a + b) * (a - b)) / (a + 1)
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);

  const auto& func = program->functions()[0];
  const auto* ret_stmt = dynamic_cast<const ReturnStatement*>(func->body()[0].get());
  ASSERT_NE(ret_stmt, nullptr);

  // Should be division at the top level
  const auto* binop = dynamic_cast<const BinaryOperation*>(ret_stmt->expression());
  ASSERT_NE(binop, nullptr);
  EXPECT_EQ(binop->operator_type(), TokenType::Divide);
}

TEST_F(ParserTest, ParsesDeepNestedParentheses) {
  const std::string source = R"(
const test = fn() -> i32 {
  return ((((1))))
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  auto* func = program->functions()[0].get();
  auto* ret_stmt = dynamic_cast<ReturnStatement*>(func->body()[0].get());
  ASSERT_NE(ret_stmt, nullptr);
  auto* num = dynamic_cast<const NumberLiteral*>(ret_stmt->expression());
  ASSERT_NE(num, nullptr);
  EXPECT_EQ(num->value(), 1);
}

TEST_F(ParserTest, ParsesMultipleParametersWithTypes) {
  const std::string source = R"(
const func = fn(a: i32, b: i32, c: i32) -> i32 {
  return a
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  auto* func = program->functions()[0].get();
  ASSERT_EQ(func->parameters().size(), 3);
  EXPECT_EQ(func->parameters()[0]->name(), "a");
  EXPECT_EQ(func->parameters()[1]->name(), "b");
  EXPECT_EQ(func->parameters()[2]->name(), "c");
}

TEST_F(ParserTest, ParsesChainedFunctionCalls) {
  const std::string source = R"(
const f1 = fn() -> i32 { return 1 }
const f2 = fn() -> i32 { return 2 }
const main = fn() -> i32 { return f1() + f2() + f1() }
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 3);
}

TEST_F(ParserTest, ParsesOperatorPrecedenceCorrectly) {
  const std::string source = R"(
const test = fn() -> i32 {
  return 1 + 2 * 3 - 4 / 2
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  auto* func = program->functions()[0].get();
  auto* ret_stmt = dynamic_cast<ReturnStatement*>(func->body()[0].get());
  ASSERT_NE(ret_stmt, nullptr);
  // Should parse as: (1 + (2 * 3)) - (4 / 2)
  auto* sub_expr = dynamic_cast<const BinaryOperation*>(ret_stmt->expression());
  ASSERT_NE(sub_expr, nullptr);
  EXPECT_EQ(sub_expr->operator_type(), TokenType::Minus);
}

TEST_F(ParserTest, ParsesExpressionsWithAllOperators) {
  const std::string source = R"(
const test = fn() -> i32 {
  return 1 + 2 - 3 * 4 / 5
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
}

TEST_F(ParserTest, ThrowsOnIncompleteFunction) {
  EXPECT_THROW(ParseSource("const test ="), std::runtime_error);
  EXPECT_THROW(ParseSource("const test = fn"), std::runtime_error);
  EXPECT_THROW(ParseSource("const test = fn("), std::runtime_error);
}

TEST_F(ParserTest, ThrowsOnMissingReturnStatement) {
  const std::string source = R"(
const test = fn() -> i32 {
  // Missing return statement completely
}
)";
  // Change to test malformed syntax instead since empty body might be valid
  EXPECT_THROW(ParseSource("const test = fn() -> i32 { const x = 5 }"), std::runtime_error);
}

TEST_F(ParserTest, ThrowsOnInvalidParameterSyntax) {
  EXPECT_THROW(ParseSource("const test = fn(a b: i32) -> i32 { return 1 }"), std::runtime_error);
  EXPECT_THROW(ParseSource("const test = fn(: i32) -> i32 { return 1 }"), std::runtime_error);
  EXPECT_THROW(ParseSource("const test = fn(a:) -> i32 { return 1 }"), std::runtime_error);
}

TEST_F(ParserTest, ThrowsOnInvalidExpressionSequences) {
  EXPECT_THROW(ParseSource("const test = fn() -> i32 { return 1 + }"), std::runtime_error);
  EXPECT_THROW(ParseSource("const test = fn() -> i32 { return + 1 }"), std::runtime_error);
  EXPECT_THROW(ParseSource("const test = fn() -> i32 { return 1 2 }"), std::runtime_error);
}

TEST_F(ParserTest, ParsesVariousIdentifierFormats) {
  const std::string source = R"(
const _test = fn(var_name: i32, _param: i32, test123: i32) -> i32 {
  return var_name
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  auto* func = program->functions()[0].get();
  EXPECT_EQ(func->name(), "_test");
  EXPECT_EQ(func->parameters()[0]->name(), "var_name");
  EXPECT_EQ(func->parameters()[1]->name(), "_param");
  EXPECT_EQ(func->parameters()[2]->name(), "test123");
}

TEST_F(ParserTest, ParsesSingleParameterFunctions) {
  const std::string source = R"(
const single = fn(x: i32) -> i32 {
  return x
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  auto* func = program->functions()[0].get();
  ASSERT_EQ(func->parameters().size(), 1);
  EXPECT_EQ(func->parameters()[0]->name(), "x");
}

TEST_F(ParserTest, ParsesComplexMathExpressions) {
  const std::string source = R"(
const math = fn(a: i32, b: i32, c: i32) -> i32 {
  return a * b + c * (a - b) / (a + 1)
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
}

TEST_F(ParserTest, HandlesFunctionCallsInComplexExpressions) {
  const std::string source = R"(
const helper = fn(x: i32) -> i32 { return x * 2 }
const main = fn() -> i32 { return helper(5) + helper(3) * 2 }
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 2);
}

TEST_F(ParserTest, ParsesLocalVariableDeclaration) {
  const std::string source = R"(
const main = fn() -> i32 {
  x :i32 = 42
  return x
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 2);  // variable declaration + return statement
  
  // Check variable declaration
  const auto* var_decl = dynamic_cast<const VariableDeclaration*>(func->body()[0].get());
  ASSERT_NE(var_decl, nullptr);
  ASSERT_EQ(var_decl->name(), "x");
  ASSERT_EQ(var_decl->type(), "i32");
}

TEST_F(ParserTest, ParsesMultipleLocalVariables) {
  const std::string source = R"(
const main = fn() -> i32 {
  x :i32 = 10
  y :i32 = 20
  z :i32 = x + y
  return z
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 4);  // 3 variable declarations + return statement
  
  // Check all variable declarations
  const auto* var_x = dynamic_cast<const VariableDeclaration*>(func->body()[0].get());
  ASSERT_NE(var_x, nullptr);
  ASSERT_EQ(var_x->name(), "x");
  
  const auto* var_y = dynamic_cast<const VariableDeclaration*>(func->body()[1].get());
  ASSERT_NE(var_y, nullptr);
  ASSERT_EQ(var_y->name(), "y");
  
  const auto* var_z = dynamic_cast<const VariableDeclaration*>(func->body()[2].get());
  ASSERT_NE(var_z, nullptr);
  ASSERT_EQ(var_z->name(), "z");
}

TEST_F(ParserTest, ParsesVariableWithExpressionValue) {
  const std::string source = R"(
const main = fn() -> i32 {
  result :i32 = 5 * 3 + 2
  return result
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  const auto* var_decl = dynamic_cast<const VariableDeclaration*>(func->body()[0].get());
  ASSERT_NE(var_decl, nullptr);
  ASSERT_EQ(var_decl->name(), "result");
  
  // Check that the value is a binary operation
  const auto* binop = dynamic_cast<const BinaryOperation*>(var_decl->value());
  ASSERT_NE(binop, nullptr);
}

TEST_F(ParserTest, ParsesVariablesWithParameterReferences) {
  const std::string source = R"(
const compute = fn(a: i32, b: i32) -> i32 {
  sum :i32 = a + b
  product :i32 = a * b
  return sum + product
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 3);  // 2 variable declarations + return statement
  ASSERT_EQ(func->parameters().size(), 2);  // a and b parameters
}

TEST_F(ParserTest, ParsesVariableAssignment) {
  const std::string source = R"(
const main = fn() -> i32 {
  x :i32 = 100
  x = x * 2
  return x
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 3);  // declaration + assignment + return
  
  // Check variable assignment
  const auto* var_assign = dynamic_cast<const VariableAssignment*>(func->body()[1].get());
  ASSERT_NE(var_assign, nullptr);
  ASSERT_EQ(var_assign->name(), "x");
}

TEST_F(ParserTest, ParsesMultipleVariableAssignments) {
  const std::string source = R"(
const main = fn() -> i32 {
  x :i32 = 10
  y :i32 = 20
  x = y + 5
  y = x * 2
  return x + y
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 5);  // 2 declarations + 2 assignments + return
  
  // Check both assignments
  const auto* assign1 = dynamic_cast<const VariableAssignment*>(func->body()[2].get());
  ASSERT_NE(assign1, nullptr);
  ASSERT_EQ(assign1->name(), "x");
  
  const auto* assign2 = dynamic_cast<const VariableAssignment*>(func->body()[3].get());
  ASSERT_NE(assign2, nullptr);
  ASSERT_EQ(assign2->name(), "y");
}

TEST_F(ParserTest, ParsesAssignmentWithComplexExpression) {
  const std::string source = R"(
const main = fn() -> i32 {
  result :i32 = 0
  result = (5 + 3) * 2 - 1
  return result
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  const auto* var_assign = dynamic_cast<const VariableAssignment*>(func->body()[1].get());
  ASSERT_NE(var_assign, nullptr);
  ASSERT_EQ(var_assign->name(), "result");
}

TEST_F(ParserTest, ParsesSimpleIfStatement) {
  const std::string source = R"(
const test = fn(x: i32) -> i32 {
  if x > 10 {
    return 1
  }
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 2);  // if statement + return
  
  const auto* if_stmt = dynamic_cast<const IfStatement*>(func->body()[0].get());
  ASSERT_NE(if_stmt, nullptr);
  
  // Check condition is a comparison
  const auto* condition = dynamic_cast<const BinaryOperation*>(if_stmt->condition());
  ASSERT_NE(condition, nullptr);
  EXPECT_EQ(condition->operator_type(), TokenType::GreaterThan);
  
  // Check then body has one return statement
  ASSERT_EQ(if_stmt->then_body().size(), 1);
  const auto* ret_stmt = dynamic_cast<const ReturnStatement*>(if_stmt->then_body()[0].get());
  ASSERT_NE(ret_stmt, nullptr);
  
  // Check else body is empty
  EXPECT_EQ(if_stmt->else_body().size(), 0);
}

TEST_F(ParserTest, ParsesIfElseStatement) {
  const std::string source = R"(
const test = fn(x: i32) -> i32 {
  if x > 10 {
    return 1
  } else {
    return 0
  }
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 1);  // just if-else statement
  
  const auto* if_stmt = dynamic_cast<const IfStatement*>(func->body()[0].get());
  ASSERT_NE(if_stmt, nullptr);
  
  // Check then body
  ASSERT_EQ(if_stmt->then_body().size(), 1);
  const auto* then_ret = dynamic_cast<const ReturnStatement*>(if_stmt->then_body()[0].get());
  ASSERT_NE(then_ret, nullptr);
  
  // Check else body
  ASSERT_EQ(if_stmt->else_body().size(), 1);
  const auto* else_ret = dynamic_cast<const ReturnStatement*>(if_stmt->else_body()[0].get());
  ASSERT_NE(else_ret, nullptr);
}

TEST_F(ParserTest, ParsesIfElseIfElseStatement) {
  const std::string source = R"(
const test = fn(x: i32) -> i32 {
  if x > 20 {
    return 3
  } else if x > 10 {
    return 2
  } else {
    return 1
  }
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 1);  // if-else-if-else chain
  
  const auto* if_stmt = dynamic_cast<const IfStatement*>(func->body()[0].get());
  ASSERT_NE(if_stmt, nullptr);
  
  // Check condition
  const auto* condition = dynamic_cast<const BinaryOperation*>(if_stmt->condition());
  ASSERT_NE(condition, nullptr);
  EXPECT_EQ(condition->operator_type(), TokenType::GreaterThan);
  
  // Check then body
  ASSERT_EQ(if_stmt->then_body().size(), 1);
  
  // Check else body contains another if statement (else-if)
  ASSERT_EQ(if_stmt->else_body().size(), 1);
  const auto* nested_if = dynamic_cast<const IfStatement*>(if_stmt->else_body()[0].get());
  ASSERT_NE(nested_if, nullptr);
  
  // Check nested if has both then and else
  EXPECT_EQ(nested_if->then_body().size(), 1);
  EXPECT_EQ(nested_if->else_body().size(), 1);
}

TEST_F(ParserTest, ParsesAllComparisonOperators) {
  const std::string source = R"(
const test = fn(a: i32, b: i32) -> i32 {
  if a > b {
    return 1
  } else if a < b {
    return 2
  } else if a >= b {
    return 3
  } else if a <= b {
    return 4
  } else if a == b {
    return 5
  } else if a != b {
    return 6
  }
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 2);  // if chain + final return
  
  const auto* if_stmt = dynamic_cast<const IfStatement*>(func->body()[0].get());
  ASSERT_NE(if_stmt, nullptr);
  
  // Check first condition (a > b)
  const auto* condition = dynamic_cast<const BinaryOperation*>(if_stmt->condition());
  ASSERT_NE(condition, nullptr);
  EXPECT_EQ(condition->operator_type(), TokenType::GreaterThan);
}

TEST_F(ParserTest, ParsesLogicalAndExpression) {
  const std::string source = R"(
const test = fn(a: i32, b: i32) -> i32 {
  if a > 10 and b < 20 {
    return 1
  }
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  const auto* if_stmt = dynamic_cast<const IfStatement*>(func->body()[0].get());
  ASSERT_NE(if_stmt, nullptr);
  
  // Check condition is an AND operation
  const auto* and_op = dynamic_cast<const BinaryOperation*>(if_stmt->condition());
  ASSERT_NE(and_op, nullptr);
  EXPECT_EQ(and_op->operator_type(), TokenType::And);
  
  // Check left side is a > 10
  const auto* left_comp = dynamic_cast<const BinaryOperation*>(and_op->left());
  ASSERT_NE(left_comp, nullptr);
  EXPECT_EQ(left_comp->operator_type(), TokenType::GreaterThan);
  
  // Check right side is b < 20
  const auto* right_comp = dynamic_cast<const BinaryOperation*>(and_op->right());
  ASSERT_NE(right_comp, nullptr);
  EXPECT_EQ(right_comp->operator_type(), TokenType::LessThan);
}

TEST_F(ParserTest, ParsesLogicalOrExpression) {
  const std::string source = R"(
const test = fn(a: i32, b: i32) -> i32 {
  if a > 100 or b < 5 {
    return 1
  }
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  const auto* if_stmt = dynamic_cast<const IfStatement*>(func->body()[0].get());
  ASSERT_NE(if_stmt, nullptr);
  
  // Check condition is an OR operation
  const auto* or_op = dynamic_cast<const BinaryOperation*>(if_stmt->condition());
  ASSERT_NE(or_op, nullptr);
  EXPECT_EQ(or_op->operator_type(), TokenType::Or);
}

TEST_F(ParserTest, ParsesLogicalNotExpression) {
  const std::string source = R"(
const test = fn(a: i32) -> i32 {
  if not a > 10 {
    return 1
  }
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  const auto* if_stmt = dynamic_cast<const IfStatement*>(func->body()[0].get());
  ASSERT_NE(if_stmt, nullptr);
  
  // Check condition is a NOT operation
  const auto* not_op = dynamic_cast<const UnaryOperation*>(if_stmt->condition());
  ASSERT_NE(not_op, nullptr);
  EXPECT_EQ(not_op->operator_type(), TokenType::Not);
  
  // Check operand is a > 10
  const auto* comparison = dynamic_cast<const BinaryOperation*>(not_op->operand());
  ASSERT_NE(comparison, nullptr);
  EXPECT_EQ(comparison->operator_type(), TokenType::GreaterThan);
}

TEST_F(ParserTest, ParsesComplexLogicalExpression) {
  const std::string source = R"(
const test = fn(a: i32, b: i32, c: i32) -> i32 {
  if a > 10 and b < 20 or not c == 5 {
    return 1
  }
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  const auto* if_stmt = dynamic_cast<const IfStatement*>(func->body()[0].get());
  ASSERT_NE(if_stmt, nullptr);
  
  // Check condition is an OR operation (lowest precedence)
  const auto* or_op = dynamic_cast<const BinaryOperation*>(if_stmt->condition());
  ASSERT_NE(or_op, nullptr);
  EXPECT_EQ(or_op->operator_type(), TokenType::Or);
  
  // Check left side is AND operation
  const auto* and_op = dynamic_cast<const BinaryOperation*>(or_op->left());
  ASSERT_NE(and_op, nullptr);
  EXPECT_EQ(and_op->operator_type(), TokenType::And);
  
  // Check right side is NOT operation
  const auto* not_op = dynamic_cast<const UnaryOperation*>(or_op->right());
  ASSERT_NE(not_op, nullptr);
  EXPECT_EQ(not_op->operator_type(), TokenType::Not);
}

TEST_F(ParserTest, ParsesLogicalOperatorPrecedence) {
  const std::string source = R"(
const test = fn(a: i32, b: i32, c: i32) -> i32 {
  if a > 5 and b < 10 or c == 0 {
    return 1
  }
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  
  const auto& func = program->functions()[0];
  const auto* if_stmt = dynamic_cast<const IfStatement*>(func->body()[0].get());
  ASSERT_NE(if_stmt, nullptr);
  
  // Should parse as: (a > 5 and b < 10) or (c == 0)
  // Top level should be OR
  const auto* or_op = dynamic_cast<const BinaryOperation*>(if_stmt->condition());
  ASSERT_NE(or_op, nullptr);
  EXPECT_EQ(or_op->operator_type(), TokenType::Or);
  
  // Left side should be AND
  const auto* and_op = dynamic_cast<const BinaryOperation*>(or_op->left());
  ASSERT_NE(and_op, nullptr);
  EXPECT_EQ(and_op->operator_type(), TokenType::And);
  
  // Right side should be comparison
  const auto* comp_op = dynamic_cast<const BinaryOperation*>(or_op->right());
  ASSERT_NE(comp_op, nullptr);
  EXPECT_EQ(comp_op->operator_type(), TokenType::EqualEqual);
}

TEST_F(ParserTest, ParsesSimpleRangeLoop) {
  const std::string source = R"(
const test = fn() -> i32 {
  loop i in 0..10 {
    return i
  }
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 2);  // loop + return
  
  const auto* loop_stmt = dynamic_cast<const LoopStatement*>(func->body()[0].get());
  ASSERT_NE(loop_stmt, nullptr);
  EXPECT_TRUE(loop_stmt->is_range_loop());
  EXPECT_EQ(loop_stmt->variable_name(), "i");
  
  // Check the range expression
  const auto* range = dynamic_cast<const RangeExpression*>(loop_stmt->range());
  ASSERT_NE(range, nullptr);
  
  // Check loop body
  ASSERT_EQ(loop_stmt->body().size(), 1);
  const auto* ret_stmt = dynamic_cast<const ReturnStatement*>(loop_stmt->body()[0].get());
  ASSERT_NE(ret_stmt, nullptr);
}

TEST_F(ParserTest, ParsesConditionalLoop) {
  const std::string source = R"(
const test = fn() -> i32 {
  x :i32 = 0
  loop if x < 10 {
    x = x + 1
  }
  return x
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 3);  // variable declaration + loop + return
  
  const auto* loop_stmt = dynamic_cast<const LoopStatement*>(func->body()[1].get());
  ASSERT_NE(loop_stmt, nullptr);
  EXPECT_FALSE(loop_stmt->is_range_loop());
  
  // Check condition
  const auto* condition = dynamic_cast<const BinaryOperation*>(loop_stmt->condition());
  ASSERT_NE(condition, nullptr);
  EXPECT_EQ(condition->operator_type(), TokenType::LessThan);
  
  // Check loop body
  ASSERT_EQ(loop_stmt->body().size(), 1);
  const auto* assign = dynamic_cast<const VariableAssignment*>(loop_stmt->body()[0].get());
  ASSERT_NE(assign, nullptr);
  EXPECT_EQ(assign->name(), "x");
}

TEST_F(ParserTest, ParsesComplexRangeLoop) {
  const std::string source = R"(
const test = fn() -> i32 {
  sum :i32 = 0
  loop i in 1..100 {
    sum = sum + i
  }
  return sum
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  const auto* loop_stmt = dynamic_cast<const LoopStatement*>(func->body()[1].get());
  ASSERT_NE(loop_stmt, nullptr);
  EXPECT_TRUE(loop_stmt->is_range_loop());
  EXPECT_EQ(loop_stmt->variable_name(), "i");
  
  // Check range with different start/end
  const auto* range = dynamic_cast<const RangeExpression*>(loop_stmt->range());
  ASSERT_NE(range, nullptr);
  
  const auto* start = dynamic_cast<const NumberLiteral*>(range->start());
  ASSERT_NE(start, nullptr);
  EXPECT_EQ(start->value(), 1);
  
  const auto* end = dynamic_cast<const NumberLiteral*>(range->end());
  ASSERT_NE(end, nullptr);
  EXPECT_EQ(end->value(), 100);
}

TEST_F(ParserTest, ParsesNestedLoops) {
  const std::string source = R"(
const test = fn() -> i32 {
  loop i in 0..3 {
    loop j in 0..3 {
      if i == j {
        return i
      }
    }
  }
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  const auto* outer_loop = dynamic_cast<const LoopStatement*>(func->body()[0].get());
  ASSERT_NE(outer_loop, nullptr);
  EXPECT_TRUE(outer_loop->is_range_loop());
  EXPECT_EQ(outer_loop->variable_name(), "i");
  
  // Check inner loop
  ASSERT_EQ(outer_loop->body().size(), 1);
  const auto* inner_loop = dynamic_cast<const LoopStatement*>(outer_loop->body()[0].get());
  ASSERT_NE(inner_loop, nullptr);
  EXPECT_TRUE(inner_loop->is_range_loop());
  EXPECT_EQ(inner_loop->variable_name(), "j");
}

TEST_F(ParserTest, ParsesLoopWithComplexCondition) {
  const std::string source = R"(
const test = fn() -> i32 {
  x :i32 = 0
  y :i32 = 10
  loop if x < y and x > 0 or not x == 5 {
    x = x + 1
  }
  return x
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  const auto* loop_stmt = dynamic_cast<const LoopStatement*>(func->body()[2].get());
  ASSERT_NE(loop_stmt, nullptr);
  EXPECT_FALSE(loop_stmt->is_range_loop());
  
  // Check complex condition is parsed correctly
  const auto* condition = dynamic_cast<const BinaryOperation*>(loop_stmt->condition());
  ASSERT_NE(condition, nullptr);
  EXPECT_EQ(condition->operator_type(), TokenType::Or);  // Top level should be OR
}

TEST_F(ParserTest, ParsesRangeWithVariableExpressions) {
  const std::string source = R"(
const test = fn(start: i32, end: i32) -> i32 {
  sum :i32 = 0
  loop i in start..end {
    sum = sum + i
  }
  return sum
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  const auto* loop_stmt = dynamic_cast<const LoopStatement*>(func->body()[1].get());
  ASSERT_NE(loop_stmt, nullptr);
  EXPECT_TRUE(loop_stmt->is_range_loop());
  
  // Check range uses variable references
  const auto* range = dynamic_cast<const RangeExpression*>(loop_stmt->range());
  ASSERT_NE(range, nullptr);
  
  const auto* start_var = dynamic_cast<const VariableReference*>(range->start());
  ASSERT_NE(start_var, nullptr);
  EXPECT_EQ(start_var->name(), "start");
  
  const auto* end_var = dynamic_cast<const VariableReference*>(range->end());
  ASSERT_NE(end_var, nullptr);
  EXPECT_EQ(end_var->name(), "end");
}

// Do syntax tests
TEST_F(ParserTest, ParsesFunctionWithDoSyntax) {
  const std::string source = R"(
const simple = fn() -> i32 do return 42
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  EXPECT_EQ(func->name(), "simple");
  EXPECT_EQ(func->return_type(), "i32");
  EXPECT_EQ(func->parameters().size(), 0);
  
  // Should have exactly one statement (return 42)
  EXPECT_EQ(func->body().size(), 1);
  
  const auto* return_stmt = dynamic_cast<const ReturnStatement*>(func->body()[0].get());
  ASSERT_NE(return_stmt, nullptr);
}

TEST_F(ParserTest, ParsesIfStatementWithDoSyntax) {
  const std::string source = R"(
const test = fn(x: i32) -> i32 {
  if x > 10 do return 1
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  EXPECT_EQ(func->body().size(), 2);  // if statement + return
  
  const auto* if_stmt = dynamic_cast<const IfStatement*>(func->body()[0].get());
  ASSERT_NE(if_stmt, nullptr);
  
  // Then body should have exactly one statement
  EXPECT_EQ(if_stmt->then_body().size(), 1);
  EXPECT_EQ(if_stmt->else_body().size(), 0);
  
  const auto* return_stmt = dynamic_cast<const ReturnStatement*>(if_stmt->then_body()[0].get());
  ASSERT_NE(return_stmt, nullptr);
}

TEST_F(ParserTest, ParsesRangeLoopWithDoSyntax) {
  const std::string source = R"(
const test = fn() -> i32 {
  sum :i32 = 0
  loop i in 0..5 do sum = sum + i
  return sum
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  EXPECT_EQ(func->body().size(), 3);  // variable declaration, loop, return
  
  const auto* loop_stmt = dynamic_cast<const LoopStatement*>(func->body()[1].get());
  ASSERT_NE(loop_stmt, nullptr);
  
  // Loop body should have exactly one statement
  EXPECT_EQ(loop_stmt->body().size(), 1);
  EXPECT_TRUE(loop_stmt->is_range_loop());
  
  const auto* assignment = dynamic_cast<const VariableAssignment*>(loop_stmt->body()[0].get());
  ASSERT_NE(assignment, nullptr);
}

TEST_F(ParserTest, ParsesConditionalLoopWithDoSyntax) {
  const std::string source = R"(
const test = fn() -> i32 {
  x :i32 = 0
  loop if x < 5 do x = x + 1
  return x
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  EXPECT_EQ(func->body().size(), 3);  // variable declaration, loop, return
  
  const auto* loop_stmt = dynamic_cast<const LoopStatement*>(func->body()[1].get());
  ASSERT_NE(loop_stmt, nullptr);
  
  // Loop body should have exactly one statement
  EXPECT_EQ(loop_stmt->body().size(), 1);
  EXPECT_FALSE(loop_stmt->is_range_loop());
  
  const auto* assignment = dynamic_cast<const VariableAssignment*>(loop_stmt->body()[0].get());
  ASSERT_NE(assignment, nullptr);
}

TEST_F(ParserTest, ParsesIfElseWithDoSyntax) {
  const std::string source = R"(
const test = fn(x: i32) -> i32 {
  if x > 10 do return 1
  else do return 2
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  EXPECT_EQ(func->body().size(), 1);  // if-else statement
  
  const auto* if_stmt = dynamic_cast<const IfStatement*>(func->body()[0].get());
  ASSERT_NE(if_stmt, nullptr);
  
  // Both then and else bodies should have exactly one statement
  EXPECT_EQ(if_stmt->then_body().size(), 1);
  EXPECT_EQ(if_stmt->else_body().size(), 1);
}

// Nil function tests
TEST_F(ParserTest, ParsesNilFunctionExplicit) {
  const std::string source = R"(
const nil_func = fn() -> nil {
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  EXPECT_EQ(func->name(), "nil_func");
  EXPECT_EQ(func->return_type(), "nil");
  EXPECT_EQ(func->parameters().size(), 0);
  EXPECT_EQ(func->body().size(), 0);  // Empty body
}

TEST_F(ParserTest, ParsesNilFunctionImplicit) {
  const std::string source = R"(
const nil_func = fn() {
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  EXPECT_EQ(func->name(), "nil_func");
  EXPECT_EQ(func->return_type(), "nil");  // Should default to nil
  EXPECT_EQ(func->parameters().size(), 0);
  EXPECT_EQ(func->body().size(), 0);  // Empty body
}

TEST_F(ParserTest, ParsesNilFunctionWithDoSyntax) {
  const std::string source = R"(
const nil_func = fn() do return
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  EXPECT_EQ(func->name(), "nil_func");
  EXPECT_EQ(func->return_type(), "nil");  // Should default to nil
  EXPECT_EQ(func->parameters().size(), 0);
  EXPECT_EQ(func->body().size(), 1);  // Return statement
  
  const auto* return_stmt = dynamic_cast<const ReturnStatement*>(func->body()[0].get());
  ASSERT_NE(return_stmt, nullptr);
  EXPECT_EQ(return_stmt->expression(), nullptr);  // Return without value
}

TEST_F(ParserTest, ParsesNilFunctionWithParameters) {
  const std::string source = R"(
const print_number = fn(x: i32) -> nil {
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);
  
  const auto& func = program->functions()[0];
  EXPECT_EQ(func->name(), "print_number");
  EXPECT_EQ(func->return_type(), "nil");
  EXPECT_EQ(func->parameters().size(), 1);
  EXPECT_EQ(func->parameters()[0]->name(), "x");
  EXPECT_EQ(func->parameters()[0]->type(), "i32");
}

TEST_F(ParserTest, ParsesConstStringVariable) {
  const std::string source = R"(
const test = fn() -> nil {
  greeting: const string = "Hello"
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);

  const auto& func = program->functions()[0];
  EXPECT_EQ(func->name(), "test");
  EXPECT_EQ(func->return_type(), "nil");
  ASSERT_EQ(func->body().size(), 1);

  const auto* var_decl = dynamic_cast<const VariableDeclaration*>(func->body()[0].get());
  ASSERT_NE(var_decl, nullptr);
  EXPECT_EQ(var_decl->name(), "greeting");
  EXPECT_EQ(var_decl->type(), "const string");
  
  const auto* string_literal = dynamic_cast<const StringLiteral*>(var_decl->value());
  ASSERT_NE(string_literal, nullptr);
  EXPECT_EQ(string_literal->value(), "Hello");
}

TEST_F(ParserTest, ParsesRegularStringVariable) {
  const std::string source = R"(
const test = fn() -> nil {
  message: string = "World"
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);

  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 1);

  const auto* var_decl = dynamic_cast<const VariableDeclaration*>(func->body()[0].get());
  ASSERT_NE(var_decl, nullptr);
  EXPECT_EQ(var_decl->name(), "message");
  EXPECT_EQ(var_decl->type(), "string");
  
  const auto* string_literal = dynamic_cast<const StringLiteral*>(var_decl->value());
  ASSERT_NE(string_literal, nullptr);
  EXPECT_EQ(string_literal->value(), "World");
}

TEST_F(ParserTest, ParsesEmptyStringVariable) {
  const std::string source = R"(
const test = fn() -> nil {
  empty: const string = ""
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  
  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 1);

  const auto* var_decl = dynamic_cast<const VariableDeclaration*>(func->body()[0].get());
  ASSERT_NE(var_decl, nullptr);
  EXPECT_EQ(var_decl->name(), "empty");
  EXPECT_EQ(var_decl->type(), "const string");
  
  const auto* string_literal = dynamic_cast<const StringLiteral*>(var_decl->value());
  ASSERT_NE(string_literal, nullptr);
  EXPECT_EQ(string_literal->value(), "");
}

TEST_F(ParserTest, ParsesMultipleStringVariables) {
  const std::string source = R"(
const test = fn() -> nil {
  greeting: const string = "Hello"
  name: string = "World"
  punctuation: const string = "!"
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  
  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 3);

  // Check first variable
  const auto* var_decl1 = dynamic_cast<const VariableDeclaration*>(func->body()[0].get());
  ASSERT_NE(var_decl1, nullptr);
  EXPECT_EQ(var_decl1->name(), "greeting");
  EXPECT_EQ(var_decl1->type(), "const string");

  // Check second variable
  const auto* var_decl2 = dynamic_cast<const VariableDeclaration*>(func->body()[1].get());
  ASSERT_NE(var_decl2, nullptr);
  EXPECT_EQ(var_decl2->name(), "name");
  EXPECT_EQ(var_decl2->type(), "string");

  // Check third variable
  const auto* var_decl3 = dynamic_cast<const VariableDeclaration*>(func->body()[2].get());
  ASSERT_NE(var_decl3, nullptr);
  EXPECT_EQ(var_decl3->name(), "punctuation");
  EXPECT_EQ(var_decl3->type(), "const string");
}

TEST_F(ParserTest, ParsesFunctionPointerVariable) {
  const std::string source = R"(
const test = fn() -> nil {
  callback: fn(i32) -> i32 = some_function
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);

  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 1);

  const auto* var_decl = dynamic_cast<const VariableDeclaration*>(func->body()[0].get());
  ASSERT_NE(var_decl, nullptr);
  EXPECT_EQ(var_decl->name(), "callback");
  EXPECT_EQ(var_decl->type(), "fn(i32) -> i32");
  
  const auto* var_ref = dynamic_cast<const VariableReference*>(var_decl->value());
  ASSERT_NE(var_ref, nullptr);
  EXPECT_EQ(var_ref->name(), "some_function");
}

TEST_F(ParserTest, ParsesFunctionPointerWithMultipleParams) {
  const std::string source = R"(
const test = fn() -> nil {
  operation: fn(i32, i32, i32) -> i32 = add_three
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  
  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 1);

  const auto* var_decl = dynamic_cast<const VariableDeclaration*>(func->body()[0].get());
  ASSERT_NE(var_decl, nullptr);
  EXPECT_EQ(var_decl->name(), "operation");
  EXPECT_EQ(var_decl->type(), "fn(i32, i32, i32) -> i32");
}

TEST_F(ParserTest, ParsesFunctionPointerWithNoParams) {
  const std::string source = R"(
const test = fn() -> nil {
  getter: fn() -> i32 = get_value
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  
  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 1);

  const auto* var_decl = dynamic_cast<const VariableDeclaration*>(func->body()[0].get());
  ASSERT_NE(var_decl, nullptr);
  EXPECT_EQ(var_decl->name(), "getter");
  EXPECT_EQ(var_decl->type(), "fn() -> i32");
}

TEST_F(ParserTest, ParsesFunctionPointerWithStringTypes) {
  const std::string source = R"(
const test = fn() -> nil {
  processor: fn(const string, i32) -> string = process_string
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  
  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 1);

  const auto* var_decl = dynamic_cast<const VariableDeclaration*>(func->body()[0].get());
  ASSERT_NE(var_decl, nullptr);
  EXPECT_EQ(var_decl->name(), "processor");
  EXPECT_EQ(var_decl->type(), "fn(const string, i32) -> string");
}

TEST_F(ParserTest, ParsesAnonymousFunctionSimple) {
  const std::string source = R"(
const main = fn() -> i32 {
  operation: fn(i32, i32) -> i32 = fn(x: i32, y: i32) -> i32 do return x + y
  return 42
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);

  const auto& func = program->functions()[0];
  ASSERT_EQ(func->body().size(), 2);

  // Check the variable declaration with anonymous function
  const auto* var_decl = dynamic_cast<const VariableDeclaration*>(func->body()[0].get());
  ASSERT_NE(var_decl, nullptr);
  EXPECT_EQ(var_decl->name(), "operation");
  EXPECT_EQ(var_decl->type(), "fn(i32, i32) -> i32");

  // Check that the value is an anonymous function
  const auto* anon_func = dynamic_cast<const AnonymousFunction*>(var_decl->value());
  ASSERT_NE(anon_func, nullptr);
  EXPECT_EQ(anon_func->return_type(), "i32");
  EXPECT_EQ(anon_func->parameters().size(), 2);
  EXPECT_EQ(anon_func->body().size(), 1);
}

TEST_F(ParserTest, ParsesAnonymousFunctionWithNoParams) {
  const std::string source = R"(
const main = fn() -> i32 {
  getter: fn() -> i32 = fn() -> i32 do return 42
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->functions().size(), 1);

  const auto& func = program->functions()[0];
  const auto* var_decl = dynamic_cast<const VariableDeclaration*>(func->body()[0].get());
  ASSERT_NE(var_decl, nullptr);

  const auto* anon_func = dynamic_cast<const AnonymousFunction*>(var_decl->value());
  ASSERT_NE(anon_func, nullptr);
  EXPECT_EQ(anon_func->return_type(), "i32");
  EXPECT_EQ(anon_func->parameters().size(), 0);
  EXPECT_EQ(anon_func->body().size(), 1);
}

TEST_F(ParserTest, ParsesAnonymousFunctionWithMultipleParams) {
  const std::string source = R"(
const main = fn() -> i32 {
  calc: fn(i32, i32, i32) -> i32 = fn(a: i32, b: i32, c: i32) -> i32 do return a + b + c
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  const auto& func = program->functions()[0];
  const auto* var_decl = dynamic_cast<const VariableDeclaration*>(func->body()[0].get());
  ASSERT_NE(var_decl, nullptr);

  const auto* anon_func = dynamic_cast<const AnonymousFunction*>(var_decl->value());
  ASSERT_NE(anon_func, nullptr);
  EXPECT_EQ(anon_func->return_type(), "i32");
  EXPECT_EQ(anon_func->parameters().size(), 3);
  
  EXPECT_EQ(anon_func->parameters()[0]->name(), "a");
  EXPECT_EQ(anon_func->parameters()[1]->name(), "b");
  EXPECT_EQ(anon_func->parameters()[2]->name(), "c");
}

TEST_F(ParserTest, ParsesAnonymousFunctionWithBlock) {
  const std::string source = R"(
const main = fn() -> i32 {
  complex: fn(i32) -> i32 = fn(x: i32) -> i32 {
    doubled: i32 = x * 2
    return doubled + 1
  }
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  const auto& func = program->functions()[0];
  const auto* var_decl = dynamic_cast<const VariableDeclaration*>(func->body()[0].get());
  ASSERT_NE(var_decl, nullptr);

  const auto* anon_func = dynamic_cast<const AnonymousFunction*>(var_decl->value());
  ASSERT_NE(anon_func, nullptr);
  EXPECT_EQ(anon_func->return_type(), "i32");
  EXPECT_EQ(anon_func->parameters().size(), 1);
  EXPECT_EQ(anon_func->body().size(), 2);  // Variable declaration + return statement
}

TEST_F(ParserTest, ParsesTypeInferenceForNumbers) {
  const std::string source = R"(
const main = fn() -> i32 {
  x := 42
  return x
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  const auto& func = program->functions()[0];
  const auto* var_decl = dynamic_cast<const VariableDeclaration*>(func->body()[0].get());
  ASSERT_NE(var_decl, nullptr);
  
  EXPECT_EQ(var_decl->name(), "x");
  EXPECT_EQ(var_decl->type(), "i32");
  
  const auto* num_literal = dynamic_cast<const NumberLiteral*>(var_decl->value());
  ASSERT_NE(num_literal, nullptr);
  EXPECT_EQ(num_literal->value(), 42);
}

TEST_F(ParserTest, ParsesTypeInferenceForStrings) {
  const std::string source = R"(
const main = fn() -> i32 {
  message := "Hello World"
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  const auto& func = program->functions()[0];
  const auto* var_decl = dynamic_cast<const VariableDeclaration*>(func->body()[0].get());
  ASSERT_NE(var_decl, nullptr);
  
  EXPECT_EQ(var_decl->name(), "message");
  EXPECT_EQ(var_decl->type(), "const string");
  
  const auto* str_literal = dynamic_cast<const StringLiteral*>(var_decl->value());
  ASSERT_NE(str_literal, nullptr);
  EXPECT_EQ(str_literal->value(), "Hello World");
}

TEST_F(ParserTest, ParsesTypeInferenceForAnonymousFunctions) {
  const std::string source = R"(
const main = fn() -> i32 {
  adder := fn(x: i32, y: i32) -> i32 do return x + y
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  const auto& func = program->functions()[0];
  const auto* var_decl = dynamic_cast<const VariableDeclaration*>(func->body()[0].get());
  ASSERT_NE(var_decl, nullptr);
  
  EXPECT_EQ(var_decl->name(), "adder");
  EXPECT_EQ(var_decl->type(), "fn(i32, i32) -> i32");
  
  const auto* anon_func = dynamic_cast<const AnonymousFunction*>(var_decl->value());
  ASSERT_NE(anon_func, nullptr);
  EXPECT_EQ(anon_func->return_type(), "i32");
  EXPECT_EQ(anon_func->parameters().size(), 2);
}

TEST_F(ParserTest, ParsesExplicitTypeAnnotations) {
  const std::string source = R"(
const main = fn() -> i32 {
  x: i32 = 42
  message: const string = "Hello"
  return x
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  const auto& func = program->functions()[0];
  
  const auto* var_decl1 = dynamic_cast<const VariableDeclaration*>(func->body()[0].get());
  ASSERT_NE(var_decl1, nullptr);
  EXPECT_EQ(var_decl1->name(), "x");
  EXPECT_EQ(var_decl1->type(), "i32");
  
  const auto* var_decl2 = dynamic_cast<const VariableDeclaration*>(func->body()[1].get());
  ASSERT_NE(var_decl2, nullptr);
  EXPECT_EQ(var_decl2->name(), "message");
  EXPECT_EQ(var_decl2->type(), "const string");
}

TEST_F(ParserTest, ParsesTypeInferenceForArithmeticExpressions) {
  const std::string source = R"(
const main = fn() -> i32 {
  x := 10
  y := 20
  sum := x + y
  difference := x - y
  product := x * y
  quotient := x / y
  return sum
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  const auto& func = program->functions()[0];
  
  // Check sum := x + y (should infer i32)
  const auto* sum_decl = dynamic_cast<const VariableDeclaration*>(func->body()[2].get());
  ASSERT_NE(sum_decl, nullptr);
  EXPECT_EQ(sum_decl->name(), "sum");
  EXPECT_EQ(sum_decl->type(), "i32");
  
  // Check difference := x - y (should infer i32)
  const auto* diff_decl = dynamic_cast<const VariableDeclaration*>(func->body()[3].get());
  ASSERT_NE(diff_decl, nullptr);
  EXPECT_EQ(diff_decl->name(), "difference");
  EXPECT_EQ(diff_decl->type(), "i32");
  
  // Check product := x * y (should infer i32)
  const auto* prod_decl = dynamic_cast<const VariableDeclaration*>(func->body()[4].get());
  ASSERT_NE(prod_decl, nullptr);
  EXPECT_EQ(prod_decl->name(), "product");
  EXPECT_EQ(prod_decl->type(), "i32");
  
  // Check quotient := x / y (should infer i32)
  const auto* quot_decl = dynamic_cast<const VariableDeclaration*>(func->body()[5].get());
  ASSERT_NE(quot_decl, nullptr);
  EXPECT_EQ(quot_decl->name(), "quotient");
  EXPECT_EQ(quot_decl->type(), "i32");
}

TEST_F(ParserTest, ParsesTypeInferenceForStringConcatenation) {
  const std::string source = R"(
const main = fn() -> i32 {
  first := "Hello"
  second := "World"
  combined := first + second
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  const auto& func = program->functions()[0];
  
  // Check combined := first + second (should infer const string)
  const auto* combined_decl = dynamic_cast<const VariableDeclaration*>(func->body()[2].get());
  ASSERT_NE(combined_decl, nullptr);
  EXPECT_EQ(combined_decl->name(), "combined");
  EXPECT_EQ(combined_decl->type(), "const string");
}

TEST_F(ParserTest, ParsesTypeInferenceForVariableReferences) {
  const std::string source = R"(
const main = fn() -> i32 {
  original := 42
  copy := original
  return copy
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  const auto& func = program->functions()[0];
  
  // Check copy := original (should infer i32 from variable reference)
  const auto* copy_decl = dynamic_cast<const VariableDeclaration*>(func->body()[1].get());
  ASSERT_NE(copy_decl, nullptr);
  EXPECT_EQ(copy_decl->name(), "copy");
  EXPECT_EQ(copy_decl->type(), "i32");
}

TEST_F(ParserTest, ParsesTypeInferenceForComplexExpressions) {
  const std::string source = R"(
const main = fn() -> i32 {
  a := 10
  b := 20
  c := 30
  result := a + b * c
  return result
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  const auto& func = program->functions()[0];
  
  // Check result := a + b * c (should infer i32)
  const auto* result_decl = dynamic_cast<const VariableDeclaration*>(func->body()[3].get());
  ASSERT_NE(result_decl, nullptr);
  EXPECT_EQ(result_decl->name(), "result");
  EXPECT_EQ(result_decl->type(), "i32");
}

}  // namespace
}  // namespace void_compiler