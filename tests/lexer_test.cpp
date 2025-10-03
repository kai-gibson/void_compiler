#include <gtest/gtest.h>

#include "lexer.h"

namespace void_compiler {
namespace {

class LexerTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}

  std::vector<Token> TokenizeSource(const std::string& source) {
    Lexer lexer(source);
    std::vector<Token> tokens;
    Token token;
    do {
      token = lexer.next_token();
      tokens.push_back(token);
    } while (token.type != TokenType::EndOfFile);
    return tokens;
  }
};

TEST_F(LexerTest, TokenizesNumbers) {
  auto tokens = TokenizeSource("42 123 0");
  
  ASSERT_EQ(tokens.size(), 4);  // 3 numbers + EOF
  EXPECT_EQ(tokens[0].type, TokenType::Number);
  EXPECT_EQ(tokens[0].value, "42");
  EXPECT_EQ(tokens[1].type, TokenType::Number);
  EXPECT_EQ(tokens[1].value, "123");
  EXPECT_EQ(tokens[2].type, TokenType::Number);
  EXPECT_EQ(tokens[2].value, "0");
  EXPECT_EQ(tokens[3].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, TokenizesKeywords) {
  auto tokens = TokenizeSource("const fn return i32");
  
  ASSERT_EQ(tokens.size(), 5);  // 4 keywords + EOF
  EXPECT_EQ(tokens[0].type, TokenType::Const);
  EXPECT_EQ(tokens[0].value, "const");
  EXPECT_EQ(tokens[1].type, TokenType::Fn);
  EXPECT_EQ(tokens[1].value, "fn");
  EXPECT_EQ(tokens[2].type, TokenType::Return);
  EXPECT_EQ(tokens[2].value, "return");
  EXPECT_EQ(tokens[3].type, TokenType::I32);
  EXPECT_EQ(tokens[3].value, "i32");
  EXPECT_EQ(tokens[4].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, TokenizesIdentifiers) {
  auto tokens = TokenizeSource("variable_name another_var");
  
  ASSERT_EQ(tokens.size(), 3);  // 2 identifiers + EOF
  EXPECT_EQ(tokens[0].type, TokenType::Identifier);
  EXPECT_EQ(tokens[0].value, "variable_name");
  EXPECT_EQ(tokens[1].type, TokenType::Identifier);
  EXPECT_EQ(tokens[1].value, "another_var");
  EXPECT_EQ(tokens[2].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, TokenizesOperators) {
  auto tokens = TokenizeSource("+ - * / = -> , :");
  
  ASSERT_EQ(tokens.size(), 9);  // 8 operators + EOF
  EXPECT_EQ(tokens[0].type, TokenType::Plus);
  EXPECT_EQ(tokens[1].type, TokenType::Minus);
  EXPECT_EQ(tokens[2].type, TokenType::Multiply);
  EXPECT_EQ(tokens[3].type, TokenType::Divide);
  EXPECT_EQ(tokens[4].type, TokenType::Equals);
  EXPECT_EQ(tokens[5].type, TokenType::Arrow);
  EXPECT_EQ(tokens[6].type, TokenType::Comma);
  EXPECT_EQ(tokens[7].type, TokenType::Colon);
  EXPECT_EQ(tokens[8].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, TokenizesDelimiters) {
  auto tokens = TokenizeSource("( ) { }");
  
  ASSERT_EQ(tokens.size(), 5);  // 4 delimiters + EOF
  EXPECT_EQ(tokens[0].type, TokenType::LParen);
  EXPECT_EQ(tokens[1].type, TokenType::RParen);
  EXPECT_EQ(tokens[2].type, TokenType::LBrace);
  EXPECT_EQ(tokens[3].type, TokenType::RBrace);
  EXPECT_EQ(tokens[4].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, TokenizesSimpleFunction) {
  const std::string source = R"(
const add = fn(x: i32, y: i32) -> i32 {
  return x + y
}
)";
  
  auto tokens = TokenizeSource(source);
  
  // Verify key tokens (not exhaustive, but ensures basic structure)
  EXPECT_EQ(tokens[0].type, TokenType::Const);
  EXPECT_EQ(tokens[1].type, TokenType::Identifier);
  EXPECT_EQ(tokens[1].value, "add");
  EXPECT_EQ(tokens[2].type, TokenType::Equals);
  EXPECT_EQ(tokens[3].type, TokenType::Fn);
  EXPECT_EQ(tokens[4].type, TokenType::LParen);
  // ... more specific checks could go here
  EXPECT_EQ(tokens.back().type, TokenType::EndOfFile);
}

TEST_F(LexerTest, SkipsWhitespace) {
  auto tokens = TokenizeSource("  \t\n  const  \n\t  fn  ");
  
  ASSERT_EQ(tokens.size(), 3);  // 2 keywords + EOF
  EXPECT_EQ(tokens[0].type, TokenType::Const);
  EXPECT_EQ(tokens[1].type, TokenType::Fn);
  EXPECT_EQ(tokens[2].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, HandlesArrowOperator) {
  auto tokens = TokenizeSource("- -> ->");
  
  ASSERT_EQ(tokens.size(), 4);  // minus, arrow, arrow, EOF
  EXPECT_EQ(tokens[0].type, TokenType::Minus);
  EXPECT_EQ(tokens[1].type, TokenType::Arrow);
  EXPECT_EQ(tokens[2].type, TokenType::Arrow);
  EXPECT_EQ(tokens[3].type, TokenType::EndOfFile);
}

// Error handling tests
TEST_F(LexerTest, ThrowsOnInvalidCharacter) {
  EXPECT_THROW({
    try {
      TokenizeSource("const x = @");
    } catch (const std::runtime_error& e) {
      EXPECT_STREQ("Unknown character: @", e.what());
      throw;
    }
  }, std::runtime_error);
}

TEST_F(LexerTest, ThrowsOnInvalidSymbol) {
  EXPECT_THROW({
    TokenizeSource("const x = #");
  }, std::runtime_error);
}

TEST_F(LexerTest, HandlesSingleCharacterTokens) {
  auto tokens = TokenizeSource("(){}");
  
  ASSERT_EQ(tokens.size(), 5);  // 4 tokens + EOF
  EXPECT_EQ(tokens[0].type, TokenType::LParen);
  EXPECT_EQ(tokens[1].type, TokenType::RParen);
  EXPECT_EQ(tokens[2].type, TokenType::LBrace);
  EXPECT_EQ(tokens[3].type, TokenType::RBrace);
  EXPECT_EQ(tokens[4].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, HandlesLineAndColumnNumbers) {
  Lexer lexer("const\nfn");
  auto token1 = lexer.next_token();
  auto token2 = lexer.next_token();
  
  EXPECT_EQ(token1.line, 1);
  EXPECT_EQ(token1.column, 6);  // Column after reading "const"
  EXPECT_EQ(token2.line, 2);
  EXPECT_EQ(token2.column, 3);  // Column after reading "fn"
}

TEST_F(LexerTest, HandlesEmptyInput) {
  auto tokens = TokenizeSource("");
  
  ASSERT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, HandlesOnlyWhitespace) {
  auto tokens = TokenizeSource("   \t\n\r  ");
  
  ASSERT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, HandlesNumbersAtEndOfInput) {
  auto tokens = TokenizeSource("42");
  
  ASSERT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0].type, TokenType::Number);
  EXPECT_EQ(tokens[0].value, "42");
  EXPECT_EQ(tokens[1].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, HandlesIdentifierAtEndOfInput) {
  auto tokens = TokenizeSource("variable");
  
  ASSERT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0].type, TokenType::Identifier);
  EXPECT_EQ(tokens[0].value, "variable");
  EXPECT_EQ(tokens[1].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, HandlesUnderscoreInIdentifiers) {
  auto tokens = TokenizeSource("my_var _test test_ _");
  ASSERT_EQ(tokens.size(), 5);  // 4 identifiers + EOF
  EXPECT_EQ(tokens[0].type, TokenType::Identifier);
  EXPECT_EQ(tokens[0].value, "my_var");
  EXPECT_EQ(tokens[1].type, TokenType::Identifier);
  EXPECT_EQ(tokens[1].value, "_test");
  EXPECT_EQ(tokens[2].type, TokenType::Identifier);
  EXPECT_EQ(tokens[2].value, "test_");
  EXPECT_EQ(tokens[3].type, TokenType::Identifier);
  EXPECT_EQ(tokens[3].value, "_");
}

TEST_F(LexerTest, HandlesMultipleConsecutiveOperators) {
  auto tokens = TokenizeSource("+-*/()");
  ASSERT_EQ(tokens.size(), 7);  // 5 operators + RParen + EOF
  EXPECT_EQ(tokens[0].type, TokenType::Plus);
  EXPECT_EQ(tokens[1].type, TokenType::Minus);
  EXPECT_EQ(tokens[2].type, TokenType::Multiply);
  EXPECT_EQ(tokens[3].type, TokenType::Divide);
  EXPECT_EQ(tokens[4].type, TokenType::LParen);
  EXPECT_EQ(tokens[5].type, TokenType::RParen);
  EXPECT_EQ(tokens[6].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, HandlesNumbersWithWhitespace) {
  auto tokens = TokenizeSource("123 456\t789\n000");
  ASSERT_EQ(tokens.size(), 5);  // 4 numbers + EOF
  EXPECT_EQ(tokens[0].value, "123");
  EXPECT_EQ(tokens[1].value, "456");
  EXPECT_EQ(tokens[2].value, "789");
  EXPECT_EQ(tokens[3].value, "000");
}

TEST_F(LexerTest, HandlesAllKeywordCombinations) {
  auto tokens = TokenizeSource("const fn return i32 const_fn fn_const return_type i32_value");
  EXPECT_EQ(tokens[0].type, TokenType::Const);
  EXPECT_EQ(tokens[1].type, TokenType::Fn);
  EXPECT_EQ(tokens[2].type, TokenType::Return);
  EXPECT_EQ(tokens[3].type, TokenType::I32);
  EXPECT_EQ(tokens[4].type, TokenType::Identifier);  // const_fn
  EXPECT_EQ(tokens[5].type, TokenType::Identifier);  // fn_const
  EXPECT_EQ(tokens[6].type, TokenType::Identifier);  // return_type
  EXPECT_EQ(tokens[7].type, TokenType::Identifier);  // i32_value
}

TEST_F(LexerTest, HandlesComplexWhitespacePatterns) {
  auto tokens = TokenizeSource("\n\t\r   const\n\n\tfn\r\r   ");
  ASSERT_EQ(tokens.size(), 3);  // const, fn, EOF
  EXPECT_EQ(tokens[0].type, TokenType::Const);
  EXPECT_EQ(tokens[1].type, TokenType::Fn);
  EXPECT_EQ(tokens[2].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, HandlesZeroAndLargeNumbers) {
  auto tokens = TokenizeSource("0 00 123456789 987654321");
  ASSERT_EQ(tokens.size(), 5);  // 4 numbers + EOF
  EXPECT_EQ(tokens[0].value, "0");
  EXPECT_EQ(tokens[1].value, "00");
  EXPECT_EQ(tokens[2].value, "123456789");
  EXPECT_EQ(tokens[3].value, "987654321");
}

TEST_F(LexerTest, HandlesAlphanumericIdentifiers) {
  auto tokens = TokenizeSource("var1 test2var func123 a1b2c3");
  ASSERT_EQ(tokens.size(), 5);  // 4 identifiers + EOF
  EXPECT_EQ(tokens[0].value, "var1");
  EXPECT_EQ(tokens[1].value, "test2var");
  EXPECT_EQ(tokens[2].value, "func123");
  EXPECT_EQ(tokens[3].value, "a1b2c3");
}

TEST_F(LexerTest, HandlesEdgeCaseTokenSequences) {
  auto tokens = TokenizeSource("123abc");  // number followed by identifier
  ASSERT_EQ(tokens.size(), 3);  // number, identifier, EOF
  EXPECT_EQ(tokens[0].type, TokenType::Number);
  EXPECT_EQ(tokens[0].value, "123");
  EXPECT_EQ(tokens[1].type, TokenType::Identifier);
  EXPECT_EQ(tokens[1].value, "abc");
}

TEST_F(LexerTest, HandlesMinusArrowDistinction) {
  auto tokens = TokenizeSource("- -> -->");
  ASSERT_EQ(tokens.size(), 5);  // minus, arrow, minus, arrow, EOF
  EXPECT_EQ(tokens[0].type, TokenType::Minus);
  EXPECT_EQ(tokens[1].type, TokenType::Arrow);
  EXPECT_EQ(tokens[2].type, TokenType::Minus);
  EXPECT_EQ(tokens[3].type, TokenType::Arrow);
}

TEST_F(LexerTest, HandlesCarriageReturnAndMixedLineEndings) {
  auto tokens = TokenizeSource("const\r\nfn\n\rreturn");
  ASSERT_EQ(tokens.size(), 4);  // const, fn, return, EOF
  EXPECT_EQ(tokens[0].type, TokenType::Const);
  EXPECT_EQ(tokens[1].type, TokenType::Fn);
  EXPECT_EQ(tokens[2].type, TokenType::Return);
}

TEST_F(LexerTest, TokenizesControlFlowKeywords) {
  auto tokens = TokenizeSource("if else and or not");
  
  ASSERT_EQ(tokens.size(), 6);  // 5 keywords + EOF
  EXPECT_EQ(tokens[0].type, TokenType::If);
  EXPECT_EQ(tokens[0].value, "if");
  EXPECT_EQ(tokens[1].type, TokenType::Else);
  EXPECT_EQ(tokens[1].value, "else");
  EXPECT_EQ(tokens[2].type, TokenType::And);
  EXPECT_EQ(tokens[2].value, "and");
  EXPECT_EQ(tokens[3].type, TokenType::Or);
  EXPECT_EQ(tokens[3].value, "or");
  EXPECT_EQ(tokens[4].type, TokenType::Not);
  EXPECT_EQ(tokens[4].value, "not");
  EXPECT_EQ(tokens[5].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, TokenizesComparisonOperators) {
  auto tokens = TokenizeSource("> < >= <= == !=");
  
  ASSERT_EQ(tokens.size(), 7);  // 6 operators + EOF
  EXPECT_EQ(tokens[0].type, TokenType::GreaterThan);
  EXPECT_EQ(tokens[0].value, ">");
  EXPECT_EQ(tokens[1].type, TokenType::LessThan);
  EXPECT_EQ(tokens[1].value, "<");
  EXPECT_EQ(tokens[2].type, TokenType::GreaterEqual);
  EXPECT_EQ(tokens[2].value, ">=");
  EXPECT_EQ(tokens[3].type, TokenType::LessEqual);
  EXPECT_EQ(tokens[3].value, "<=");
  EXPECT_EQ(tokens[4].type, TokenType::EqualEqual);
  EXPECT_EQ(tokens[4].value, "==");
  EXPECT_EQ(tokens[5].type, TokenType::NotEqual);
  EXPECT_EQ(tokens[5].value, "!=");
  EXPECT_EQ(tokens[6].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, HandlesComparisonOperatorEdgeCases) {
  auto tokens = TokenizeSource("=== !== >>= <<=");
  
  ASSERT_EQ(tokens.size(), 9);  // ==, =, !=, =, >, >=, <, <=, EOF
  EXPECT_EQ(tokens[0].type, TokenType::EqualEqual);
  EXPECT_EQ(tokens[1].type, TokenType::Equals);
  EXPECT_EQ(tokens[2].type, TokenType::NotEqual);
  EXPECT_EQ(tokens[3].type, TokenType::Equals);
  EXPECT_EQ(tokens[4].type, TokenType::GreaterThan);
  EXPECT_EQ(tokens[5].type, TokenType::GreaterEqual);
  EXPECT_EQ(tokens[6].type, TokenType::LessThan);
  EXPECT_EQ(tokens[7].type, TokenType::LessEqual);
}

TEST_F(LexerTest, TokenizesComplexControlFlowExpression) {
  auto tokens = TokenizeSource("if x > 10 and y <= 20 or not z == 5");
  
  // Verify key tokens in the expression
  EXPECT_EQ(tokens[0].type, TokenType::If);
  EXPECT_EQ(tokens[1].type, TokenType::Identifier);
  EXPECT_EQ(tokens[1].value, "x");
  EXPECT_EQ(tokens[2].type, TokenType::GreaterThan);
  EXPECT_EQ(tokens[3].type, TokenType::Number);
  EXPECT_EQ(tokens[3].value, "10");
  EXPECT_EQ(tokens[4].type, TokenType::And);
  EXPECT_EQ(tokens[5].type, TokenType::Identifier);
  EXPECT_EQ(tokens[5].value, "y");
  EXPECT_EQ(tokens[6].type, TokenType::LessEqual);
  EXPECT_EQ(tokens[7].type, TokenType::Number);
  EXPECT_EQ(tokens[7].value, "20");
  EXPECT_EQ(tokens[8].type, TokenType::Or);
  EXPECT_EQ(tokens[9].type, TokenType::Not);
  EXPECT_EQ(tokens[10].type, TokenType::Identifier);
  EXPECT_EQ(tokens[10].value, "z");
  EXPECT_EQ(tokens[11].type, TokenType::EqualEqual);
  EXPECT_EQ(tokens[12].type, TokenType::Number);
  EXPECT_EQ(tokens[12].value, "5");
}

TEST_F(LexerTest, TokenizesLoopKeywords) {
  auto tokens = TokenizeSource("loop in");
  
  ASSERT_EQ(tokens.size(), 3);  // loop, in, EOF
  EXPECT_EQ(tokens[0].type, TokenType::Loop);
  EXPECT_EQ(tokens[0].value, "loop");
  EXPECT_EQ(tokens[1].type, TokenType::In);
  EXPECT_EQ(tokens[1].value, "in");
  EXPECT_EQ(tokens[2].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, TokenizesRangeOperator) {
  auto tokens = TokenizeSource("0..10");
  
  ASSERT_EQ(tokens.size(), 4);  // 0, .., 10, EOF
  EXPECT_EQ(tokens[0].type, TokenType::Number);
  EXPECT_EQ(tokens[0].value, "0");
  EXPECT_EQ(tokens[1].type, TokenType::DotDot);
  EXPECT_EQ(tokens[1].value, "..");
  EXPECT_EQ(tokens[2].type, TokenType::Number);
  EXPECT_EQ(tokens[2].value, "10");
  EXPECT_EQ(tokens[3].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, TokenizesRangeLoopExpression) {
  auto tokens = TokenizeSource("loop i in 0..10");
  
  ASSERT_EQ(tokens.size(), 7);  // loop, i, in, 0, .., 10, EOF
  EXPECT_EQ(tokens[0].type, TokenType::Loop);
  EXPECT_EQ(tokens[1].type, TokenType::Identifier);
  EXPECT_EQ(tokens[1].value, "i");
  EXPECT_EQ(tokens[2].type, TokenType::In);
  EXPECT_EQ(tokens[3].type, TokenType::Number);
  EXPECT_EQ(tokens[3].value, "0");
  EXPECT_EQ(tokens[4].type, TokenType::DotDot);
  EXPECT_EQ(tokens[5].type, TokenType::Number);
  EXPECT_EQ(tokens[5].value, "10");
  EXPECT_EQ(tokens[6].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, TokenizesConditionalLoopExpression) {
  auto tokens = TokenizeSource("loop if x < 10");
  
  ASSERT_EQ(tokens.size(), 6);  // loop, if, x, <, 10, EOF
  EXPECT_EQ(tokens[0].type, TokenType::Loop);
  EXPECT_EQ(tokens[1].type, TokenType::If);
  EXPECT_EQ(tokens[2].type, TokenType::Identifier);
  EXPECT_EQ(tokens[2].value, "x");
  EXPECT_EQ(tokens[3].type, TokenType::LessThan);
  EXPECT_EQ(tokens[4].type, TokenType::Number);
  EXPECT_EQ(tokens[4].value, "10");
  EXPECT_EQ(tokens[5].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, HandlesDotVsDotDotDistinction) {
  auto tokens = TokenizeSource(". .. .");
  
  ASSERT_EQ(tokens.size(), 4);  // ., .., ., EOF  
  EXPECT_EQ(tokens[0].type, TokenType::Dot);
  EXPECT_EQ(tokens[0].value, ".");
  EXPECT_EQ(tokens[1].type, TokenType::DotDot);
  EXPECT_EQ(tokens[1].value, "..");
  EXPECT_EQ(tokens[2].type, TokenType::Dot);
  EXPECT_EQ(tokens[2].value, ".");
  EXPECT_EQ(tokens[3].type, TokenType::EndOfFile);
}

// Do keyword tests
TEST_F(LexerTest, TokenizesDoKeyword) {
  auto tokens = TokenizeSource("do");
  
  ASSERT_EQ(tokens.size(), 2);  // do, EOF
  EXPECT_EQ(tokens[0].type, TokenType::Do);
  EXPECT_EQ(tokens[0].value, "do");
  EXPECT_EQ(tokens[1].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, TokenizesDoWithOtherKeywords) {
  auto tokens = TokenizeSource("if condition do return");
  
  ASSERT_EQ(tokens.size(), 5);  // if, condition, do, return, EOF
  EXPECT_EQ(tokens[0].type, TokenType::If);
  EXPECT_EQ(tokens[1].type, TokenType::Identifier);
  EXPECT_EQ(tokens[1].value, "condition");
  EXPECT_EQ(tokens[2].type, TokenType::Do);
  EXPECT_EQ(tokens[2].value, "do");
  EXPECT_EQ(tokens[3].type, TokenType::Return);
  EXPECT_EQ(tokens[4].type, TokenType::EndOfFile);
}

TEST_F(LexerTest, TokenizesFunctionDoSyntax) {
  auto tokens = TokenizeSource("fn() -> i32 do return 42");
  
  ASSERT_EQ(tokens.size(), 9);  // fn, (, ), ->, i32, do, return, 42, EOF
  EXPECT_EQ(tokens[0].type, TokenType::Fn);
  EXPECT_EQ(tokens[1].type, TokenType::LParen);
  EXPECT_EQ(tokens[2].type, TokenType::RParen);
  EXPECT_EQ(tokens[3].type, TokenType::Arrow);
  EXPECT_EQ(tokens[4].type, TokenType::I32);
  EXPECT_EQ(tokens[5].type, TokenType::Do);
  EXPECT_EQ(tokens[5].value, "do");
  EXPECT_EQ(tokens[6].type, TokenType::Return);
  EXPECT_EQ(tokens[7].type, TokenType::Number);
  EXPECT_EQ(tokens[7].value, "42");
  EXPECT_EQ(tokens[8].type, TokenType::EndOfFile);
}

}  // namespace
}  // namespace void_compiler