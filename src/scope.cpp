#include "../include/scope.h"
#include <stdexcept>

// Scope class implementation

void Scope::declare(const std::string &name, Symbol symbol) {
    // Add variable to current scope
    symbols[name] = std::move(symbol);
}

void Scope::define(const std::string &name) {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        it->second.isInitialized = true;
    }
}

bool Scope::exists(const std::string &name) const { return symbols.find(name) != symbols.end(); }

Symbol *Scope::getSymbol(const std::string &name) {
    auto it = symbols.find(name);
    return it != symbols.end() ? &it->second : nullptr;
}

Symbol *Scope::resolveSymbol(const std::string &name) {
    // Look in current scope
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return &it->second;
    }

    // If not found and we have an enclosing scope, look there
    if (enclosing) {
        // If we're looking from a function scope to an outer scope, variables are captured
        if (type == ScopeType::FUNCTION) {
            Symbol *symbol = enclosing->resolveSymbol(name);
            if (symbol) {
                symbol->isCaptured = true;
            }
            return symbol;
        } else {
            return enclosing->resolveSymbol(name);
        }
    }

    return nullptr;
}

void Scope::markReferenced(const std::string &name) {
    auto symbol = resolveSymbol(name);
    if (symbol) {
        symbol->isReferenced = true;
    }
}

void Scope::markReferencedWithPosition(const std::string &name, const SourcePosition &pos) {
    auto symbol = resolveSymbol(name);
    if (symbol) {
        symbol->isReferenced = true;
        symbol->addReference(pos);
    }
}

void Scope::markCaptured(const std::string &name) {
    auto symbol = resolveSymbol(name);
    if (symbol) {
        symbol->isCaptured = true;
    }
}

std::unordered_map<std::string, int> Scope::getCapturedVariables() const {
    std::unordered_map<std::string, int> captured;

    // Collect captured variables from current scope
    for (const auto &pair: symbols) {
        if (pair.second.isCaptured) {
            captured[pair.first] = pair.second.scopeDepth;
        }
    }

    return captured;
}

std::vector<std::pair<std::string, Symbol>> Scope::getAllSymbols() const {
    std::vector<std::pair<std::string, Symbol>> result;
    for (const auto &pair: symbols) {
        result.emplace_back(pair.first, pair.second);
    }
    return result;
}

std::vector<std::pair<std::string, Symbol>> Scope::getAllSymbolsInChain() const {
    std::vector<std::pair<std::string, Symbol>> result = getAllSymbols();

    // If there's an enclosing scope, add its symbols too
    if (enclosing) {
        auto enclosingSymbols = enclosing->getAllSymbolsInChain();
        result.insert(result.end(), enclosingSymbols.begin(), enclosingSymbols.end());
    }

    return result;
}

bool Scope::isAncestorOf(const std::shared_ptr<Scope> &other) const {
    std::shared_ptr<Scope> current = other->enclosing;
    while (current) {
        if (current.get() == this) {
            return true;
        }
        current = current->enclosing;
    }
    return false;
}

// ScopeManager implementation

ScopeManager::ScopeManager() {
    // Create global scope
    globalScope = std::make_shared<Scope>(Scope::ScopeType::GLOBAL);
    currentScope = globalScope;
}

void ScopeManager::beginScope(Scope::ScopeType type) {
    // Create new scope with the current one as enclosing
    currentScope = std::make_shared<Scope>(type, currentScope);
}

void ScopeManager::endScope() {
    if (currentScope == globalScope) {
        throw std::runtime_error("Cannot end global scope");
    }

    // Move back to the enclosing scope
    currentScope = currentScope->getEnclosing();
}

void ScopeManager::declare(const std::string &name, Symbol symbol) { currentScope->declare(name, std::move(symbol)); }

void ScopeManager::define(const std::string &name) { currentScope->define(name); }

Symbol *ScopeManager::resolve(const std::string &name) { return currentScope->resolveSymbol(name); }

void ScopeManager::markReferenced(const std::string &name) { currentScope->markReferenced(name); }

void ScopeManager::markReferencedWithPosition(const std::string &name, const SourcePosition &pos) {
    currentScope->markReferencedWithPosition(name, pos);
}

void ScopeManager::markCaptured(const std::string &name) { currentScope->markCaptured(name); }

bool ScopeManager::inFunctionScope() const {
    // Check if we're in a function scope or any of its ancestors are function scopes
    std::shared_ptr<Scope> scope = currentScope;
    while (scope && scope != globalScope) {
        if (scope->getType() == Scope::ScopeType::FUNCTION) {
            return true;
        }
        scope = scope->getEnclosing();
    }
    return false;
}

