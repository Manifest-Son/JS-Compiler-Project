#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include "source_position.h"
#include "token.h"
#include "parser.h"

class Statement;
class ExprVisitor;
class StmtVisitor;
class ASTVisitor;

// Base AST node class
class ASTNode {
public:
    virtual ~ASTNode() = default;

    // Source tracking for error reporting and source maps
    SourcePosition start_pos;
    SourcePosition end_pos;

    // Optional source file name
    std::string source_file;

    // Set position from line and column numbers
    void setPosition(uint32_t start_line, uint32_t start_column, uint32_t end_line, uint32_t end_column) {
        start_pos = SourcePosition(start_line, start_column);
        end_pos = SourcePosition(end_line, end_column);
    }

    // Set position from tokens
    void setPosition(const Token &start_token, const Token &end_token) {
        start_pos = SourcePosition(start_token.line, start_token.column);
        end_pos = SourcePosition(end_token.line, end_token.column + end_token.lexeme.length());
    }

    // Set position from a single token (start = end)
    void setPosition(const Token &token) { setPosition(token, token); }

    // Get source range
    SourceRange getSourceRange() const { return SourceRange(start_pos, end_pos); }

    // Get location as string for error messages
    std::string getLocationString() const {
        std::string file_prefix = source_file.empty() ? "" : source_file + ":";
        return file_prefix + start_pos.toString();
    }
};

// Expression node base class
class Expression : public ASTNode {
public:
    ~Expression() override = default;
    virtual void accept(ExprVisitor &visitor) const = 0;

    // Type for storing evaluated compile-time constants
    using ConstantValue = std::variant<std::monostate, // No constant value
                                       double, // Number
                                       std::string, // String
                                       bool // Boolean
                                       >;

    // For constant folding optimization
    mutable ConstantValue constantValue;
    mutable bool isConstantEvaluated = false;

    // For type inference
    enum class Type { Unknown, Number, String, Boolean, Object, Array, Function, Null, Undefined };

    mutable Type inferredType = Type::Unknown;
};

// Literal expression (numbers, strings, booleans, null)
class LiteralExpr final : public Expression {
public:
    explicit LiteralExpr(Token token) : token(std::move(token)) {
        // Pre-compute constant value for optimization
        isConstantEvaluated = true;

        switch (token.type) {
            case TokenType::NUMBER:
                constantValue = std::stod(token.lexeme);
                inferredType = Type::Number;
                break;
            case TokenType::STRING:
                constantValue = token.lexeme.substr(1, token.lexeme.length() - 2); // Remove quotes
                inferredType = Type::String;
                break;
            case TokenType::TRUE:
                constantValue = true;
                inferredType = Type::Boolean;
                break;
            case TokenType::FALSE:
                constantValue = false;
                inferredType = Type::Boolean;
                break;
            case TokenType::NULL_KEYWORD:
                inferredType = Type::Null;
                break;
            default:
                isConstantEvaluated = false;
                break;
        }
    }

    // Additional constructor for boolean literals
    explicit LiteralExpr(bool value) {
        isConstantEvaluated = true;
        constantValue = value;
        inferredType = Type::Boolean;
    }

    // Additional constructor for null literal
    LiteralExpr(std::nullptr_t) {
        isConstantEvaluated = true;
        inferredType = Type::Null;
    }

    // Additional constructor for numeric literals
    explicit LiteralExpr(double value) {
        isConstantEvaluated = true;
        constantValue = value;
        inferredType = Type::Number;
    }

    // Additional constructor for string literals
    explicit LiteralExpr(const std::string &value) {
        isConstantEvaluated = true;
        constantValue = value;
        inferredType = Type::String;
    }

    void accept(ExprVisitor &visitor) const override;
    Token token;
};

// Variable reference
class VariableExpr final : public Expression {
public:
    explicit VariableExpr(Token name) : name(std::move(name)) {}
    void accept(ExprVisitor &visitor) const override;
    Token name;

    // For variable analysis
    mutable bool isInitialized = false;
    mutable bool isReferenced = false;
    mutable int scopeDepth = 0;
};

