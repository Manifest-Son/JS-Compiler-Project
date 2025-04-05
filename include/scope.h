#ifndef SCOPE_H
#define SCOPE_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "token.h"

// Forward declarations
class Expression;
class VariableExpr;
class Statement;

// Symbol represents a variable declaration
struct Symbol {
    Token declaration; // Token where this variable was declared
    bool isInitialized; // Whether the variable has been initialized
    bool isConst; // Whether the variable is a constant
    bool isFunction; // Whether this symbol is a function
    int scopeDepth; // Lexical depth where this variable was declared
    bool isReferenced; // Whether this variable is referenced
    bool isCaptured; // Whether this variable is captured by a closure
    std::vector<SourcePosition> referencePositions; // Positions where this variable is referenced

    Symbol(Token declaration, bool isInitialized, bool isConst, bool isFunction, int scopeDepth) :
        declaration(std::move(declaration)), isInitialized(isInitialized), isConst(isConst), isFunction(isFunction),
        scopeDepth(scopeDepth), isReferenced(false), isCaptured(false) {}

    // Track a new reference to this symbol
    void addReference(const SourcePosition &position) {
        isReferenced = true;
        referencePositions.push_back(position);
    }
};

// Scope represents a lexical scope in the program
class Scope {
public:
    enum class ScopeType {
        GLOBAL, // Global scope
        FUNCTION, // Function scope
        BLOCK // Block scope
    };

    explicit Scope(ScopeType type = ScopeType::BLOCK, std::shared_ptr<Scope> enclosing = nullptr) :
        type(type), enclosing(std::move(enclosing)), depth(enclosing ? enclosing->depth + 1 : 0) {}

    // Declare a new variable in this scope
    void declare(const std::string &name, Symbol symbol);

    // Define an existing declared variable
    void define(const std::string &name);

    // Check if a variable exists in this scope (not any parent scopes)
    bool exists(const std::string &name) const;

    // Get a symbol from this scope (not any parent scopes)
    Symbol *getSymbol(const std::string &name);

    // Get a symbol from this scope or any enclosing scope
    Symbol *resolveSymbol(const std::string &name);

    // Mark a variable as referenced
    void markReferenced(const std::string &name);

    // Mark a variable as referenced with source position
    void markReferencedWithPosition(const std::string &name, const SourcePosition &pos);

    // Mark a variable as captured by a closure
    void markCaptured(const std::string &name);

    // Get the scope depth
    int getDepth() const { return depth; }

    // Get the enclosing scope
    std::shared_ptr<Scope> getEnclosing() const { return enclosing; }

    // Get the scope type
    ScopeType getType() const { return type; }

    // Get all captured variables
    std::unordered_map<std::string, int> getCapturedVariables() const;

    // Get all variables in this scope
    std::vector<std::pair<std::string, Symbol>> getAllSymbols() const;

    // Get all symbols in this scope or any enclosing scope
    std::vector<std::pair<std::string, Symbol>> getAllSymbolsInChain() const;

    // Check if this scope is an ancestor of another scope
    bool isAncestorOf(const std::shared_ptr<Scope> &other) const;

private:
    ScopeType type;
    std::shared_ptr<Scope> enclosing;
    std::unordered_map<std::string, Symbol> symbols;
    int depth; // Lexical depth of this scope
};

// ScopeManager handles scope creation and tracking during AST traversal
class ScopeManager {
public:
    ScopeManager();

    // Begin a new scope
    void beginScope(Scope::ScopeType type = Scope::ScopeType::BLOCK);

    // End the current scope
    void endScope();

    // Declare a variable in the current scope
    void declare(const std::string &name, Symbol symbol);

    // Define a variable in the current scope
    void define(const std::string &name);

    // Resolve a variable reference to its declaration
    Symbol *resolve(const std::string &name);

    // Mark a variable as referenced
    void markReferenced(const std::string &name);

    // Mark a variable as referenced with source position
    void markReferencedWithPosition(const std::string &name, const SourcePosition &pos);

    // Mark a variable as captured by a closure
    void markCaptured(const std::string &name);

    // Get the current scope
    std::shared_ptr<Scope> getCurrentScope() const { return currentScope; }

    // Get the current scope depth
    int getCurrentScopeDepth() const { return currentScope ? currentScope->getDepth() : 0; }

    // Check if we're currently in a function scope
    bool inFunctionScope() const;

    // Get all captured variables in the current function scope
    std::unordered_map<std::string, int> getCapturedVariables() const;

    // Find nearest enclosing function scope
    std::shared_ptr<Scope> findNearestFunctionScope() const;

    // Get entire scope chain to global scope
    std::vector<std::shared_ptr<Scope>> getScopeChain() const;

    // Get all variable declarations and references in the current scope chain
    std::unordered_map<std::string, std::vector<std::pair<int, bool>>> getVariablesInScopeChain() const;

    // Get the global scope
    std::shared_ptr<Scope> getGlobalScope() const { return globalScope; }

private:
    std::shared_ptr<Scope> currentScope;
    std::shared_ptr<Scope> globalScope;
};

#endif // SCOPE_H
