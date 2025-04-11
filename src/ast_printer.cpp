#include "../include/ast_printer.h"
#include <sstream>

void ASTPrinter::visitProgram(const Program &program) {
    std::cout << "Program:" << std::endl;
    indent_level++;
    for (const auto &stmt: program.statements) {
        stmt->accept(*this);
    }
    indent_level--;
}

void ASTPrinter::visitExpressionStmt(const ExpressionStmt &stmt) {
    printIndent();
    std::cout << "ExpressionStmt:" << std::endl;
    indent_level++;
    stmt.expression->accept(*this);
    indent_level--;
}

void ASTPrinter::visitVarDeclStmt(const VarDeclStmt &stmt) {
    printIndent();
    std::cout << "VarDeclStmt: " << stmt.name << std::endl;
    if (stmt.initializer) {
        indent_level++;
        stmt.initializer->accept(*this);
        indent_level--;
    }
}

void ASTPrinter::visitBlockStmt(const BlockStmt &stmt) {
    printIndent();
    std::cout << "BlockStmt:" << std::endl;
    indent_level++;
    for (const auto &statement: stmt.statements) {
        statement->accept(*this);
    }
    indent_level--;
}

void ASTPrinter::visitIfStmt(const IfStmt &stmt) {
    printIndent();
    std::cout << "IfStmt:" << std::endl;
    indent_level++;
    printIndent();
    std::cout << "Condition:" << std::endl;
    indent_level++;
    stmt.condition->accept(*this);
    indent_level--;
    printIndent();
    std::cout << "ThenBranch:" << std::endl;
    indent_level++;
    stmt.then_branch->accept(*this);
    indent_level--;
    if (stmt.else_branch) {
        printIndent();
        std::cout << "ElseBranch:" << std::endl;
        indent_level++;
        stmt.else_branch->accept(*this);
        indent_level--;
    }
    indent_level--;
}

void ASTPrinter::visitWhileStmt(const WhileStmt &stmt) {
    printIndent();
    std::cout << "WhileStmt:" << std::endl;
    indent_level++;
    printIndent();
    std::cout << "Condition:" << std::endl;
    indent_level++;
    stmt.condition->accept(*this);
    indent_level--;
    printIndent();
    std::cout << "Body:" << std::endl;
    indent_level++;
    stmt.body->accept(*this);
    indent_level--;
    indent_level--;
}

void ASTPrinter::visitForStmt(const ForStmt &stmt) {
    printIndent();
    std::cout << "ForStmt:" << std::endl;
    indent_level++;

    if (stmt.initializer) {
        printIndent();
        std::cout << "Initializer:" << std::endl;
        indent_level++;
        stmt.initializer->accept(*this);
        indent_level--;
    }

    if (stmt.condition) {
        printIndent();
        std::cout << "Condition:" << std::endl;
        indent_level++;
        stmt.condition->accept(*this);
        indent_level--;
    }

    if (stmt.increment) {
        printIndent();
        std::cout << "Increment:" << std::endl;
        indent_level++;
        stmt.increment->accept(*this);
        indent_level--;
    }

    printIndent();
    std::cout << "Body:" << std::endl;
    indent_level++;
    stmt.body->accept(*this);
    indent_level--;

    indent_level--;
}

void ASTPrinter::visitFunctionDeclStmt(const FunctionDeclStmt &stmt) {
    printIndent();
    std::cout << "FunctionDeclStmt: " << stmt.name << std::endl;
    indent_level++;
    printIndent();
    std::cout << "Parameters:";
    for (const auto &param: stmt.parameters) {
        std::cout << " " << param;
    }
    std::cout << std::endl;
    printIndent();
    std::cout << "Body:" << std::endl;
    indent_level++;
    stmt.body->accept(*this);
    indent_level--;
    indent_level--;
}

void ASTPrinter::visitReturnStmt(const ReturnStmt &stmt) {
    printIndent();
    std::cout << "ReturnStmt:" << std::endl;
    if (stmt.value) {
        indent_level++;
        stmt.value->accept(*this);
        indent_level--;
    }
}

void ASTPrinter::visitBreakStmt(const BreakStmt &stmt) {
    printIndent();
    std::cout << "BreakStmt" << std::endl;
}

void ASTPrinter::visitContinueStmt(const ContinueStmt &stmt) {
    printIndent();
    std::cout << "ContinueStmt" << std::endl;
}

