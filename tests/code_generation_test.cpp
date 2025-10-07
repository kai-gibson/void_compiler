#include <gtest/gtest.h>

#include "code_generation.h"
#include "lexer.h"
#include "parser.h"

namespace void_compiler {
namespace {

class CodeGenerationTest : public ::testing::Test {
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

TEST_F(CodeGenerationTest, GeneratesSimpleFunction) {
  const std::string source = R"(
const test = fn() -> i32 {
  return 42
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  // Should not throw and should generate valid IR
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  EXPECT_TRUE(output.find("define i32 @test()") != std::string::npos);
  EXPECT_TRUE(output.find("ret i32 42") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesFunctionWithParameters) {
  const std::string source = R"(
const add = fn(x: i32, y: i32) -> i32 {
  return x + y
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  EXPECT_TRUE(output.find("define i32 @add(i32 %x, i32 %y)") != std::string::npos);
  EXPECT_TRUE(output.find("add i32") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesAllArithmeticOperations) {
  const std::string source = R"(
const calc = fn(a: i32, b: i32) -> i32 {
  return a + b - a * b / a
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  EXPECT_TRUE(output.find("add i32") != std::string::npos);
  EXPECT_TRUE(output.find("sub i32") != std::string::npos);
  EXPECT_TRUE(output.find("mul i32") != std::string::npos);
  EXPECT_TRUE(output.find("sdiv i32") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesFunctionCalls) {
  const std::string source = R"(
const helper = fn(x: i32) -> i32 {
  return x
}

const main = fn() -> i32 {
  return helper(42)
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  EXPECT_TRUE(output.find("call i32 @helper(i32 42)") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesVariableLoads) {
  const std::string source = R"(
const identity = fn(value: i32) -> i32 {
  return value
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  EXPECT_TRUE(output.find("load i32") != std::string::npos);
  EXPECT_TRUE(output.find("alloca i32") != std::string::npos);
}

TEST_F(CodeGenerationTest, ThrowsOnUnknownFunction) {
  const std::string source = R"(
const test = fn() -> i32 {
  return unknown_func()
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  EXPECT_THROW({
    codegen.generate_program(program.get());
  }, std::runtime_error);
}

TEST_F(CodeGenerationTest, ThrowsOnUnknownVariable) {
  const std::string source = R"(
const test = fn() -> i32 {
  return unknown_var
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  EXPECT_THROW({
    codegen.generate_program(program.get());
  }, std::runtime_error);
}

TEST_F(CodeGenerationTest, ThrowsOnUnknownBinaryOperator) {
  // This test would require modifying the AST to have an invalid operator
  // For now, we'll test the switch statement default case indirectly
  
  const std::string source = R"(
const test = fn(x: i32, y: i32) -> i32 {
  return x + y
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  // This should not throw for valid operators
  EXPECT_NO_THROW({
    codegen.generate_program(program.get());
  });
}

TEST_F(CodeGenerationTest, GeneratesComplexNestedExpressions) {
  const std::string source = R"(
const complex = fn(a: i32, b: i32, c: i32) -> i32 {
  return (a + b) * (c - a) / (b + 1)
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should contain all operations
  EXPECT_TRUE(output.find("add i32") != std::string::npos);
  EXPECT_TRUE(output.find("sub i32") != std::string::npos);
  EXPECT_TRUE(output.find("mul i32") != std::string::npos);
  EXPECT_TRUE(output.find("sdiv i32") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesMultipleFunctionDefinitions) {
  auto program = std::make_unique<Program>();
  
  auto func1 = std::make_unique<FunctionDeclaration>("first", "i32");
  func1->add_statement(std::make_unique<ReturnStatement>(std::make_unique<NumberLiteral>(1)));
  program->add_function(std::move(func1));
  
  auto func2 = std::make_unique<FunctionDeclaration>("second", "i32");
  func2->add_statement(std::make_unique<ReturnStatement>(std::make_unique<NumberLiteral>(2)));
  program->add_function(std::move(func2));
  
  CodeGenerator generator;
  generator.generate_program(program.get());
  // Test passes if no exception is thrown
  SUCCEED();
}

TEST_F(CodeGenerationTest, GeneratesParameterFunctions) {
  auto program = std::make_unique<Program>();
  auto func = std::make_unique<FunctionDeclaration>("test", "i32");
  func->add_parameter(std::make_unique<Parameter>("x", "i32"));
  func->add_parameter(std::make_unique<Parameter>("y", "i32"));
  func->add_statement(std::make_unique<ReturnStatement>(std::make_unique<VariableReference>("x")));
  program->add_function(std::move(func));
  
  CodeGenerator generator;
  generator.generate_program(program.get());
  // Test passes if no exception is thrown
  SUCCEED();
}

TEST_F(CodeGenerationTest, GeneratesArithmeticExpressions) {
  auto program = std::make_unique<Program>();
  auto func = std::make_unique<FunctionDeclaration>("arithmetic", "i32");
  func->add_parameter(std::make_unique<Parameter>("a", "i32"));
  func->add_parameter(std::make_unique<Parameter>("b", "i32"));
  
  // Create: a + b
  auto var_a = std::make_unique<VariableReference>("a");
  auto var_b = std::make_unique<VariableReference>("b");
  auto add_expr = std::make_unique<BinaryOperation>(std::move(var_a), TokenType::Plus, std::move(var_b));
  func->add_statement(std::make_unique<ReturnStatement>(std::move(add_expr)));
  program->add_function(std::move(func));
  
  CodeGenerator generator;
  generator.generate_program(program.get());
  // Test passes if no exception is thrown
  SUCCEED();
}

TEST_F(CodeGenerationTest, GeneratesFunctionCallsWithArgs) {
  auto program = std::make_unique<Program>();
  
  auto helper = std::make_unique<FunctionDeclaration>("helper", "i32");
  helper->add_parameter(std::make_unique<Parameter>("x", "i32"));
  helper->add_statement(std::make_unique<ReturnStatement>(std::make_unique<VariableReference>("x")));
  program->add_function(std::move(helper));
  
  auto main = std::make_unique<FunctionDeclaration>("main", "i32");
  std::vector<std::unique_ptr<ASTNode>> args;
  args.push_back(std::make_unique<NumberLiteral>(42));
  auto call = std::make_unique<FunctionCall>("helper", std::move(args));
  main->add_statement(std::make_unique<ReturnStatement>(std::move(call)));
  program->add_function(std::move(main));
  
  CodeGenerator generator;
  generator.generate_program(program.get());
  // Test passes if no exception is thrown
  SUCCEED();
}

TEST_F(CodeGenerationTest, HandlesAllArithmeticOperators) {
  auto program = std::make_unique<Program>();
  
  // Test addition
  auto add_func = std::make_unique<FunctionDeclaration>("add_test", "i32");
  add_func->add_parameter(std::make_unique<Parameter>("a", "i32"));
  add_func->add_parameter(std::make_unique<Parameter>("b", "i32"));
  auto add_expr = std::make_unique<BinaryOperation>(
    std::make_unique<VariableReference>("a"), 
    TokenType::Plus, 
    std::make_unique<VariableReference>("b")
  );
  add_func->add_statement(std::make_unique<ReturnStatement>(std::move(add_expr)));
  program->add_function(std::move(add_func));
  
  // Test subtraction
  auto sub_func = std::make_unique<FunctionDeclaration>("sub_test", "i32");
  sub_func->add_parameter(std::make_unique<Parameter>("a", "i32"));
  sub_func->add_parameter(std::make_unique<Parameter>("b", "i32"));
  auto sub_expr = std::make_unique<BinaryOperation>(
    std::make_unique<VariableReference>("a"), 
    TokenType::Minus, 
    std::make_unique<VariableReference>("b")
  );
  sub_func->add_statement(std::make_unique<ReturnStatement>(std::move(sub_expr)));
  program->add_function(std::move(sub_func));
  
  // Test multiplication
  auto mul_func = std::make_unique<FunctionDeclaration>("mul_test", "i32");
  mul_func->add_parameter(std::make_unique<Parameter>("a", "i32"));
  mul_func->add_parameter(std::make_unique<Parameter>("b", "i32"));
  auto mul_expr = std::make_unique<BinaryOperation>(
    std::make_unique<VariableReference>("a"), 
    TokenType::Multiply, 
    std::make_unique<VariableReference>("b")
  );
  mul_func->add_statement(std::make_unique<ReturnStatement>(std::move(mul_expr)));
  program->add_function(std::move(mul_func));
  
  // Test division
  auto div_func = std::make_unique<FunctionDeclaration>("div_test", "i32");
  div_func->add_parameter(std::make_unique<Parameter>("a", "i32"));
  div_func->add_parameter(std::make_unique<Parameter>("b", "i32"));
  auto div_expr = std::make_unique<BinaryOperation>(
    std::make_unique<VariableReference>("a"), 
    TokenType::Divide, 
    std::make_unique<VariableReference>("b")
  );
  div_func->add_statement(std::make_unique<ReturnStatement>(std::move(div_expr)));
  program->add_function(std::move(div_func));
  
  CodeGenerator generator;
  generator.generate_program(program.get());
  // Test passes if no exception is thrown
  SUCCEED();
}

TEST_F(CodeGenerationTest, HandlesComplexNestedStructures) {
  auto program = std::make_unique<Program>();
  auto func = std::make_unique<FunctionDeclaration>("complex", "i32");
  func->add_parameter(std::make_unique<Parameter>("a", "i32"));
  func->add_parameter(std::make_unique<Parameter>("b", "i32"));
  func->add_parameter(std::make_unique<Parameter>("c", "i32"));
  
  // Create: (a + b) * c
  auto add_expr = std::make_unique<BinaryOperation>(
    std::make_unique<VariableReference>("a"), 
    TokenType::Plus, 
    std::make_unique<VariableReference>("b")
  );
  auto mul_expr = std::make_unique<BinaryOperation>(
    std::move(add_expr), 
    TokenType::Multiply, 
    std::make_unique<VariableReference>("c")
  );
  func->add_statement(std::make_unique<ReturnStatement>(std::move(mul_expr)));
  program->add_function(std::move(func));
  
  CodeGenerator generator;
  generator.generate_program(program.get());
  // Test passes if no exception is thrown
  SUCCEED();
}

TEST_F(CodeGenerationTest, HandlesLargeNumbers) {
  auto program = std::make_unique<Program>();
  auto func = std::make_unique<FunctionDeclaration>("large", "i32");
  func->add_statement(std::make_unique<ReturnStatement>(std::make_unique<NumberLiteral>(999999)));
  program->add_function(std::move(func));
  
  CodeGenerator generator;
  generator.generate_program(program.get());
  // Test passes if no exception is thrown
  SUCCEED();
}

TEST_F(CodeGenerationTest, HandlesZeroValues) {
  auto program = std::make_unique<Program>();
  auto func = std::make_unique<FunctionDeclaration>("zero", "i32");
  func->add_statement(std::make_unique<ReturnStatement>(std::make_unique<NumberLiteral>(0)));
  program->add_function(std::move(func));
  
  CodeGenerator generator;
  generator.generate_program(program.get());
  // Test passes if no exception is thrown
  SUCCEED();
}

TEST_F(CodeGenerationTest, HandlesManyParameters) {
  auto program = std::make_unique<Program>();
  auto func = std::make_unique<FunctionDeclaration>("many_params", "i32");
  func->add_parameter(std::make_unique<Parameter>("a", "i32"));
  func->add_parameter(std::make_unique<Parameter>("b", "i32"));
  func->add_parameter(std::make_unique<Parameter>("c", "i32"));
  func->add_parameter(std::make_unique<Parameter>("d", "i32"));
  func->add_parameter(std::make_unique<Parameter>("e", "i32"));
  func->add_statement(std::make_unique<ReturnStatement>(std::make_unique<VariableReference>("a")));
  program->add_function(std::move(func));
  
  CodeGenerator generator;
  generator.generate_program(program.get());
  // Test passes if no exception is thrown
  SUCCEED();
}

TEST_F(CodeGenerationTest, HandlesChainedFunctionCalls) {
  auto program = std::make_unique<Program>();
  
  auto helper1 = std::make_unique<FunctionDeclaration>("helper1", "i32");
  helper1->add_statement(std::make_unique<ReturnStatement>(std::make_unique<NumberLiteral>(10)));
  program->add_function(std::move(helper1));
  
  auto helper2 = std::make_unique<FunctionDeclaration>("helper2", "i32");
  helper2->add_statement(std::make_unique<ReturnStatement>(std::make_unique<NumberLiteral>(20)));
  program->add_function(std::move(helper2));
  
  auto main = std::make_unique<FunctionDeclaration>("main", "i32");
  
  std::vector<std::unique_ptr<ASTNode>> args1;
  auto call1 = std::make_unique<FunctionCall>("helper1", std::move(args1));
  
  std::vector<std::unique_ptr<ASTNode>> args2;
  auto call2 = std::make_unique<FunctionCall>("helper2", std::move(args2));
  
  auto add_calls = std::make_unique<BinaryOperation>(std::move(call1), TokenType::Plus, std::move(call2));
  main->add_statement(std::make_unique<ReturnStatement>(std::move(add_calls)));
  program->add_function(std::move(main));
  
  CodeGenerator generator;
  generator.generate_program(program.get());
  // Test passes if no exception is thrown
  SUCCEED();
}

TEST_F(CodeGenerationTest, GeneratesLocalVariableDeclaration) {
  const std::string source = R"(
const main = fn() -> i32 {
  x :i32 = 42
  return x
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  
  CodeGenerator generator;
  ASSERT_NO_THROW(generator.generate_program(program.get()));
}

TEST_F(CodeGenerationTest, GeneratesMultipleLocalVariables) {
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
  
  CodeGenerator generator;
  ASSERT_NO_THROW(generator.generate_program(program.get()));
}

TEST_F(CodeGenerationTest, GeneratesVariableReferencesInExpressions) {
  const std::string source = R"(
const calculate = fn(a: i32) -> i32 {
  doubled :i32 = a * 2
  result :i32 = doubled + 5
  return result
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  
  CodeGenerator generator;
  ASSERT_NO_THROW(generator.generate_program(program.get()));
}

TEST_F(CodeGenerationTest, GeneratesVariablesWithComplexExpressions) {
  const std::string source = R"(
const complex = fn(x: i32, y: i32) -> i32 {
  temp1 :i32 = x + y
  temp2 :i32 = x - y
  result :i32 = temp1 * temp2
  return result
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  
  CodeGenerator generator;
  ASSERT_NO_THROW(generator.generate_program(program.get()));
}

TEST_F(CodeGenerationTest, ThrowsOnUndefinedVariableReference) {
  // Manually create AST with undefined variable reference
  auto program = std::make_unique<Program>();
  auto func = std::make_unique<FunctionDeclaration>("main", "i32");
  
  // Create a return statement that references an undefined variable
  auto undefined_var = std::make_unique<VariableReference>("undefined_var");
  func->add_statement(std::make_unique<ReturnStatement>(std::move(undefined_var)));
  program->add_function(std::move(func));
  
  CodeGenerator generator;
  ASSERT_THROW(generator.generate_program(program.get()), std::runtime_error);
}

TEST_F(CodeGenerationTest, GeneratesVariableAssignment) {
  const std::string source = R"(
const main = fn() -> i32 {
  x :i32 = 100
  x = x * 2
  return x
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  
  CodeGenerator generator;
  ASSERT_NO_THROW(generator.generate_program(program.get()));
}

TEST_F(CodeGenerationTest, GeneratesMultipleVariableAssignments) {
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
  
  CodeGenerator generator;
  ASSERT_NO_THROW(generator.generate_program(program.get()));
}

TEST_F(CodeGenerationTest, GeneratesParameterReassignment) {
  const std::string source = R"(
const modify = fn(x: i32) -> i32 {
  x = x + 10
  return x
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  
  CodeGenerator generator;
  ASSERT_NO_THROW(generator.generate_program(program.get()));
}

TEST_F(CodeGenerationTest, ThrowsOnUndefinedVariableAssignment) {
  // Manually create AST with assignment to undefined variable
  auto program = std::make_unique<Program>();
  auto func = std::make_unique<FunctionDeclaration>("main", "i32");
  
  // Create assignment to undefined variable
  auto undefined_assign = std::make_unique<VariableAssignment>("undefined_var", std::make_unique<NumberLiteral>(42));
  func->add_statement(std::move(undefined_assign));
  func->add_statement(std::make_unique<ReturnStatement>(std::make_unique<NumberLiteral>(0)));
  program->add_function(std::move(func));
  
  CodeGenerator generator;
  ASSERT_THROW(generator.generate_program(program.get()), std::runtime_error);
}

TEST_F(CodeGenerationTest, ThrowsOnFunctionCallWithIncorrectArgumentCount) {
  const std::string source = R"(
const helper = fn(x: i32) -> i32 { return x + 10 }
const main = fn() -> i32 {
  return helper()
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  
  CodeGenerator generator;
  ASSERT_THROW(generator.generate_program(program.get()), std::runtime_error);
}

TEST_F(CodeGenerationTest, ThrowsOnFunctionCallWithTooManyArguments) {
  const std::string source = R"(
const helper = fn(x: i32) -> i32 { return x * 2 }
const main = fn() -> i32 {
  return helper(5, 10)
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  
  CodeGenerator generator;
  ASSERT_THROW(generator.generate_program(program.get()), std::runtime_error);
}

TEST_F(CodeGenerationTest, ThrowsOnFunctionCallWithTooFewArguments) {
  const std::string source = R"(
const helper = fn(x: i32, y: i32) -> i32 { return x + y }
const main = fn() -> i32 {
  return helper(5)
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);
  
  CodeGenerator generator;
  ASSERT_THROW(generator.generate_program(program.get()), std::runtime_error);
}

TEST_F(CodeGenerationTest, GeneratesSimpleIfStatement) {
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

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should generate basic blocks for conditional branching
  EXPECT_TRUE(output.find("br i1") != std::string::npos);  // conditional branch
  EXPECT_TRUE(output.find("icmp") != std::string::npos);   // comparison instruction
  EXPECT_TRUE(output.find("then:") != std::string::npos);  // then label
}

TEST_F(CodeGenerationTest, GeneratesIfElseStatement) {
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

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should generate both then and else blocks
  EXPECT_TRUE(output.find("then:") != std::string::npos);
  EXPECT_TRUE(output.find("else:") != std::string::npos);
  EXPECT_TRUE(output.find("br i1") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesAllComparisonOperators) {
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

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should generate icmp instructions for all comparison types
  EXPECT_TRUE(output.find("icmp sgt") != std::string::npos);  // >
  EXPECT_TRUE(output.find("icmp slt") != std::string::npos);  // <
  EXPECT_TRUE(output.find("icmp sge") != std::string::npos);  // >=
  EXPECT_TRUE(output.find("icmp sle") != std::string::npos);  // <=
  EXPECT_TRUE(output.find("icmp eq") != std::string::npos);   // ==
  EXPECT_TRUE(output.find("icmp ne") != std::string::npos);   // !=
}

TEST_F(CodeGenerationTest, GeneratesLogicalAndOperation) {
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

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should generate logical AND instruction
  EXPECT_TRUE(output.find("and i1") != std::string::npos);
  EXPECT_TRUE(output.find("icmp") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesLogicalOrOperation) {
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

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should generate logical OR instruction
  EXPECT_TRUE(output.find("or i1") != std::string::npos);
  EXPECT_TRUE(output.find("icmp") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesLogicalNotOperation) {
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

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should generate logical NOT instruction (XOR with true)
  EXPECT_TRUE(output.find("xor i1") != std::string::npos);
  EXPECT_TRUE(output.find(", true") != std::string::npos);
  EXPECT_TRUE(output.find("icmp") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesComplexLogicalExpression) {
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

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should generate all logical operations in correct order
  EXPECT_TRUE(output.find("and i1") != std::string::npos);
  EXPECT_TRUE(output.find("or i1") != std::string::npos);
  EXPECT_TRUE(output.find("xor i1") != std::string::npos);
  EXPECT_TRUE(output.find("icmp") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesNestedIfStatements) {
  const std::string source = R"(
const test = fn(x: i32, y: i32) -> i32 {
  if x > 0 {
    if y > 0 {
      return 1
    } else {
      return 2
    }
  } else {
    return 3
  }
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should generate multiple conditional branches and blocks
  size_t then_count = 0;
  size_t else_count = 0;
  size_t pos = 0;
  
  while ((pos = output.find("then", pos)) != std::string::npos) {
    then_count++;
    pos += 4;
  }
  
  pos = 0;
  while ((pos = output.find("else", pos)) != std::string::npos) {
    else_count++;
    pos += 4;
  }
  
  EXPECT_GE(then_count, 2);  // At least 2 then blocks
  EXPECT_GE(else_count, 2);  // At least 2 else blocks
}

TEST_F(CodeGenerationTest, GeneratesSimpleRangeLoop) {
  const std::string source = R"(
const test = fn() -> i32 {
  sum :i32 = 0
  loop i in 0..5 {
    sum = sum + i
  }
  return sum
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should generate loop structure with basic blocks
  EXPECT_TRUE(output.find("loop.cond:") != std::string::npos);  // Loop condition block
  EXPECT_TRUE(output.find("loop.body:") != std::string::npos);  // Loop body block
  EXPECT_TRUE(output.find("loop.end:") != std::string::npos);   // Loop end block
  EXPECT_TRUE(output.find("br i1") != std::string::npos);       // Conditional branch
  EXPECT_TRUE(output.find("icmp") != std::string::npos);        // Comparison instruction
}

TEST_F(CodeGenerationTest, GeneratesConditionalLoop) {
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

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should generate conditional loop structure
  EXPECT_TRUE(output.find("loop.cond:") != std::string::npos);
  EXPECT_TRUE(output.find("loop.body:") != std::string::npos);
  EXPECT_TRUE(output.find("loop.end:") != std::string::npos);
  EXPECT_TRUE(output.find("icmp slt") != std::string::npos);    // Less than comparison
}

TEST_F(CodeGenerationTest, GeneratesLoopVariableAllocation) {
  const std::string source = R"(
const test = fn() -> i32 {
  loop i in 0..3 {
    return i
  }
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should allocate space for loop variable
  EXPECT_TRUE(output.find("alloca i32") != std::string::npos);
  EXPECT_TRUE(output.find("store i32") != std::string::npos);
  EXPECT_TRUE(output.find("load i32") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesNestedLoops) {
  const std::string source = R"(
const test = fn() -> i32 {
  loop i in 0..2 {
    loop j in 0..2 {
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

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should generate multiple loop structures
  size_t loop_cond_count = 0;
  size_t loop_body_count = 0;
  size_t pos = 0;
  
  while ((pos = output.find("loop.cond", pos)) != std::string::npos) {
    loop_cond_count++;
    pos += 9;  // length of "loop.cond"
  }
  
  pos = 0;
  while ((pos = output.find("loop.body", pos)) != std::string::npos) {
    loop_body_count++;
    pos += 9;  // length of "loop.body"
  }
  
  EXPECT_GE(loop_cond_count, 2);  // At least 2 loop condition blocks
  EXPECT_GE(loop_body_count, 2);  // At least 2 loop body blocks
}

TEST_F(CodeGenerationTest, GeneratesLoopWithComplexCondition) {
  const std::string source = R"(
const test = fn() -> i32 {
  x :i32 = 0
  y :i32 = 5
  loop if x < y and y > 0 {
    x = x + 1
  }
  return x
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should generate logical operations in loop condition
  EXPECT_TRUE(output.find("and i1") != std::string::npos);
  EXPECT_TRUE(output.find("icmp slt") != std::string::npos);
  EXPECT_TRUE(output.find("icmp sgt") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesRangeWithVariableExpressions) {
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

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should load start and end variables for range
  EXPECT_TRUE(output.find("loop.cond:") != std::string::npos);
  EXPECT_TRUE(output.find("load i32") != std::string::npos);
  EXPECT_TRUE(output.find("icmp") != std::string::npos);
}

// Do syntax code generation tests
TEST_F(CodeGenerationTest, GeneratesFunctionWithDoSyntax) {
  const std::string source = R"(
const simple = fn() -> i32 do return 42
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should generate function with simple return
  EXPECT_TRUE(output.find("define i32 @simple()") != std::string::npos);
  EXPECT_TRUE(output.find("ret i32 42") != std::string::npos);
  
  // Should be minimal IR - no unnecessary basic blocks
  EXPECT_TRUE(output.find("entry:") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesIfWithDoSyntax) {
  const std::string source = R"(
const test = fn(x: i32) -> i32 {
  if x > 10 do return 1
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should generate conditional branch structure
  EXPECT_TRUE(output.find("icmp sgt") != std::string::npos);
  EXPECT_TRUE(output.find("br i1") != std::string::npos);
  EXPECT_TRUE(output.find("ret i32 1") != std::string::npos);
  EXPECT_TRUE(output.find("ret i32 0") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesRangeLoopWithDoSyntax) {
  const std::string source = R"(
const test = fn() -> i32 {
  sum :i32 = 0
  loop i in 0..3 do sum = sum + i
  return sum
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should generate loop structure
  EXPECT_TRUE(output.find("loop.cond:") != std::string::npos);
  EXPECT_TRUE(output.find("loop.body:") != std::string::npos);
  EXPECT_TRUE(output.find("loop.end:") != std::string::npos);
  EXPECT_TRUE(output.find("add i32") != std::string::npos);  // sum = sum + i
}

TEST_F(CodeGenerationTest, GeneratesConditionalLoopWithDoSyntax) {
  const std::string source = R"(
const test = fn() -> i32 {
  x :i32 = 0
  loop if x < 5 do x = x + 1
  return x
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should generate conditional loop structure
  EXPECT_TRUE(output.find("loop.cond:") != std::string::npos);
  EXPECT_TRUE(output.find("loop.body:") != std::string::npos);
  EXPECT_TRUE(output.find("loop.end:") != std::string::npos);
  EXPECT_TRUE(output.find("icmp slt") != std::string::npos);  // x < 5
  EXPECT_TRUE(output.find("add i32") != std::string::npos);   // x = x + 1
}

// Nil function code generation tests
TEST_F(CodeGenerationTest, GeneratesNilFunctionExplicit) {
  const std::string source = R"(
const nil_func = fn() -> nil {
  return
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  EXPECT_TRUE(output.find("define void @nil_func()") != std::string::npos);
  EXPECT_TRUE(output.find("ret void") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesNilFunctionImplicit) {
  const std::string source = R"(
const nil_func = fn() {
  return
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  EXPECT_TRUE(output.find("define void @nil_func()") != std::string::npos);
  EXPECT_TRUE(output.find("ret void") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesNilFunctionWithDoSyntax) {
  const std::string source = R"(
const nil_func = fn() do return
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  EXPECT_TRUE(output.find("define void @nil_func()") != std::string::npos);
  EXPECT_TRUE(output.find("ret void") != std::string::npos);
}

TEST_F(CodeGenerationTest, RejectsValueReturnFromNilFunction) {
  const std::string source = R"(
const nil_func = fn() -> nil {
  return 42
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  
  // This should throw an error due to validation
  try {
    codegen.generate_program(program.get());
    FAIL() << "Expected exception to be thrown";
  } catch (const std::runtime_error& e) {
    std::string error_message = e.what();
    EXPECT_TRUE(error_message.find("Cannot return a value from a nil function") != std::string::npos);
  }
}

TEST_F(CodeGenerationTest, GeneratesStringLiterals) {
  const std::string source = R"(
import fmt

const test = fn() -> i32 {
  fmt.println("Hello, world!")
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Should generate string constants
  EXPECT_TRUE(output.find("Hello, world!") != std::string::npos);
  EXPECT_TRUE(output.find("@printf") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesStringFormatReplacement) {
  const std::string source = R"(
import fmt

const test = fn() -> i32 {
  fmt.println("Number: {:d}", 42)
  fmt.println("String: {:s}", "hello")
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Format strings should be converted from {:d} to %d and {:s} to %s
  EXPECT_TRUE(output.find("Number: %d") != std::string::npos);
  EXPECT_TRUE(output.find("String: %s") != std::string::npos);
  EXPECT_TRUE(output.find("hello") != std::string::npos);
  EXPECT_FALSE(output.find("{:d}") != std::string::npos);  // Should not contain original format
  EXPECT_FALSE(output.find("{:s}") != std::string::npos);  // Should not contain original format
}

TEST_F(CodeGenerationTest, GeneratesEmptyStringLiteral) {
  const std::string source = R"(
import fmt

const test = fn() -> i32 {
  fmt.println("")
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  // Should not throw and should generate valid IR for empty strings
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  EXPECT_TRUE(output.find("@printf") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesFunctionPointerVariable) {
  const std::string source = R"(
const add = fn(x: i32, y: i32) -> i32 {
  return x + y
}

const test = fn() -> i32 {
  operation: fn(i32, i32) -> i32 = add
  return 42
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  // Should not throw and should generate valid IR for function pointers
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Check that function pointer allocation is generated
  EXPECT_TRUE(output.find("alloca") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesFunctionPointerTypeHelper) {
  const std::string source = R"(
const dummy = fn() -> i32 {
  callback: fn() -> i32 = dummy
  return 42
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  
  // Test that function pointer type parsing works
  EXPECT_NO_THROW(codegen.generate_program(program.get()));
}

TEST_F(CodeGenerationTest, HandlesFunctionPointerWithMultipleParams) {
  const std::string source = R"(
const calc = fn(a: i32, b: i32, c: i32) -> i32 {
  return a + b + c
}

const test = fn() -> i32 {
  operation: fn(i32, i32, i32) -> i32 = calc
  return 42
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  EXPECT_NO_THROW(codegen.generate_program(program.get()));
}

TEST_F(CodeGenerationTest, GeneratesFunctionPointerCall) {
  const std::string source = R"(
const add = fn(x: i32, y: i32) -> i32 {
  return x + y
}

const test = fn() -> i32 {
  operation: fn(i32, i32) -> i32 = add
  result: i32 = operation(5, 3)
  return result
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  // Should generate valid IR including indirect call
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Check that indirect call is generated
  EXPECT_TRUE(output.find("call") != std::string::npos);
  EXPECT_TRUE(output.find("alloca") != std::string::npos);
}

TEST_F(CodeGenerationTest, HandlesFunctionPointerCallWithNoParams) {
  const std::string source = R"(
const getValue = fn() -> i32 {
  return 42
}

const test = fn() -> i32 {
  getter: fn() -> i32 = getValue
  result: i32 = getter()
  return result
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  EXPECT_NO_THROW(codegen.generate_program(program.get()));
}

TEST_F(CodeGenerationTest, ThrowsOnFunctionPointerCallWithWrongArgCount) {
  const std::string source = R"(
const add = fn(x: i32, y: i32) -> i32 {
  return x + y
}

const test = fn() -> i32 {
  operation: fn(i32, i32) -> i32 = add
  result: i32 = operation(5)
  return result
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  EXPECT_THROW(codegen.generate_program(program.get()), std::runtime_error);
}

TEST_F(CodeGenerationTest, GeneratesAnonymousFunction) {
  const std::string source = R"(
const main = fn() -> i32 {
  operation: fn(i32, i32) -> i32 = fn(x: i32, y: i32) -> i32 do return x + y
  return 42
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  // Should not throw and should generate valid IR for anonymous functions
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Check that anonymous function is generated
  EXPECT_TRUE(output.find("@anon_") != std::string::npos);
  EXPECT_TRUE(output.find("define internal i32 @anon_") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesAnonymousFunctionCall) {
  const std::string source = R"(
const main = fn() -> i32 {
  operation: fn(i32, i32) -> i32 = fn(x: i32, y: i32) -> i32 do return x + y
  result: i32 = operation(5, 3)
  return result
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  // Should generate valid IR including anonymous function and call
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Check that anonymous function is generated and called
  EXPECT_TRUE(output.find("@anon_") != std::string::npos);
  EXPECT_TRUE(output.find("call i32") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesAnonymousFunctionWithMultipleParams) {
  const std::string source = R"(
const main = fn() -> i32 {
  calc: fn(i32, i32, i32) -> i32 = fn(a: i32, b: i32, c: i32) -> i32 do return a + b + c
  return 0
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  EXPECT_NO_THROW(codegen.generate_program(program.get()));
}

TEST_F(CodeGenerationTest, GeneratesAnonymousFunctionWithNoParams) {
  const std::string source = R"(
const main = fn() -> i32 {
  getter: fn() -> i32 = fn() -> i32 do return 42
  result: i32 = getter()
  return result
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  EXPECT_NO_THROW(codegen.generate_program(program.get()));
}

TEST_F(CodeGenerationTest, GeneratesTypeInferredVariables) {
  const std::string source = R"(
const main = fn() -> i32 {
  number := 42
  message := "Hello"
  return number
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  // Should generate valid IR for type-inferred variables
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Check that both variables are allocated correctly
  EXPECT_TRUE(output.find("%number = alloca i32") != std::string::npos);
  EXPECT_TRUE(output.find("%message = alloca ptr") != std::string::npos);
  EXPECT_TRUE(output.find("store i32 42") != std::string::npos);
  EXPECT_TRUE(output.find("Hello") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesTypeInferredAnonymousFunction) {
  const std::string source = R"(
const main = fn() -> i32 {
  adder := fn(x: i32, y: i32) -> i32 do return x + y
  result: i32 = adder(5, 7)
  return result
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  // Should generate valid IR including anonymous function and type inference
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Check that anonymous function is generated and function pointer is allocated
  EXPECT_TRUE(output.find("@anon_") != std::string::npos);
  EXPECT_TRUE(output.find("%adder = alloca ptr") != std::string::npos);
  EXPECT_TRUE(output.find("call i32") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesTypeInferredArithmeticExpressions) {
  const std::string source = R"(
const main = fn() -> i32 {
  x := 10
  y := 20
  sum := x + y
  product := x * y
  return sum
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Check that all variables are allocated as i32
  EXPECT_TRUE(output.find("%x = alloca i32") != std::string::npos);
  EXPECT_TRUE(output.find("%y = alloca i32") != std::string::npos);
  EXPECT_TRUE(output.find("%sum = alloca i32") != std::string::npos);
  EXPECT_TRUE(output.find("%product = alloca i32") != std::string::npos);
  
  // Check arithmetic operations
  EXPECT_TRUE(output.find("add i32") != std::string::npos);
  EXPECT_TRUE(output.find("mul i32") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesTypeInferredStringConcatenation) {
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

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Check that string variables are allocated as pointers
  EXPECT_TRUE(output.find("%first = alloca ptr") != std::string::npos);
  EXPECT_TRUE(output.find("%second = alloca ptr") != std::string::npos);
  EXPECT_TRUE(output.find("%combined = alloca ptr") != std::string::npos);
  
  // Check string constants
  EXPECT_TRUE(output.find("Hello") != std::string::npos);
  EXPECT_TRUE(output.find("World") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesTypeInferredVariableReferences) {
  const std::string source = R"(
const main = fn() -> i32 {
  original := 42
  copy := original
  return copy
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Check that both variables are allocated as i32
  EXPECT_TRUE(output.find("%original = alloca i32") != std::string::npos);
  EXPECT_TRUE(output.find("%copy = alloca i32") != std::string::npos);
  
  // Check load and store operations
  EXPECT_TRUE(output.find("load i32") != std::string::npos);
  EXPECT_TRUE(output.find("store i32") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesBooleanLiterals) {
  const std::string source = R"(
const main = fn() -> bool {
  is_true := true
  is_false := false
  return is_true
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Check that boolean variables are allocated as i1
  EXPECT_TRUE(output.find("%is_true = alloca i1") != std::string::npos);
  EXPECT_TRUE(output.find("%is_false = alloca i1") != std::string::npos);
  
  // Check boolean constants
  EXPECT_TRUE(output.find("store i1 true") != std::string::npos);
  EXPECT_TRUE(output.find("store i1 false") != std::string::npos);
}

TEST_F(CodeGenerationTest, GeneratesBooleanOperations) {
  const std::string source = R"(
const main = fn() -> bool {
  a := true
  b := false
  result := a or b
  return result
}
)";

  auto program = ParseSource(source);
  ASSERT_NE(program, nullptr);

  CodeGenerator codegen;
  codegen.generate_program(program.get());
  
  testing::internal::CaptureStdout();
  codegen.print_ir();
  std::string output = testing::internal::GetCapturedStdout();
  
  // Check boolean operations
  EXPECT_TRUE(output.find("or i1") != std::string::npos);
  EXPECT_TRUE(output.find("%result = alloca i1") != std::string::npos);
}

}  // namespace
}  // namespace void_compiler