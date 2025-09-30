#include <gtest/gtest.h>

#include "compiler.h"

namespace void_compiler {
namespace {

class IntegrationTest : public testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}

  Compiler compiler_;
};

TEST_F(IntegrationTest, CompileAndRunSimpleFunction) {
  const std::string source = R"(
const main = fn() -> i32 {
  return 42
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 42);
}

TEST_F(IntegrationTest, CompileAndRunFunctionWithParameters) {
  const std::string source = R"(
const add = fn(x: i32, y: i32) -> i32 {
  return x + y
}

const main = fn() -> i32 {
  return add(5, 3)
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 8);
}

TEST_F(IntegrationTest, CompileAndRunArithmeticExpression) {
  const std::string source = R"(
const calculate = fn(a: i32, b: i32, c: i32) -> i32 {
  return a + b * c - a / b
}

const main = fn() -> i32 {
  return calculate(10, 5, 3)
}
)";

  int result = compiler_.compile_and_run(source);
  // 10 + 5 * 3 - 10 / 5 = 10 + 15 - 2 = 23
  EXPECT_EQ(result, 23);
}

TEST_F(IntegrationTest, CompileAndRunComplexExpression) {
  const std::string source = R"(
const helper = fn(x: i32) -> i32 {
  return x * 2
}

const main = fn() -> i32 {
  return helper(5) + helper(3) * 2
}
)";

  int result = compiler_.compile_and_run(source);
  // helper(5) = 10, helper(3) = 6, 10 + 6 * 2 = 10 + 12 = 22
  EXPECT_EQ(result, 22);
}

TEST_F(IntegrationTest, CompileAndRunNestedFunctionCalls) {
  const std::string source = R"(
const add = fn(x: i32, y: i32) -> i32 {
  return x + y
}

const multiply = fn(a: i32, b: i32) -> i32 {
  return a * b
}

const main = fn() -> i32 {
  return add(multiply(2, 3), multiply(4, 5))
}
)";

  int result = compiler_.compile_and_run(source);
  // multiply(2, 3) = 6, multiply(4, 5) = 20, add(6, 20) = 26
  EXPECT_EQ(result, 26);
}

TEST_F(IntegrationTest, CompileAndRunWithParentheses) {
  const std::string source = R"(
const calculate = fn(x: i32, y: i32) -> i32 {
  return (x + y) * (x - y)
}

const main = fn() -> i32 {
  return calculate(7, 3)
}
)";

  int result = compiler_.compile_and_run(source);
  // (7 + 3) * (7 - 3) = 10 * 4 = 40
  EXPECT_EQ(result, 40);
}

TEST_F(IntegrationTest, CompileAndRunDivision) {
  const std::string source = R"(
const divide = fn(x: i32, y: i32) -> i32 {
  return x / y
}

const main = fn() -> i32 {
  return divide(15, 3)
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 5);
}

TEST_F(IntegrationTest, CompileAndRunOperatorPrecedence) {
  const std::string source = R"(
const main = fn() -> i32 {
  return 2 + 3 * 4 - 8 / 2
}
)";

  int result = compiler_.compile_and_run(source);
  // 2 + 3 * 4 - 8 / 2 = 2 + 12 - 4 = 10
  EXPECT_EQ(result, 10);
}

TEST_F(IntegrationTest, CompileAndRunVariableReferences) {
  const std::string source = R"(
const identity = fn(value: i32) -> i32 {
  return value
}

const main = fn() -> i32 {
  return identity(99)
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 99);
}

TEST_F(IntegrationTest, CompileToExecutableSucceeds) {
  const std::string source = R"(
const main = fn() -> i32 {
  return 123
}
)";

  // Test that compilation to executable succeeds
  bool success = compiler_.compile_to_executable(source, "test_executable");
  EXPECT_TRUE(success);
  
  // Clean up the generated executable
  std::remove("test_executable");
}

}  // namespace
}  // namespace void_compiler