// Binary operation (a + b, a * b, etc.)
class BinaryExpr final : public Expression {
public:
    BinaryExpr(std::shared_ptr<Expression> left, Token op, std::shared_ptr<Expression> right) :
        left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    BinaryExpr(std::unique_ptr<Expression> left, Token op, std::unique_ptr<Expression> right) :
        left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    void accept(ExprVisitor &visitor) const override;
    std::shared_ptr<Expression> left;
    Token op;
    std::shared_ptr<Expression> right;
};

// Unary operation (-a, !b, etc.)
class UnaryExpr final : public Expression {
public:
    UnaryExpr(Token op, std::shared_ptr<Expression> right) : op(std::move(op)), right(std::move(right)) {}

    UnaryExpr(Token op, std::unique_ptr<Expression> right) : op(std::move(op)), right(std::move(right)) {}

    void accept(ExprVisitor &visitor) const override;
    Token op;
    std::shared_ptr<Expression> right;
};

// Assignment expression (a = value)
class AssignExpr final : public Expression {
public:
    AssignExpr(Token name, std::shared_ptr<Expression> value) : name(std::move(name)), value(std::move(value)) {}

    AssignExpr(Token name, std::unique_ptr<Expression> value) : name(std::move(name)), value(std::move(value)) {}

    void accept(ExprVisitor &visitor) const override;
    Token name;
    std::shared_ptr<Expression> value;
};

// Logical expression (&&, ||)
class LogicalExpr final : public Expression {
public:
    LogicalExpr(std::shared_ptr<Expression> left, Token op, std::shared_ptr<Expression> right) :
        left(std::move(left)), op(std::move(op)), right(std::move(right).get()) {}

    LogicalExpr(std::shared_ptr<Expression> left, Token op, std::unique_ptr<Expression> right) :
        left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    void accept(ExprVisitor &visitor) const override;
    std::shared_ptr<Expression> left;
    Token op;
    std::remove_reference<std::unique_ptr<Expression> &>::type right;
};

// Grouping expression (parenthesized expressions)
class GroupingExpr final : public Expression {
public:
    explicit GroupingExpr(std::shared_ptr<Expression> expression) : expression(std::move(expression)) {}

    explicit GroupingExpr(std::unique_ptr<Expression> expression) : expression(std::move(expression)) {}

    void accept(ExprVisitor &visitor) const override;
    std::shared_ptr<Expression> expression;
};

// Function call expression
class CallExpr final : public Expression {
public:
    CallExpr(std::shared_ptr<Expression> callee, Token paren, std::vector<std::shared_ptr<Expression>> arguments) :
        callee(std::move(callee)), paren(std::move(paren)), arguments(std::move(arguments)) {}

    CallExpr(std::unique_ptr<Expression> callee, Token paren, std::vector<std::unique_ptr<Expression>> arguments) :
        paren(std::move(paren)) {
        this->callee = std::move(callee);
        for (auto &arg: arguments) {
            this->arguments.push_back(std::move(arg));
        }
    }

    void accept(ExprVisitor &visitor) const override;
    std::shared_ptr<Expression> callee;
    Token paren; // Closing parenthesis token, for error reporting
    std::vector<std::shared_ptr<Expression>> arguments;
};

// Property access: obj.prop
class GetExpr final : public Expression {
public:
    GetExpr(std::shared_ptr<Expression> object, Token name) : object(std::move(object)), name(std::move(name)) {}

    GetExpr(std::unique_ptr<Expression> object, Token name) : object(std::move(object)), name(std::move(name)) {}

    void accept(ExprVisitor &visitor) const override;
    std::shared_ptr<Expression> object;
    Token name;
};

// Array literal: [1, 2, 3]
class ArrayExpr final : public Expression {
public:
    explicit ArrayExpr(std::vector<std::shared_ptr<Expression>> elements) : elements(std::move(elements)) {
        inferredType = Type::Array;
    }

    explicit ArrayExpr(std::vector<std::unique_ptr<Expression>> elements) {
        inferredType = Type::Array;
        for (auto &element: elements) {
            this->elements.push_back(std::move(element));
        }
    }

    void accept(ExprVisitor &visitor) const override;
    std::vector<std::shared_ptr<Expression>> elements;
};