void ASTPrinter::visitClassDeclStmt(const ClassDeclStmt &stmt) {
    printIndent();
    std::cout << "ClassDeclStmt: " << stmt.name << std::endl;
    indent_level++;
    for (const auto &method: stmt.methods) {
        method->accept(*this);
    }
    indent_level--;
}

void ASTPrinter::visitLiteralExpr(const LiteralExpr &expr) {
    printIndent();
    std::cout << "LiteralExpr: " << expr.value << std::endl;
}

void ASTPrinter::visitVariableExpr(const VariableExpr &expr) {
    printIndent();
    std::cout << "VariableExpr: " << expr.name << std::endl;
}

void ASTPrinter::visitBinaryExpr(const BinaryExpr &expr) {
    printIndent();
    std::cout << "BinaryExpr: " << tokenTypeToString(expr.op) << std::endl;
    indent_level++;
    expr.left->accept(*this);
    expr.right->accept(*this);
    indent_level--;
}

void ASTPrinter::visitUnaryExpr(const UnaryExpr &expr) {
    printIndent();
    std::cout << "UnaryExpr: " << tokenTypeToString(expr.op) << std::endl;
    indent_level++;
    expr.right->accept(*this);
    indent_level--;
}

void ASTPrinter::visitCallExpr(const CallExpr &expr) {
    printIndent();
    std::cout << "CallExpr:" << std::endl;
    indent_level++;
    printIndent();
    std::cout << "Callee:" << std::endl;
    indent_level++;
    expr.callee->accept(*this);
    indent_level--;
    printIndent();
    std::cout << "Arguments:" << std::endl;
    indent_level++;
    for (const auto &arg: expr.arguments) {
        arg->accept(*this);
    }
    indent_level--;
    indent_level--;
}

void ASTPrinter::visitGetExpr(const GetExpr &expr) {
    printIndent();
    std::cout << "GetExpr: " << expr.name << std::endl;
    indent_level++;
    expr.object->accept(*this);
    indent_level--;
}

void ASTPrinter::visitArrayExpr(const ArrayExpr &expr) {
    printIndent();
    std::cout << "ArrayExpr: [" << expr.elements.size() << " elements]" << std::endl;
    indent_level++;
    for (const auto &element: expr.elements) {
        element->accept(*this);
    }
    indent_level--;
}

void ASTPrinter::visitObjectExpr(const ObjectExpr &expr) {
    printIndent();
    std::cout << "ObjectExpr: {" << expr.properties.size() << " properties}" << std::endl;
    indent_level++;
    for (const auto &[name, value]: expr.properties) {
        printIndent();
        std::cout << "Property: " << name << std::endl;
        indent_level++;
        value->accept(*this);
        indent_level--;
    }
    indent_level--;
}

void ASTPrinter::visitArrowFunctionExpr(const ArrowFunctionExpr &expr) {
    printIndent();
    std::cout << "ArrowFunctionExpr:" << std::endl;
    indent_level++;
    printIndent();
    std::cout << "Parameters:";
    for (const auto &param: expr.parameters) {
        std::cout << " " << param;
    }
    std::cout << std::endl;
    printIndent();
    std::cout << "Body:" << std::endl;
    indent_level++;
    expr.body->accept(*this);
    indent_level--;
    indent_level--;
}

void ASTPrinter::visitAssignExpr(const AssignExpr &expr) {
    printIndent();
    std::cout << "AssignExpr: " << expr.name.lexeme << std::endl;
    indent_level++;
    expr.value->accept(*this);
    indent_level--;
}

void ASTPrinter::visitLogicalExpr(const LogicalExpr &expr) {
    printIndent();
    std::cout << "LogicalExpr: " << expr.op.lexeme << std::endl;
    indent_level++;
    expr.left->accept(*this);
    expr.right->accept(*this);
    indent_level--;
}

void ASTPrinter::visitGroupingExpr(const GroupingExpr &expr) {
    printIndent();
    std::cout << "GroupingExpr:" << std::endl;
    indent_level++;
    expr.expression->accept(*this);
    indent_level--;
}

std::string ASTPrinter::print(const Program *program) {
    // Redirect cout to a stringstream to capture the output
    std::stringstream buffer;
    std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());

    // Visit the program to generate the output
    program->accept(*this);

    // Restore cout
    std::cout.rdbuf(old);

    // Return the captured output
    return buffer.str();
}
