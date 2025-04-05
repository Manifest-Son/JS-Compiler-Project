#include "../../include/ast.h"
#include <gtest/gtest.h>
#include "../../include/ast_visitor.h"
#include "../../include/token.h"

#include <memory>
#include <string>
#include <vector>

// Mock visitor for testing visitor pattern
class MockASTVisitor : public ASTVisitor {
public:
    std::vector<std::string> visitedNodes;

    // Expression node visit methods
    void visitLiteralExpr(const LiteralExpr &expr) override { visitedNodes.push_back("LiteralExpr"); }

    void visitVariableExpr(const VariableExpr &expr) override {
        visitedNodes.push_back("VariableExpr:" + expr.name.lexeme);
    }

    void visitBinaryExpr(const BinaryExpr &expr) override {
        visitedNodes.push_back("BinaryExpr:" + expr.op.lexeme);
        expr.left->accept(*this);
        expr.right->accept(*this);
    }

    void visitUnaryExpr(const UnaryExpr &expr) override {
        visitedNodes.push_back("UnaryExpr:" + expr.op.lexeme);
        expr.right->accept(*this);
    }

    void visitCallExpr(const CallExpr &expr) override {
        visitedNodes.push_back("CallExpr");
        expr.callee->accept(*this);
        for (const auto &arg: expr.arguments) {
            arg->accept(*this);
        }
    }

    void visitGetExpr(const GetExpr &expr) override {
        visitedNodes.push_back("GetExpr:" + expr.name.lexeme);
        expr.object->accept(*this);
    }

    void visitArrayExpr(const ArrayExpr &expr) override {
        visitedNodes.push_back("ArrayExpr");
        for (const auto &element: expr.elements) {
            element->accept(*this);
        }
    }

    void visitObjectExpr(const ObjectExpr &expr) override {
        visitedNodes.push_back("ObjectExpr");
        for (const auto &prop: expr.properties) {
            visitedNodes.push_back("Property:" + prop.key.lexeme);
            prop.value->accept(*this);
        }
    }

    void visitArrowFunctionExpr(const ArrowFunctionExpr &expr) override {
        std::string params;
        for (const auto &param: expr.parameters) {
            if (!params.empty())
                params += ",";
            params += param.lexeme;
        }
        visitedNodes.push_back("ArrowFunctionExpr:" + params);

        if (expr.bodyIsExpression) {
            expr.body->accept(*this);
        } else {
            if (expr.blockBody) {
                expr.blockBody->accept(*this);
            }
        }
    }

    // Statement node visit methods
    void visitExpressionStmt(const ExpressionStmt &stmt) override {
        visitedNodes.push_back("ExpressionStmt");
        stmt.expression->accept(*this);
    }

    void visitVarDeclStmt(const VarDeclStmt &stmt) override {
        visitedNodes.push_back("VarDeclStmt:" + stmt.name.lexeme);
        if (stmt.initializer) {
            stmt.initializer->accept(*this);
        }
    }

    void visitBlockStmt(const BlockStmt &stmt) override {
        visitedNodes.push_back("BlockStmt");
        for (const auto &statement: stmt.statements) {
            statement->accept(*this);
        }
    }

    void visitIfStmt(const IfStmt &stmt) override {
        visitedNodes.push_back("IfStmt");
        stmt.condition->accept(*this);
        stmt.thenBranch->accept(*this);
        if (stmt.elseBranch) {
            stmt.elseBranch->accept(*this);
        }
    }

    void visitWhileStmt(const WhileStmt &stmt) override {
        visitedNodes.push_back("WhileStmt");
        stmt.condition->accept(*this);
        stmt.body->accept(*this);
    }

    void visitForStmt(const ForStmt &stmt) override {
        visitedNodes.push_back("ForStmt");
        if (stmt.initializer) {
            stmt.initializer->accept(*this);
        }
        if (stmt.condition) {
            stmt.condition->accept(*this);
        }
        if (stmt.increment) {
            stmt.increment->accept(*this);
        }
        stmt.body->accept(*this);
    }

    void visitFunctionDeclStmt(const FunctionDeclStmt &stmt) override {
        visitedNodes.push_back("FunctionDeclStmt:" + stmt.name.lexeme);
        for (const auto &statement: stmt.body) {
            statement->accept(*this);
        }
    }

    void visitReturnStmt(const ReturnStmt &stmt) override {
        visitedNodes.push_back("ReturnStmt");
        if (stmt.value) {
            stmt.value->accept(*this);
        }
    }

    void visitBreakStmt(const BreakStmt &stmt) override { visitedNodes.push_back("BreakStmt"); }

    void visitContinueStmt(const ContinueStmt &stmt) override { visitedNodes.push_back("ContinueStmt"); }

