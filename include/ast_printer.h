#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include <string>
#include <sstream>
#include "ast.h"

// AST printer implementation using the visitor pattern
class ASTPrinter {
public:
    std::string print(const std::shared_ptr<ASTNode> &node) {
        if (auto program = std::dynamic_pointer_cast<Program>(node)) {
            return printProgram(program);
        } else if (auto expr = std::dynamic_pointer_cast<Expression>(node)) {
            return printExpression(expr);
        } else if (auto stmt = std::dynamic_pointer_cast<Statement>(node)) {
            return printStatement(stmt);
        }
        return "Unknown AST node";
    }

    std::string print(const Program& node) {
        std::stringstream ss;
        ss << "Program:\n";
        indentLevel++;

        for (const auto& stmt : node.statements) {
            ss << getIndent() << printStatement(stmt) << "\n";
        }

        indentLevel--;
        return ss.str();
    }

private:
    int indentLevel = 0;
    
    std::string getIndent() {
        return std::string(indentLevel * 2, ' ');
    }
    
    std::string printProgram(const std::shared_ptr<Program>& program) {
        std::stringstream ss;
        ss << "Program:\n";
        indentLevel++;
        
        for (const auto& stmt : program->statements) {
            ss << getIndent() << printStatement(stmt) << "\n";
        }
        
        indentLevel--;
        return ss.str();
    }
    
    std::string printStatement(const std::shared_ptr<Statement>& stmt) {
        if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStmt>(stmt)) {
            return "ExpressionStmt: " + printExpression(exprStmt->expression);
        } else if (auto varDeclStmt = std::dynamic_pointer_cast<VarDeclStmt>(stmt)) {
            std::string result = "VarDeclStmt: " + varDeclStmt->name.lexeme;
            if (varDeclStmt->initializer) {
                result += " = " + printExpression(varDeclStmt->initializer);
            }
            return result;
        } else if (auto blockStmt = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
            std::stringstream ss;
            ss << "BlockStmt:";
            indentLevel++;
            
            for (const auto& s : blockStmt->statements) {
                ss << "\n" << getIndent() << printStatement(s);
            }
            
            indentLevel--;
            return ss.str();
        } else if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
            std::stringstream ss;
            ss << "IfStmt: condition=" << printExpression(ifStmt->condition);
            
            indentLevel++;
            ss << "\n" << getIndent() << "then: " << printStatement(ifStmt->thenBranch);
            
            if (ifStmt->elseBranch) {
                ss << "\n" << getIndent() << "else: " << printStatement(ifStmt->elseBranch);
            }
            
            indentLevel--;
            return ss.str();
        }
        
        return "Unknown statement";
    }
    
    std::string printExpression(const std::shared_ptr<Expression>& expr) {
        if (auto literal = std::dynamic_pointer_cast<LiteralExpr>(expr)) {
            return "Literal(" + literal->token.lexeme + ")";
        } else if (auto variable = std::dynamic_pointer_cast<VariableExpr>(expr)) {
            return "Variable(" + variable->name.lexeme + ")";
        } else if (auto binary = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
            return "Binary(" + 
                   printExpression(binary->left) + " " + 
                   binary->op.lexeme + " " + 
                   printExpression(binary->right) + ")";
        } else if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
            return "Unary(" + unary->op.lexeme + printExpression(unary->right) + ")";
        }
        
        return "Unknown expression";
    }
};

#endif // AST_PRINTER_H
