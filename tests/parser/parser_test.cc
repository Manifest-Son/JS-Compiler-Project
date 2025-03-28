#include "gtest/gtest.h"
#include "../../include/lexer.h"
#include "../../include/parser.h"
#include "../../include/ast.h"

TEST(ParserTest, SimpleExpression) {
    Lexer lexer("5 + 3;");
    std::vector<Token> tokens = lexer.tokenize();
    
    Parser parser(tokens);
    std::shared_ptr<Program> program = parser.parse();
    
    ASSERT_EQ(program->statements.size(), 1);
    
    auto exprStmt = std::dynamic_pointer_cast<ExpressionStmt>(program->statements[0]);
    ASSERT_NE(exprStmt, nullptr);
    
    auto binaryExpr = std::dynamic_pointer_cast<BinaryExpr>(exprStmt->expression);
    ASSERT_NE(binaryExpr, nullptr);
    EXPECT_EQ(binaryExpr->op.lexeme, "+");
    
    auto left = std::dynamic_pointer_cast<LiteralExpr>(binaryExpr->left);
    auto right = std::dynamic_pointer_cast<LiteralExpr>(binaryExpr->right);
    
    ASSERT_NE(left, nullptr);
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(left->token.lexeme, "5");
    EXPECT_EQ(right->token.lexeme, "3");
}

TEST(ParserTest, VariableDeclaration) {
    Lexer lexer("let x = 10;");
    std::vector<Token> tokens = lexer.tokenize();
    
    Parser parser(tokens);
    std::shared_ptr<Program> program = parser.parse();
    
    ASSERT_EQ(program->statements.size(), 1);
    
    auto varDecl = std::dynamic_pointer_cast<VarDeclStmt>(program->statements[0]);
    ASSERT_NE(varDecl, nullptr);
    EXPECT_EQ(varDecl->name.lexeme, "x");
    
    auto initializer = std::dynamic_pointer_cast<LiteralExpr>(varDecl->initializer);
    ASSERT_NE(initializer, nullptr);
    EXPECT_EQ(initializer->token.lexeme, "10");
}

TEST(ParserTest, IfStatement) {
    Lexer lexer("if (x > 5) { let y = 10; }");
    std::vector<Token> tokens = lexer.tokenize();
    
    Parser parser(tokens);
    std::shared_ptr<Program> program = parser.parse();
    
    ASSERT_EQ(program->statements.size(), 1);
    
    auto ifStmt = std::dynamic_pointer_cast<IfStmt>(program->statements[0]);
    ASSERT_NE(ifStmt, nullptr);
    
    auto condition = std::dynamic_pointer_cast<BinaryExpr>(ifStmt->condition);
    ASSERT_NE(condition, nullptr);
    EXPECT_EQ(condition->op.lexeme, ">");
    
    auto thenBranch = std::dynamic_pointer_cast<BlockStmt>(ifStmt->thenBranch);
    ASSERT_NE(thenBranch, nullptr);
    EXPECT_EQ(thenBranch->statements.size(), 1);
}
