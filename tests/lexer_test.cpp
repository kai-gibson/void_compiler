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

}  // namespace
}  // namespace void_compiler