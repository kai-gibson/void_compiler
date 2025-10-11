#include "runtime.h"
#include "lexer.h"
#include "gtest/gtest.h"
#include <memory>

namespace void_compiler {

// Test bounds checking function (existing runtime support)
TEST(SliceTests, BoundsCheckValid) {
    EXPECT_NO_THROW(bounds_check(0, 5));
    EXPECT_NO_THROW(bounds_check(4, 5));
    EXPECT_NO_THROW(bounds_check(0, 1));
}

TEST(SliceTests, BoundsCheckInvalid) {
    EXPECT_THROW(bounds_check(-1, 5), std::out_of_range);
    EXPECT_THROW(bounds_check(5, 5), std::out_of_range);
    EXPECT_THROW(bounds_check(1, 1), std::out_of_range);
    EXPECT_THROW(bounds_check(-5, 10), std::out_of_range);
    EXPECT_THROW(bounds_check(10, 10), std::out_of_range);
}

// Test lexer tokenizes slice syntax
TEST(SliceTests, LexerTokenizesSliceSyntax) {
    Lexer lexer("[]");
    Token token = lexer.next_token();
    EXPECT_EQ(token.type, TokenType::Slice);
    EXPECT_EQ(token.value, "[]");
}

TEST(SliceTests, LexerTokenizesLBracketRBracket) {
    // Individual bracket tokens should work too
    Lexer lexer("[ ]");
    Token token1 = lexer.next_token();
    Token token2 = lexer.next_token();
    
    EXPECT_EQ(token1.type, TokenType::LBracket);
    EXPECT_EQ(token1.value, "[");
    EXPECT_EQ(token2.type, TokenType::RBracket);
    EXPECT_EQ(token2.value, "]");
}

// Test lexer handles slice syntax in context
TEST(SliceTests, LexerTokenizesSliceInContext) {
    Lexer lexer("array[:]");
    
    Token token1 = lexer.next_token();
    EXPECT_EQ(token1.type, TokenType::Identifier);
    EXPECT_EQ(token1.value, "array");
    
    Token token2 = lexer.next_token();
    EXPECT_EQ(token2.type, TokenType::LBracket);
    
    Token token3 = lexer.next_token();
    EXPECT_EQ(token3.type, TokenType::Colon);
    
    Token token4 = lexer.next_token();
    EXPECT_EQ(token4.type, TokenType::RBracket);
}

// Test SliceType class
TEST(SliceTests, SliceTypeCreation) {
    SliceType int_slice("i32");
    EXPECT_EQ(int_slice.element_type(), "i32");
    EXPECT_EQ(int_slice.to_string(), "[]i32");
    
    SliceType string_slice("string");
    EXPECT_EQ(string_slice.element_type(), "string");
    EXPECT_EQ(string_slice.to_string(), "[]string");
}

// Test SliceExpression creation
TEST(SliceTests, SliceExpressionCreation) {
    auto base = std::make_unique<NumberLiteral>(42);
    auto slice_expr = std::make_unique<SliceExpression>(std::move(base));
    
    ASSERT_NE(slice_expr, nullptr);
    ASSERT_NE(slice_expr->base(), nullptr);
    
    // Verify the base is a NumberLiteral
    auto number_base = dynamic_cast<const NumberLiteral*>(slice_expr->base());
    EXPECT_NE(number_base, nullptr);
}

// Integration test - full lexing and parsing workflow
TEST(SliceTests, FullSliceTokenizationWorkflow) {
    std::string source = "variable[:]";
    
    Lexer lexer(source);
    std::vector<Token> tokens;
    Token token;
    do {
        token = lexer.next_token();
        tokens.push_back(token);
    } while (token.type != TokenType::EndOfFile);
    
    // Verify we get the expected tokens
    ASSERT_EQ(tokens.size(), 5); // identifier, [, :, ], EOF
    EXPECT_EQ(tokens[0].type, TokenType::Identifier);
    EXPECT_EQ(tokens[0].value, "variable");
    EXPECT_EQ(tokens[1].type, TokenType::LBracket);
    EXPECT_EQ(tokens[2].type, TokenType::Colon);
    EXPECT_EQ(tokens[3].type, TokenType::RBracket);
    EXPECT_EQ(tokens[4].type, TokenType::EndOfFile);
}

// Test complex slice tokenization
TEST(SliceTests, ComplexSliceTokenization) {
    // Test function_call()[:]
    std::string source = "get_array()[:]";
    
    Lexer lexer(source);
    std::vector<Token> tokens;
    Token token;
    do {
        token = lexer.next_token();
        tokens.push_back(token);
    } while (token.type != TokenType::EndOfFile);
    
    // Verify token sequence: identifier, (, ), [, :, ], EOF
    ASSERT_EQ(tokens.size(), 7);
    EXPECT_EQ(tokens[0].type, TokenType::Identifier);
    EXPECT_EQ(tokens[0].value, "get_array");
    EXPECT_EQ(tokens[1].type, TokenType::LParen);
    EXPECT_EQ(tokens[2].type, TokenType::RParen);
    EXPECT_EQ(tokens[3].type, TokenType::LBracket);
    EXPECT_EQ(tokens[4].type, TokenType::Colon);
    EXPECT_EQ(tokens[5].type, TokenType::RBracket);
    EXPECT_EQ(tokens[6].type, TokenType::EndOfFile);
}

// Test that slice syntax is properly tokenized in different contexts
TEST(SliceTests, SliceSyntaxInVariousContexts) {
    // Test: []i32
    Lexer lexer1("[]i32");
    Token t1 = lexer1.next_token();
    Token t2 = lexer1.next_token();
    EXPECT_EQ(t1.type, TokenType::Slice);
    EXPECT_EQ(t2.type, TokenType::I32);
    
    // Test: array[0:]
    Lexer lexer2("array[0:]");
    std::vector<Token> tokens2;
    Token token;
    do {
        token = lexer2.next_token();
        tokens2.push_back(token);
    } while (token.type != TokenType::EndOfFile);
    
    EXPECT_EQ(tokens2[0].type, TokenType::Identifier); // array
    EXPECT_EQ(tokens2[1].type, TokenType::LBracket);   // [
    EXPECT_EQ(tokens2[2].type, TokenType::Number);     // 0
    EXPECT_EQ(tokens2[3].type, TokenType::Colon);      // :
    EXPECT_EQ(tokens2[4].type, TokenType::RBracket);   // ]
}

// Test slice-related error conditions in lexer
TEST(SliceTests, LexerSliceErrorHandling) {
    // Test incomplete slice syntax [
    Lexer lexer1("[");
    Token t1 = lexer1.next_token();
    EXPECT_EQ(t1.type, TokenType::LBracket);
    
    // The lexer should handle individual brackets correctly
    // Error handling for incomplete syntax happens at parser level
}

}  // namespace void_compiler