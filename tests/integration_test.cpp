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
  const auto source = void_compiler::SourcePath{R"(
const main = fn() -> i32 {
  return 123
}
)"};

  // Test that compilation to executable succeeds
  bool success = compiler_.compile_to_executable(source, void_compiler::OutputPath{"test_executable"});
  EXPECT_TRUE(success);
  
  // Clean up the generated executable
  std::remove("test_executable");
}

TEST_F(IntegrationTest, CompileAndRunLocalVariable) {
  const std::string source = R"(
const main = fn() -> i32 {
  x :i32 = 42
  return x
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 42);
}

TEST_F(IntegrationTest, CompileAndRunMultipleLocalVariables) {
  const std::string source = R"(
const main = fn() -> i32 {
  x :i32 = 10
  y :i32 = 20
  z :i32 = x + y
  return z
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 30);
}

TEST_F(IntegrationTest, CompileAndRunVariablesWithComplexExpressions) {
  const std::string source = R"(
const main = fn() -> i32 {
  a :i32 = 5
  b :i32 = 3
  sum :i32 = a + b
  product :i32 = a * b
  result :i32 = sum + product
  return result
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 23); // (5+3) + (5*3) = 8 + 15 = 23
}

TEST_F(IntegrationTest, CompileAndRunVariablesWithParameters) {
  const std::string source = R"(
const calculate = fn(x: i32, y: i32) -> i32 {
  doubled_x :i32 = x * 2
  halved_y :i32 = y / 2
  result :i32 = doubled_x + halved_y
  return result
}

const main = fn() -> i32 {
  return calculate(10, 8)
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 24); // (10*2) + (8/2) = 20 + 4 = 24
}

TEST_F(IntegrationTest, CompileAndRunVariableReassignment) {
  const std::string source = R"(
const main = fn() -> i32 {
  x :i32 = 100
  x = x * 2
  return x
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 200); // 100 * 2 = 200
}

TEST_F(IntegrationTest, CompileAndRunMultipleReassignments) {
  const std::string source = R"(
const main = fn() -> i32 {
  x :i32 = 10
  y :i32 = 5
  x = x + y
  y = x - 3
  x = y * 2
  return x + y
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 36); // x=15, y=12, x=24, return 24+12=36
}

TEST_F(IntegrationTest, CompileAndRunParameterReassignment) {
  const std::string source = R"(
const modify = fn(x: i32, y: i32) -> i32 {
  x = x + 10
  y = y * 2
  return x + y
}

const main = fn() -> i32 {
  return modify(5, 3)
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 21); // (5+10) + (3*2) = 15 + 6 = 21
}

TEST_F(IntegrationTest, CompileAndRunComplexReassignmentPattern) {
  const std::string source = R"(
const main = fn() -> i32 {
  counter :i32 = 0
  counter = counter + 1
  counter = counter * 5
  counter = counter - 2
  return counter
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 3); // ((0+1)*5)-2 = 3
}

TEST_F(IntegrationTest, CompileAndRunSimpleIfStatement) {
  const std::string source = R"(
const main = fn() -> i32 {
  x :i32 = 15
  if x > 10 {
    return 1
  }
  return 0
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 1);
}

TEST_F(IntegrationTest, CompileAndRunIfElseStatement) {
  const std::string source = R"(
const test = fn(x: i32) -> i32 {
  if x > 10 {
    return 1
  } else {
    return 2
  }
}

const main = fn() -> i32 {
  return test(5)
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 2); // 5 is not > 10, so returns 2
}

TEST_F(IntegrationTest, CompileAndRunIfElseIfElseChain) {
  const std::string source = R"(
const classify = fn(x: i32) -> i32 {
  if x > 20 {
    return 3
  } else if x > 10 {
    return 2
  } else {
    return 1
  }
}

const main = fn() -> i32 {
  return classify(15)
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 2); // 15 is > 10 but not > 20
}

TEST_F(IntegrationTest, CompileAndRunAllComparisonOperators) {
  const std::string source = R"(
const test = fn() -> i32 {
  a :i32 = 10
  b :i32 = 5
  
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

const main = fn() -> i32 {
  return test()
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 1); // 10 > 5 is true
}

TEST_F(IntegrationTest, CompileAndRunLogicalAndExpression) {
  const std::string source = R"(
const test = fn(a: i32, b: i32) -> i32 {
  if a > 10 and b < 100 {
    return 1
  } else {
    return 2
  }
}

const main = fn() -> i32 {
  return test(15, 50)
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 1); // 15 > 10 AND 50 < 100 = true AND true = true
}

TEST_F(IntegrationTest, CompileAndRunLogicalOrExpression) {
  const std::string source = R"(
const test = fn(a: i32, b: i32) -> i32 {
  if a > 100 or b < 10 {
    return 1
  } else {
    return 2
  }
}

const main = fn() -> i32 {
  return test(15, 5)
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 1); // 15 > 100 OR 5 < 10 = false OR true = true
}

TEST_F(IntegrationTest, CompileAndRunLogicalNotExpression) {
  const std::string source = R"(
const test = fn(a: i32) -> i32 {
  if not a > 20 {
    return 1
  } else {
    return 2
  }
}

const main = fn() -> i32 {
  return test(15)
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 1); // NOT(15 > 20) = NOT(false) = true
}

TEST_F(IntegrationTest, CompileAndRunComplexLogicalExpression) {
  const std::string source = R"(
const test = fn(a: i32, b: i32, c: i32) -> i32 {
  if a > 10 and b < 100 or not c == 0 {
    return 1
  } else {
    return 2
  }
}

const main = fn() -> i32 {
  return test(5, 50, 10)
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 1); // (5>10 AND 50<100) OR NOT(10==0) = (false AND true) OR NOT(false) = false OR true = true
}

TEST_F(IntegrationTest, CompileAndRunLogicalOperatorPrecedence) {
  const std::string source = R"(
const test = fn(a: i32, b: i32, c: i32) -> i32 {
  if a > 0 and b > 0 or c > 0 {
    return 1
  } else {
    return 2
  }
}

const main = fn() -> i32 {
  neg_a :i32 = 0 - 1
  neg_b :i32 = 0 - 2  
  pos_c :i32 = 5
  return test(neg_a, neg_b, pos_c)
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 1); // (-1>0 AND -2>0) OR 5>0 = (false AND false) OR true = false OR true = true
}

TEST_F(IntegrationTest, CompileAndRunNestedIfStatements) {
  const std::string source = R"(
const test = fn(x: i32, y: i32) -> i32 {
  if x > 0 {
    if y > 0 {
      return 1
    } else {
      return 2
    }
  } else {
    if y > 0 {
      return 3
    } else {
      return 4
    }
  }
}

const main = fn() -> i32 {
  neg_y :i32 = 0 - 3
  return test(5, neg_y)
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 2); // x=5>0 is true, y=-3>0 is false, so return 2
}

TEST_F(IntegrationTest, CompileAndRunControlFlowWithVariables) {
  const std::string source = R"(
const main = fn() -> i32 {
  score :i32 = 85
  grade :i32 = 0
  
  if score >= 90 {
    grade = 4
  } else if score >= 80 {
    grade = 3
  } else if score >= 70 {
    grade = 2
  } else if score >= 60 {
    grade = 1
  } else {
    grade = 0
  }
  
  return grade
}
)";

  int result = compiler_.compile_and_run(source);
  EXPECT_EQ(result, 3); // score=85 >= 80 but < 90, so grade = 3
}

}  // namespace
}  // namespace void_compiler