    void visitClassDeclStmt(const ClassDeclStmt &stmt) override {
        visitedNodes.push_back("ClassDeclStmt:" + stmt.name.lexeme);
        if (stmt.superclass) {
            stmt.superclass->accept(*this);
        }
        for (const auto &method: stmt.methods) {
            visitedNodes.push_back(std::string(method.isStatic ? "StaticMethod:" : "Method:") + method.name.lexeme);
        }
    }

    void visitProgram(const Program &program) override {
        visitedNodes.push_back("Program");
        for (const auto &statement: program.statements) {
            statement->accept(*this);
        }
    }
};

// Helper to create tokens for testing
Token createToken(TokenType type, const std::string &lexeme) {
    Token token;
    token.type = type;
    token.lexeme = lexeme;
    token.line = 1;
    token.column = 1;
    return token;
}

// Test fixture
class ASTTest : public ::testing::Test {
protected:
    MockASTVisitor visitor;
};

// Tests for expression nodes
TEST_F(ASTTest, LiteralExprNodeTest) {
    // Test number literal
    auto numberToken = createToken(TokenType::NUMBER, "42");
    LiteralExpr numberExpr(numberToken);

    EXPECT_EQ(numberExpr.token.lexeme, "42");
    EXPECT_TRUE(numberExpr.isConstantEvaluated);
    EXPECT_EQ(std::get<double>(numberExpr.constantValue), 42.0);
    EXPECT_EQ(numberExpr.inferredType, Expression::Type::Number);

    // Test string literal
    auto stringToken = createToken(TokenType::STRING, "\"hello\"");
    LiteralExpr stringExpr(stringToken);

    EXPECT_EQ(stringExpr.token.lexeme, "\"hello\"");
    EXPECT_TRUE(stringExpr.isConstantEvaluated);
    EXPECT_EQ(std::get<std::string>(stringExpr.constantValue), "hello");
    EXPECT_EQ(stringExpr.inferredType, Expression::Type::String);

    // Test boolean literal
    auto boolToken = createToken(TokenType::TRUE_KEYWORD, "true");
    LiteralExpr boolExpr(boolToken);

    EXPECT_EQ(boolExpr.token.lexeme, "true");
    EXPECT_TRUE(boolExpr.isConstantEvaluated);
    EXPECT_EQ(std::get<bool>(boolExpr.constantValue), true);
    EXPECT_EQ(boolExpr.inferredType, Expression::Type::Boolean);

    // Test visitor pattern
    numberExpr.accept(visitor);
    EXPECT_EQ(visitor.visitedNodes.size(), 1);
    EXPECT_EQ(visitor.visitedNodes[0], "LiteralExpr");
}

TEST_F(ASTTest, VariableExprNodeTest) {
    auto nameToken = createToken(TokenType::IDENTIFIER, "someVar");
    VariableExpr varExpr(nameToken);

    EXPECT_EQ(varExpr.name.lexeme, "someVar");
    EXPECT_FALSE(varExpr.isInitialized);
    EXPECT_FALSE(varExpr.isReferenced);
    EXPECT_EQ(varExpr.scopeDepth, 0);

    // Test visitor pattern
    varExpr.accept(visitor);
    EXPECT_EQ(visitor.visitedNodes.size(), 1);
    EXPECT_EQ(visitor.visitedNodes[0], "VariableExpr:someVar");
}

TEST_F(ASTTest, BinaryExprNodeTest) {
    auto leftExpr = std::make_shared<LiteralExpr>(createToken(TokenType::NUMBER, "10"));
    auto opToken = createToken(TokenType::PLUS, "+");
    auto rightExpr = std::make_shared<LiteralExpr>(createToken(TokenType::NUMBER, "20"));

    BinaryExpr binaryExpr(leftExpr, opToken, rightExpr);

    EXPECT_EQ(binaryExpr.op.lexeme, "+");

    // Test visitor pattern
    binaryExpr.accept(visitor);
    EXPECT_EQ(visitor.visitedNodes.size(), 3);
    EXPECT_EQ(visitor.visitedNodes[0], "BinaryExpr:+");
    EXPECT_EQ(visitor.visitedNodes[1], "LiteralExpr");
    EXPECT_EQ(visitor.visitedNodes[2], "LiteralExpr");
}

TEST_F(ASTTest, UnaryExprNodeTest) {
    auto opToken = createToken(TokenType::MINUS, "-");
    auto rightExpr = std::make_shared<LiteralExpr>(createToken(TokenType::NUMBER, "42"));

    UnaryExpr unaryExpr(opToken, rightExpr);

    EXPECT_EQ(unaryExpr.op.lexeme, "-");

    // Test visitor pattern
    unaryExpr.accept(visitor);
    EXPECT_EQ(visitor.visitedNodes.size(), 2);
    EXPECT_EQ(visitor.visitedNodes[0], "UnaryExpr:-");
    EXPECT_EQ(visitor.visitedNodes[1], "LiteralExpr");
}