// Object literal: { a: 1, b: 2 }
class ObjectExpr final : public Expression {
public:
    struct Property {
        Token key;
        std::shared_ptr<Expression> value;

        // Constructor that accepts unique_ptr
        Property(Token key, std::unique_ptr<Expression> value) : key(std::move(key)) { this->value = std::move(value); }

        // Constructor that accepts shared_ptr
        Property(Token key, std::shared_ptr<Expression> value) : key(std::move(key)), value(std::move(value)) {}
    };

    explicit ObjectExpr(std::vector<Property> properties) : properties(std::move(properties)) {
        inferredType = Type::Object;
    }

    void accept(ExprVisitor &visitor) const override;
    std::vector<Property> properties;
};

// Arrow function: (a, b) => a + b
class ArrowFunctionExpr final : public Expression {
public:
    ArrowFunctionExpr(std::vector<Token> parameters, std::shared_ptr<Expression> body) :
        parameters(std::move(parameters)), body(std::move(body)), bodyIsExpression(true) {}

    ArrowFunctionExpr(std::vector<Token> parameters, std::unique_ptr<Expression> body) :
        parameters(std::move(parameters)), bodyIsExpression(true) {
        this->body = std::move(body);
    }

    ArrowFunctionExpr(std::vector<Token> parameters, std::shared_ptr<Statement> blockBody) :
        parameters(std::move(parameters)), blockBody(std::move(blockBody)), bodyIsExpression(false) {}

    ArrowFunctionExpr(std::vector<Token> parameters, std::unique_ptr<Statement> blockBody) :
        parameters(std::move(parameters)), bodyIsExpression(false) {
        this->blockBody = std::move(blockBody);
    }

    void accept(ExprVisitor &visitor) const override;
    std::vector<Token> parameters;
    std::shared_ptr<Expression> body; // For expression bodies: (a) => a * 2
    std::shared_ptr<Statement> blockBody; // For block bodies: (a) => { return a * 2; }
    bool bodyIsExpression;

    // Function analysis
    mutable std::unordered_map<std::string, int> capturedVariables;
};

// Statement node base class
class Statement : public ASTNode {
public:
    ~Statement() override = default;
    virtual void accept(StmtVisitor &visitor) const = 0;
};

// Expression statement (standalone expression)
class ExpressionStmt final : public Statement {
public:
    explicit ExpressionStmt(std::shared_ptr<Expression> expression) : expression(std::move(expression)) {}

    explicit ExpressionStmt(std::unique_ptr<Expression> expression) : expression(std::move(expression)) {}

    void accept(StmtVisitor &visitor) const override;
    std::shared_ptr<Expression> expression;
};

// Variable declaration (let x = 5)
class VarDeclStmt final : public Statement {
public:
    VarDeclStmt(Token name, std::shared_ptr<Expression> initializer) :
        name(std::move(name)), initializer(std::move(initializer)) {}

    VarDeclStmt(Token name, std::unique_ptr<Expression> initializer) :
        name(std::move(name)), initializer(std::move(initializer)) {}

    void accept(StmtVisitor &visitor) const override;
    Token name;
    std::shared_ptr<Expression> initializer;

    // Variable analysis
    mutable bool isConst = false;
    mutable int scopeDepth = 0;
};

// Block statement ({ ... })
class BlockStmt final : public Statement {
public:
    explicit BlockStmt(std::vector<std::shared_ptr<Statement>> statements) : statements(std::move(statements)) {}

    explicit BlockStmt(std::vector<std::unique_ptr<Statement>> statements) {
        for (auto &stmt: statements) {
            this->statements.push_back(std::move(stmt));
        }
    }

    void accept(StmtVisitor &visitor) const override;
    std::vector<std::shared_ptr<Statement>> statements;
};

// If statement
class IfStmt final : public Statement {
public:
    IfStmt(std::shared_ptr<Expression> condition, std::shared_ptr<Statement> thenBranch,
           std::shared_ptr<Statement> elseBranch) :
        condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}

    IfStmt(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> thenBranch, std::unique_ptr<Statement> elseBranch) :
        condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}

    void accept(StmtVisitor &visitor) const override;
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Statement> thenBranch;
    std::shared_ptr<Statement> elseBranch;
};

