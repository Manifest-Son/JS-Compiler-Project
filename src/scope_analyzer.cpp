#include "../include/scope_analyzer.h"
#include <iostream>
#include "../include/ast.h"

ScopeAnalyzer::ScopeAnalyzer() = default;

void ScopeAnalyzer::analyze(const Program &program) { visitProgram(program); }

void ScopeAnalyzer::visitProgram(const Program &program) {
    // Start with global scope
    scopeManager.beginScope(Scope::ScopeType::GLOBAL);

    // Visit all statements in the program
    for (const auto &stmt: program.statements) {
        stmt->accept(*this);
    }

    scopeManager.endScope();
}

void ScopeAnalyzer::visitLiteralExpr(const LiteralExpr &expr) {
    // Literals don't reference variables
}

void ScopeAnalyzer::visitVariableExpr(const VariableExpr &expr) {
    // Mark the variable as referenced in its scope
    std::string name = expr.name.lexeme;
    scopeManager.markReferenced(name);

    // Check if we're in a function and this variable is from an outer scope
    if (!functionStack.empty()) {
        Symbol *symbol = scopeManager.resolve(name);
        if (symbol && symbol->scopeDepth < scopeManager.getCurrentScopeDepth() &&
            symbol->scopeDepth > 0) { // Exclude globals

            // This is a captured variable
            scopeManager.markCaptured(name);
            functionStack.top().capturedVars[name] = symbol->scopeDepth;
        }
    }
}

void ScopeAnalyzer::visitBinaryExpr(const BinaryExpr &expr) {
    // Visit both sides of the binary expression
    expr.left->accept(*this);
    expr.right->accept(*this);
}

void ScopeAnalyzer::visitUnaryExpr(const UnaryExpr &expr) {
    // Visit the expression being operated on
    expr.right->accept(*this);
}

void ScopeAnalyzer::visitCallExpr(const CallExpr &expr) {
    // Visit the callee expression
    expr.callee->accept(*this);

    // Visit all arguments
    for (const auto &arg: expr.arguments) {
        arg->accept(*this);
    }
}

void ScopeAnalyzer::visitGetExpr(const GetExpr &expr) {
    // Visit the object expression
    expr.object->accept(*this);
}

void ScopeAnalyzer::visitArrayExpr(const ArrayExpr &expr) {
    // Visit all elements in the array
    for (const auto &element: expr.elements) {
        element->accept(*this);
    }
}

void ScopeAnalyzer::visitObjectExpr(const ObjectExpr &expr) {
    // Visit all property values in the object
    for (const auto &prop: expr.properties) {
        prop.value->accept(*this);
    }
}

void ScopeAnalyzer::visitArrowFunctionExpr(const ArrowFunctionExpr &expr) {
    // Create a new function scope
    std::string funcName = "<arrow_function>";
    enterFunction(funcName);

    // Begin function scope
    scopeManager.beginScope(Scope::ScopeType::FUNCTION);

    // Declare parameters in the function scope
    for (const auto &param: expr.parameters) {
        Symbol symbol(param, true, false, false, scopeManager.getCurrentScopeDepth());
        scopeManager.declare(param.lexeme, symbol);
    }

    // Visit function body
    if (expr.bodyIsExpression) {
        expr.body->accept(*this);
    } else {
        expr.blockBody->accept(*this);
    }

    // Update the arrow function with captured variables
    expr.capturedVariables = scopeManager.getCapturedVariables();

    // Store closure info
    closureInfo[funcName] = functionStack.top().capturedVars;

    // End function scope
    scopeManager.endScope();

    // Exit function context
    exitFunction();
}

void ScopeAnalyzer::visitExpressionStmt(const ExpressionStmt &stmt) {
    // Visit the expression
    stmt.expression->accept(*this);
}

void ScopeAnalyzer::visitVarDeclStmt(const VarDeclStmt &stmt) {
    // Check if the variable has an initializer
    if (stmt.initializer) {
        // Process the initializer first to handle cases like: let a = a;
        stmt.initializer->accept(*this);
    }

    // Declare variable in current scope
    std::string name = stmt.name.lexeme;
    Symbol symbol(stmt.name, stmt.initializer != nullptr, stmt.isConst, false, scopeManager.getCurrentScopeDepth());
    scopeManager.declare(name, symbol);

    // Mark the variable scope depth
    stmt.scopeDepth = scopeManager.getCurrentScopeDepth();

    // If initializer is missing and it's not a const, add to uninitialized list
    if (!stmt.initializer && !stmt.isConst) {
        uninitializedVars.push_back(name);
    }
}

void ScopeAnalyzer::visitBlockStmt(const BlockStmt &stmt) {
    // Create a new block scope
    scopeManager.beginScope();

    // Visit all statements in the block
    for (const auto &s: stmt.statements) {
        s->accept(*this);
    }

    // End the block scope
    scopeManager.endScope();
}