TEST_F(ASTTest, CallExprNodeTest) {
    auto callee = std::make_shared<VariableExpr>(createToken(TokenType::IDENTIFIER, "func"));
    auto parenToken = createToken(TokenType::RIGHT_PAREN, ")");
    std::vector<std::shared_ptr<Expression>> args = {
            std::make_shared<LiteralExpr>(createToken(TokenType::NUMBER, "1")),
            std::make_shared<LiteralExpr>(createToken(TokenType::NUMBER, "2"))};

    CallExpr callExpr(callee, parenToken, args);

    EXPECT_EQ(callExpr.arguments.size(), 2);

    // Test visitor pattern
    callExpr.accept(visitor);
    EXPECT_EQ(visitor.visitedNodes.size(), 4);
    EXPECT_EQ(visitor.visitedNodes[0], "CallExpr");
    EXPECT_EQ(visitor.visitedNodes[1], "VariableExpr:func");
    EXPECT_EQ(visitor.visitedNodes[2], "LiteralExpr");
    EXPECT_EQ(visitor.visitedNodes[3], "LiteralExpr");
}

TEST_F(ASTTest, GetExprNodeTest) {
    auto object = std::make_shared<VariableExpr>(createToken(TokenType::IDENTIFIER, "obj"));
    auto propToken = createToken(TokenType::IDENTIFIER, "prop");

    GetExpr getExpr(object, propToken);

    EXPECT_EQ(getExpr.name.lexeme, "prop");

    // Test visitor pattern
    getExpr.accept(visitor);
    EXPECT_EQ(visitor.visitedNodes.size(), 2);
    EXPECT_EQ(visitor.visitedNodes[0], "GetExpr:prop");
    EXPECT_EQ(visitor.visitedNodes[1], "VariableExpr:obj");
}

TEST_F(ASTTest, ArrayExprNodeTest) {
    std::vector<std::shared_ptr<Expression>> elements = {
            std::make_shared<LiteralExpr>(createToken(TokenType::NUMBER, "1")),
            std::make_shared<LiteralExpr>(createToken(TokenType::NUMBER, "2")),
            std::make_shared<LiteralExpr>(createToken(TokenType::NUMBER, "3"))};

    ArrayExpr arrayExpr(elements);

    EXPECT_EQ(arrayExpr.elements.size(), 3);
    EXPECT_EQ(arrayExpr.inferredType, Expression::Type::Array);

    // Test visitor pattern
    arrayExpr.accept(visitor);
    EXPECT_EQ(visitor.visitedNodes.size(), 4);
    EXPECT_EQ(visitor.visitedNodes[0], "ArrayExpr");
    EXPECT_EQ(visitor.visitedNodes[1], "LiteralExpr");
    EXPECT_EQ(visitor.visitedNodes[2], "LiteralExpr");
    EXPECT_EQ(visitor.visitedNodes[3], "LiteralExpr");
}

TEST_F(ASTTest, ObjectExprNodeTest) {
    std::vector<ObjectExpr::Property> properties = {
            {createToken(TokenType::IDENTIFIER, "x"),
             std::make_shared<LiteralExpr>(createToken(TokenType::NUMBER, "10"))},
            {createToken(TokenType::IDENTIFIER, "y"),
             std::make_shared<LiteralExpr>(createToken(TokenType::NUMBER, "20"))}};

    ObjectExpr objectExpr(properties);

    EXPECT_EQ(objectExpr.properties.size(), 2);
    EXPECT_EQ(objectExpr.inferredType, Expression::Type::Object);

    // Test visitor pattern
    objectExpr.accept(visitor);
    EXPECT_EQ(visitor.visitedNodes.size(), 6);
    EXPECT_EQ(visitor.visitedNodes[0], "ObjectExpr");
    EXPECT_EQ(visitor.visitedNodes[1], "Property:x");
    EXPECT_EQ(visitor.visitedNodes[2], "LiteralExpr");
    EXPECT_EQ(visitor.visitedNodes[3], "Property:y");
    EXPECT_EQ(visitor.visitedNodes[4], "LiteralExpr");
}

// Tests for statement nodes
TEST_F(ASTTest, ExpressionStmtNodeTest) {
    auto expr = std::make_shared<LiteralExpr>(createToken(TokenType::NUMBER, "42"));
    ExpressionStmt exprStmt(expr);

    // Test visitor pattern
    exprStmt.accept(visitor);
    EXPECT_EQ(visitor.visitedNodes.size(), 2);
    EXPECT_EQ(visitor.visitedNodes[0], "ExpressionStmt");
    EXPECT_EQ(visitor.visitedNodes[1], "LiteralExpr");
}