// While statement
class WhileStmt final : public Statement {
public:
    WhileStmt(std::shared_ptr<Expression> condition, std::shared_ptr<Statement> body) :
        condition(std::move(condition)), body(std::move(body)) {}

    WhileStmt(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> body) :
        condition(std::move(condition)), body(std::move(body)) {}

    void accept(StmtVisitor &visitor) const override;
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Statement> body;
};

// For statement
class ForStmt final : public Statement {
public:
    ForStmt(std::shared_ptr<Statement> initializer, std::shared_ptr<Expression> condition,
            std::shared_ptr<Expression> increment, std::shared_ptr<Statement> body) :
        initializer(std::move(initializer)), condition(std::move(condition)), increment(std::move(increment)),
        body(std::move(body)) {}

    ForStmt(std::unique_ptr<Statement> initializer, std::unique_ptr<Expression> condition, std::unique_ptr<Expression> increment,
            std::unique_ptr<Statement> body) :
        initializer(std::move(initializer)), condition(std::move(condition)), increment(std::move(increment)),
        body(std::move(body)) {}

    void accept(StmtVisitor &visitor) const override;
    std::shared_ptr<Statement> initializer;
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Expression> increment;
    std::shared_ptr<Statement> body;
};

// Function declaration
class FunctionDeclStmt final : public Statement {
public:
    FunctionDeclStmt(Token name, std::vector<Token> params, std::vector<std::shared_ptr<Statement>> body) :
        name(std::move(name)), params(std::move(params)), body(std::move(body)) {}

    FunctionDeclStmt(Token name, std::vector<Token> params, std::vector<std::unique_ptr<Statement>> body) :
        name(std::move(name)), params(std::move(params)) {
        for (auto &stmt: body) {
            this->body.push_back(std::move(stmt));
        }
    }

    void accept(StmtVisitor &visitor) const override;
    Token name;
    std::vector<Token> params;
    std::vector<std::shared_ptr<Statement>> body;

    // Function analysis
    mutable bool isRecursive = false;
    mutable std::unordered_map<std::string, int> capturedVariables;
};

// Return statement
class ReturnStmt final : public Statement {
public:
    ReturnStmt(Token keyword, std::shared_ptr<Expression> value) :
        keyword(std::move(keyword)), value(std::move(value)) {}

    ReturnStmt(Token keyword, std::unique_ptr<Expression> value) : keyword(std::move(keyword)), value(std::move(value)) {}

    void accept(StmtVisitor &visitor) const override;
    Token keyword; // For location information in error reporting
    std::shared_ptr<Expression> value;
};

// Break statement
class BreakStmt final : public Statement {
public:
    explicit BreakStmt(Token keyword) : keyword(std::move(keyword)) {}

    void accept(StmtVisitor &visitor) const override;
    Token keyword;
};

// Continue statement
class ContinueStmt final : public Statement {
public:
    explicit ContinueStmt(Token keyword) : keyword(std::move(keyword)) {}

    void accept(StmtVisitor &visitor) const override;
    Token keyword;
};

// Class declaration
class ClassDeclStmt final : public Statement {
public:
    struct Method {
        Token name;
        std::vector<Token> params;
        std::vector<std::shared_ptr<Statement>> body;
        bool isStatic = false;
    };

    ClassDeclStmt(Token name, std::shared_ptr<Expression> superclass, std::vector<Method> methods) :
        name(std::move(name)), superclass(std::move(superclass)), methods(std::move(methods)) {}

    void accept(StmtVisitor &visitor) const override;
    Token name;
    std::shared_ptr<Expression> superclass; // null if no superclass
    std::vector<Method> methods;
};

// Program node (root of AST)
class Program final : public ASTNode {
public:
    explicit Program(std::vector<std::shared_ptr<Statement>> statements) : statements(std::move(statements)) {}

    explicit Program(std::vector<std::unique_ptr<Statement>> statements) {
        for (auto &stmt: statements) {
            this->statements.push_back(std::move(stmt));
        }
    }

    void accept(ASTVisitor &visitor) const;
    std::vector<std::shared_ptr<Statement>> statements;
};

#endif // AST_H