std::unordered_map<std::string, int> ScopeManager::getCapturedVariables() const {
    std::unordered_map<std::string, int> captured;

    // Find the nearest function scope
    std::shared_ptr<Scope> scope = currentScope;
    while (scope && scope->getType() != Scope::ScopeType::FUNCTION) {
        scope = scope->getEnclosing();
    }

    // If we found a function scope, get its captured variables
    if (scope && scope->getType() == Scope::ScopeType::FUNCTION) {
        captured = scope->getCapturedVariables();
    }

    return captured;
}

std::shared_ptr<Scope> ScopeManager::findNearestFunctionScope() const {
    std::shared_ptr<Scope> scope = currentScope;
    while (scope && scope->getType() != Scope::ScopeType::FUNCTION) {
        scope = scope->getEnclosing();
    }
    return scope;
}

std::vector<std::shared_ptr<Scope>> ScopeManager::getScopeChain() const {
    std::vector<std::shared_ptr<Scope>> chain;
    std::shared_ptr<Scope> scope = currentScope;

    // Build the scope chain from current to global
    while (scope) {
        chain.push_back(scope);
        scope = scope->getEnclosing();
    }

    return chain;
}

std::unordered_map<std::string, std::vector<std::pair<int, bool>>> ScopeManager::getVariablesInScopeChain() const {
    std::unordered_map<std::string, std::vector<std::pair<int, bool>>> result;

    // Get the entire scope chain
    auto scopeChain = getScopeChain();

    // For each scope, collect its variables
    for (const auto &scope: scopeChain) {
        auto symbols = scope->getAllSymbols();

        for (const auto &[name, symbol]: symbols) {
            // Store variable info: <scopeDepth, isCaptured>
            result[name].emplace_back(symbol.scopeDepth, symbol.isCaptured);
        }
    }

    return result;
}

// Example demonstration of variable tracking and analysis
void analyzeVariableUsage(ScopeManager &scopeManager, const std::string &variableName) {
    // Resolve the variable in the current scope chain
    Symbol *symbol = scopeManager.resolve(variableName);

    if (!symbol) {
        std::cerr << "Variable '" << variableName << "' not found in scope chain." << std::endl;
        return;
    }

    // Print basic information about the variable
    std::cout << "Variable '" << variableName << "' found:" << std::endl;
    std::cout << " - Declared at: " << symbol->declaration.position << std::endl;
    std::cout << " - Scope depth: " << symbol->scopeDepth << std::endl;
    std::cout << " - Is initialized: " << (symbol->isInitialized ? "yes" : "no") << std::endl;
    std::cout << " - Is constant: " << (symbol->isConst ? "yes" : "no") << std::endl;
    std::cout << " - Is function: " << (symbol->isFunction ? "yes" : "no") << std::endl;
    std::cout << " - Is referenced: " << (symbol->isReferenced ? "yes" : "no") << std::endl;
    std::cout << " - Is captured by closure: " << (symbol->isCaptured ? "yes" : "no") << std::endl;

    // Print all reference positions
    std::cout << " - Reference positions:" << std::endl;
    for (const auto &pos: symbol->referencePositions) {
        std::cout << "   * " << pos << std::endl;
    }

    // Print scope chain information
    std::cout << "Scope chain for '" << variableName << "':" << std::endl;
    auto scopeChain = scopeManager.getScopeChain();
    for (size_t i = 0; i < scopeChain.size(); ++i) {
        const auto &scope = scopeChain[i];
        std::cout << " - Scope level " << i << " (depth " << scope->getDepth() << "): ";

        switch (scope->getType()) {
            case Scope::ScopeType::GLOBAL:
                std::cout << "Global scope";
                break;
            case Scope::ScopeType::FUNCTION:
                std::cout << "Function scope";
                break;
            case Scope::ScopeType::BLOCK:
                std::cout << "Block scope";
                break;
        }

        // Check if this variable is declared in this scope
        if (scope->exists(variableName)) {
            std::cout << " (variable declared here)";
        }

        std::cout << std::endl;
    }

    // If captured, analyze closure requirements
    if (symbol->isCaptured) {
        std::cout << "Closure requirements:" << std::endl;

        // Get the nearest function scope that captures this variable
        auto functionScope = scopeManager.findNearestFunctionScope();
        if (functionScope) {
            auto capturedVars = functionScope->getCapturedVariables();
            std::cout << " - This variable is captured by a closure in function scope at depth "
                      << functionScope->getDepth() << std::endl;

            std::cout << " - Other variables captured by the same closure:" << std::endl;
            for (const auto &[name, depth]: capturedVars) {
                if (name != variableName) {
                    std::cout << "   * '" << name << "' from scope at depth " << depth << std::endl;
                }
            }
        }
    }
}
