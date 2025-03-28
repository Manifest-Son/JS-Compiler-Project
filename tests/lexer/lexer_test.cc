#include "gtest/gtest.h"
#include "../../include/lexer.h"
#include "../../include/token.h"

TEST(LexerTest, TokenizeKeywords) {
    Lexer lexer("let if else function return");
    std::vector<Token> tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 6);
    EXPECT_EQ(tokens[0].type, TokenType::KEYWORD);
    EXPECT_EQ(tokens[1].type, TokenType::KEYWORD);
    EXPECT_EQ(tokens[2].type, TokenType::KEYWORD);
    EXPECT_EQ(tokens[3].type, TokenType::KEYWORD);
    EXPECT_EQ(tokens[4].type, TokenType::KEYWORD);
    EXPECT_EQ(tokens[5].type, TokenType::END_OF_FILE);
}

TEST(LexerTest, TokenizeIdentifiers) {
    Lexer lexer("variable_name anotherVariable");
    std::vector<Token> tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[2].type, TokenType::END_OF_FILE);
}

TEST(LexerTest, TokenizeNumbers) {
    Lexer lexer("123 456.789");
    std::vector<Token> tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::NUMBER);
    EXPECT_EQ(tokens[1].type, TokenType::NUMBER);
    EXPECT_EQ(tokens[2].type, TokenType::END_OF_FILE);
}

TEST(LexerTest, TokenizeStrings) {
    Lexer lexer("\"hello\" \"world\"");
    std::vector<Token> tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::STRING);
    EXPECT_EQ(tokens[1].type, TokenType::STRING);
    EXPECT_EQ(tokens[2].type, TokenType::END_OF_FILE);
}

TEST(LexerTest, TokenizeSymbols) {
    Lexer lexer("(){}[]");
    std::vector<Token> tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 7);
    EXPECT_EQ(tokens[0].type, TokenType::SYMBOL);
    EXPECT_EQ(tokens[1].type, TokenType::SYMBOL);
    EXPECT_EQ(tokens[2].type, TokenType::SYMBOL);
    EXPECT_EQ(tokens[3].type, TokenType::SYMBOL);
    EXPECT_EQ(tokens[4].type, TokenType::SYMBOL);
    EXPECT_EQ(tokens[5].type, TokenType::SYMBOL);
    EXPECT_EQ(tokens[6].type, TokenType::END_OF_FILE);
}

TEST(LexerTest, TokenizeOperators) {
    Lexer lexer("== <= >=");
    std::vector<Token> tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::OPERATOR);
    EXPECT_EQ(tokens[1].type, TokenType::OPERATOR);
    EXPECT_EQ(tokens[2].type, TokenType::OPERATOR);
    EXPECT_EQ(tokens[3].type, TokenType::END_OF_FILE);
}

TEST(LexerTest, TokenizeUnknown) {
    Lexer lexer("@");
    std::vector<Token> tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, TokenType::ERROR);
    EXPECT_EQ(tokens[1].type, TokenType::END_OF_FILE);
}

TEST(LexerTest, TokenizeComments) {
    Lexer lexer("// This is a comment\nlet x = 5; /* This is a\nmulti-line comment */");
    std::vector<Token> tokens = lexer.tokenize();
    
    ASSERT_GT(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type, TokenType::COMMENT);
    EXPECT_EQ(tokens[0].lexeme, " This is a comment");
    
    // Find the multi-line comment
    bool foundMultilineComment = false;
    for (const auto& token : tokens) {
        if (token.type == TokenType::COMMENT && token.lexeme.find("This is a\nmulti-line comment") != std::string::npos) {
            foundMultilineComment = true;
            break;
        }
    }
    EXPECT_TRUE(foundMultilineComment);
}

TEST(LexerTest, TokenizeComplexOperators) {
    Lexer lexer("== != <= >= && || ++ -- += -= *= /=");
    std::vector<Token> tokens = lexer.tokenize();
    
    ASSERT_EQ(tokens.size(), 13); // 12 operators + EOF
    EXPECT_EQ(tokens[0].type, TokenType::OPERATOR);
    EXPECT_EQ(tokens[0].lexeme, "==");
    EXPECT_EQ(tokens[1].type, TokenType::OPERATOR);
    EXPECT_EQ(tokens[1].lexeme, "!=");
    EXPECT_EQ(tokens[2].type, TokenType::OPERATOR);
    EXPECT_EQ(tokens[2].lexeme, "<=");
    EXPECT_EQ(tokens[3].type, TokenType::OPERATOR);
    EXPECT_EQ(tokens[3].lexeme, ">=");
    EXPECT_EQ(tokens[4].type, TokenType::OPERATOR);
    EXPECT_EQ(tokens[4].lexeme, "&&");
    EXPECT_EQ(tokens[5].type, TokenType::OPERATOR);
    EXPECT_EQ(tokens[5].lexeme, "||");
    EXPECT_EQ(tokens[6].type, TokenType::OPERATOR);
    EXPECT_EQ(tokens[6].lexeme, "++");
    EXPECT_EQ(tokens[7].type, TokenType::OPERATOR);
    EXPECT_EQ(tokens[7].lexeme, "--");
    EXPECT_EQ(tokens[8].type, TokenType::OPERATOR);
    EXPECT_EQ(tokens[8].lexeme, "+=");
    EXPECT_EQ(tokens[9].type, TokenType::OPERATOR);
    EXPECT_EQ(tokens[9].lexeme, "-=");
    EXPECT_EQ(tokens[10].type, TokenType::OPERATOR);
    EXPECT_EQ(tokens[10].lexeme, "*=");
    EXPECT_EQ(tokens[11].type, TokenType::OPERATOR);
    EXPECT_EQ(tokens[11].lexeme, "/=");
}

TEST(LexerTest, TokenizeStringWithEscapedQuotes) {
    Lexer lexer("\"Hello \\\"world\\\"\"");
    std::vector<Token> tokens = lexer.tokenize();
    
    ASSERT_EQ(tokens.size(), 2); // String + EOF
    EXPECT_EQ(tokens[0].type, TokenType::STRING);
    EXPECT_EQ(tokens[0].lexeme, "Hello \\\"world\\\"");
}