void ScopeAnalyzer::visitIfStmt(const IfStmt &stmt) {
    // Visit the condition
    stmt.condition->accept(*this);

    // Visit the then branch
    stmt.thenBranch->accept(*this);

    // Visit the else branch if it exists
    if (stmt.elseBranch) {
        stmt.elseBranch->accept(*this);
    }
}

void ScopeAnalyzer::visitWhileStmt(const WhileStmt &stmt) {
    // Visit the condition
    stmt.condition->accept(*this);

    // Visit the body
    stmt.body->accept(*this);
}

void ScopeAnalyzer::visitForStmt(const ForStmt &stmt) {
    // Create a new scope for the for statement
    scopeManager.beginScope();

    // Visit the initializer if it exists
    if (stmt.initializer) {
        stmt.initializer->accept(*this);
    }

    // Visit the condition if it exists
    if (stmt.condition) {
        stmt.condition->accept(*this);
    }

    // Visit the increment if it exists
    if (stmt.increment) {
        stmt.increment->accept(*this);
    }

    // Visit the body
    stmt.body->accept(*this);

    // End the for statement scope
    scopeManager.endScope();
}

void ScopeAnalyzer::visitFunctionDeclStmt(const FunctionDeclStmt &stmt) {
    // Declare the function in the current scope (hoisted)
    std::string name = stmt.name.lexeme;
    Symbol symbol(stmt.name, true, false, true, scopeManager.getCurrentScopeDepth());
    scopeManager.declare(name, symbol);

    // Start a new function context
    enterFunction(name);

    // Create a new function scope
    scopeManager.beginScope(Scope::ScopeType::FUNCTION);

    // Declare parameters in the function scope
    for (const auto &param: stmt.params) {
        Symbol paramSymbol(param, true, false, false, scopeManager.getCurrentScopeDepth());
        scopeManager.declare(param.lexeme, paramSymbol);
    }

    // Visit all statements in the function body
    for (const auto &s: stmt.body) {
        s->accept(*this);
    }

    // Update the function with captured variables
    stmt.capturedVariables = scopeManager.getCapturedVariables();

    // Store closure info
    closureInfo[name] = functionStack.top().capturedVars;

    // End the function scope
    scopeManager.endScope();

    // Exit function context
    exitFunction();
}

void ScopeAnalyzer::visitReturnStmt(const ReturnStmt &stmt) {
    // Visit the return value if it exists
    if (stmt.value) {
        stmt.value->accept(*this);
    }
}

void ScopeAnalyzer::visitBreakStmt(const BreakStmt &stmt) {
    // Break statement doesn't involve variables
}

void ScopeAnalyzer::visitContinueStmt(const ContinueStmt &stmt) {
    // Continue statement doesn't involve variables
}

void ScopeAnalyzer::visitClassDeclStmt(const ClassDeclStmt &stmt) {
    // Declare the class in the current scope
    std::string name = stmt.name.lexeme;
    Symbol symbol(stmt.name, true, false, false, scopeManager.getCurrentScopeDepth());
    scopeManager.declare(name, symbol);

    // Visit the superclass expression if it exists
    if (stmt.superclass) {
        stmt.superclass->accept(*this);
    }

    // Visit each method
    for (const auto &method: stmt.methods) {
        // Create a function context for the method
        std::string methodName = name + "." + method.name.lexeme;
        enterFunction(methodName);

        // Create a new function scope
        scopeManager.beginScope(Scope::ScopeType::FUNCTION);

        // Declare 'this' parameter implicitly
        Token thisToken("this", TokenType::IDENTIFIER, stmt.name.line, stmt.name.column);
        Symbol thisSymbol(thisToken, true, false, false, scopeManager.getCurrentScopeDepth());
        scopeManager.declare("this", thisSymbol);

        // Declare parameters
        for (const auto &param: method.params) {
            Symbol paramSymbol(param, true, false, false, scopeManager.getCurrentScopeDepth());
            scopeManager.declare(param.lexeme, paramSymbol);
        }

        // Visit method body
        for (const auto &s: method.body) {
            s->accept(*this);
        }

        // Store closure info
        closureInfo[methodName] = functionStack.top().capturedVars;

        // End method scope
        scopeManager.endScope();

        // Exit function context
        exitFunction();
    }
}

void ScopeAnalyzer::enterFunction(const std::string &name) {
    // Push a new function context
    FunctionContext context;
    context.name = name;
    functionStack.push(context);
}

void ScopeAnalyzer::exitFunction() {
    if (!functionStack.empty()) {
        functionStack.pop();
    }
}

std::vector<std::string> ScopeAnalyzer::getUnreferencedVariables() const { return unreferencedVars; }

std::vector<std::string> ScopeAnalyzer::getUninitializedVariables() const { return uninitializedVars; }

std::unordered_map<std::string, std::unordered_map<std::string, int>> ScopeAnalyzer::getClosureInfo() const {
    return closureInfo;
}