TEST_F(ASTTest, VarDeclStmtNodeTest) {
    auto nameToken = createToken(TokenType::IDENTIFIER, "myVar");
    auto initializer = std::make_shared<LiteralExpr>(createToken(TokenType::NUMBER, "42"));
    VarDeclStmt varDeclStmt(nameToken, initializer);

    EXPECT_EQ(varDeclStmt.name.lexeme, "myVar");
    EXPECT_FALSE(varDeclStmt.isConst);
    EXPECT_EQ(varDeclStmt.scopeDepth, 0);

    // Test visitor pattern
    varDeclStmt.accept(visitor);
    EXPECT_EQ(visitor.visitedNodes.size(), 2);
    EXPECT_EQ(visitor.visitedNodes[0], "VarDeclStmt:myVar");
    EXPECT_EQ(visitor.visitedNodes[1], "LiteralExpr");
}

TEST_F(ASTTest, BlockStmtNodeTest) {
    std::vector<std::shared_ptr<Statement>> statements = {
            std::make_shared<ExpressionStmt>(std::make_shared<LiteralExpr>(createToken(TokenType::NUMBER, "1"))),
            std::make_shared<ExpressionStmt>(std::make_shared<LiteralExpr>(createToken(TokenType::NUMBER, "2")))};

    BlockStmt blockStmt(statements);

    EXPECT_EQ(blockStmt.statements.size(), 2);

    // Test visitor pattern
    blockStmt.accept(visitor);
    EXPECT_EQ(visitor.visitedNodes.size(), 6);
    EXPECT_EQ(visitor.visitedNodes[0], "BlockStmt");
    EXPECT_EQ(visitor.visitedNodes[1], "ExpressionStmt");
    EXPECT_EQ(visitor.visitedNodes[2], "LiteralExpr");
    EXPECT_EQ(visitor.visitedNodes[3], "ExpressionStmt");
    EXPECT_EQ(visitor.visitedNodes[4], "LiteralExpr");
}

TEST_F(ASTTest, IfStmtNodeTest) {
    auto condition = std::make_shared<LiteralExpr>(createToken(TokenType::TRUE_KEYWORD, "true"));
    auto thenBranch =
            std::make_shared<ExpressionStmt>(std::make_shared<LiteralExpr>(createToken(TokenType::NUMBER, "1")));
    auto elseBranch =
            std::make_shared<ExpressionStmt>(std::make_shared<LiteralExpr>(createToken(TokenType::NUMBER, "2")));

    IfStmt ifStmt(condition, thenBranch, elseBranch);

    // Test visitor pattern
    ifStmt.accept(visitor);
    EXPECT_EQ(visitor.visitedNodes.size(), 7);
    EXPECT_EQ(visitor.visitedNodes[0], "IfStmt");
    EXPECT_EQ(visitor.visitedNodes[1], "LiteralExpr");
    EXPECT_EQ(visitor.visitedNodes[2], "ExpressionStmt");
    EXPECT_EQ(visitor.visitedNodes[3], "LiteralExpr");
    EXPECT_EQ(visitor.visitedNodes[4], "ExpressionStmt");
    EXPECT_EQ(visitor.visitedNodes[5], "LiteralExpr");
}

// Test the full program node
TEST_F(ASTTest, ProgramNodeTest) {
    std::vector<std::shared_ptr<Statement>> statements = {
            std::make_shared<VarDeclStmt>(createToken(TokenType::IDENTIFIER, "x"),
                                          std::make_shared<LiteralExpr>(createToken(TokenType::NUMBER, "10"))),
            std::make_shared<ExpressionStmt>(std::make_shared<BinaryExpr>(
                    std::make_shared<VariableExpr>(createToken(TokenType::IDENTIFIER, "x")),
                    createToken(TokenType::PLUS, "+"),
                    std::make_shared<LiteralExpr>(createToken(TokenType::NUMBER, "5"))))};

    Program program(statements);

    // Test visitor pattern
    program.accept(visitor);
    EXPECT_EQ(visitor.visitedNodes.size(), 8);
    EXPECT_EQ(visitor.visitedNodes[0], "Program");
    EXPECT_EQ(visitor.visitedNodes[1], "VarDeclStmt:x");
    EXPECT_EQ(visitor.visitedNodes[2], "LiteralExpr");
    EXPECT_EQ(visitor.visitedNodes[3], "ExpressionStmt");
    EXPECT_EQ(visitor.visitedNodes[4], "BinaryExpr:+");
    EXPECT_EQ(visitor.visitedNodes[5], "VariableExpr:x");
    EXPECT_EQ(visitor.visitedNodes[6], "LiteralExpr");
}
