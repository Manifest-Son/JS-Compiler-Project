#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "ast.h"
#include "ast_visitor.h"
#include "lexer.h"
#include "parser.h"

// Platform-specific includes for memory monitoring
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <sys/resource.h>
#else
// Add support for other platforms (e.g., macOS)
#include <mach/mach.h>
#endif

// Custom visitor to calculate AST statistics
class ASTStatsVisitor : public ASTVisitor {
public:
    ASTStatsVisitor() : nodeCount(0), maxDepth(0), currentDepth(0) {}

    void visitProgram(const Program &program) override {
        for (const auto &stmt : program.statements) {
            if (stmt) {
                currentDepth = 1;
                stmt->accept(*this);
            }
        }
    }
    
    // Implement all required ExprVisitor methods
    void visitLiteralExpr(const LiteralExpr &expr) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        currentDepth--;
    }
    
    void visitVariableExpr(const VariableExpr &expr) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        currentDepth--;
    }
    
    void visitBinaryExpr(const BinaryExpr &expr) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        visitIfNotNull(expr.left);
        visitIfNotNull(expr.right);
        currentDepth--;
    }
    
    void visitUnaryExpr(const UnaryExpr &expr) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        visitIfNotNull(expr.right);
        currentDepth--;
    }
    
    void visitCallExpr(const CallExpr &expr) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        visitIfNotNull(expr.callee);
        for (const auto &arg : expr.arguments) {
            visitIfNotNull(arg);
        }
        currentDepth--;
    }
    
    void visitGetExpr(const GetExpr &expr) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        visitIfNotNull(expr.object);
        currentDepth--;
    }
    
    void visitArrayExpr(const ArrayExpr &expr) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        for (const auto &element : expr.elements) {
            visitIfNotNull(element);
        }
        currentDepth--;
    }
    
    void visitObjectExpr(const ObjectExpr &expr) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        for (const auto &property : expr.properties) {
            visitIfNotNull(property.value);
        }
        currentDepth--;
    }
    
    void visitArrowFunctionExpr(const ArrowFunctionExpr &expr) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        visitIfNotNull(expr.body);
        currentDepth--;
    }
    
    // Implement all required StmtVisitor methods
    void visitExpressionStmt(const ExpressionStmt &stmt) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        visitIfNotNull(stmt.expression);
        currentDepth--;
    }
    
    void visitVarDeclStmt(const VarDeclStmt &stmt) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        visitIfNotNull(stmt.initializer);
        currentDepth--;
    }
    
    void visitBlockStmt(const BlockStmt &stmt) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        for (const auto &s : stmt.statements) {
            visitIfNotNull(s);
        }
        currentDepth--;
    }
    
    void visitIfStmt(const IfStmt &stmt) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        visitIfNotNull(stmt.condition);
        visitIfNotNull(stmt.thenBranch);
        visitIfNotNull(stmt.elseBranch);
        currentDepth--;
    }
    
    void visitWhileStmt(const WhileStmt &stmt) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        visitIfNotNull(stmt.condition);
        visitIfNotNull(stmt.body);
        currentDepth--;
    }
    
    void visitForStmt(const ForStmt &stmt) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        visitIfNotNull(stmt.initializer);
        visitIfNotNull(stmt.condition);
        visitIfNotNull(stmt.increment);
        visitIfNotNull(stmt.body);
        currentDepth--;
    }
    
    void visitFunctionDeclStmt(const FunctionDeclStmt &stmt) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        // if (stmt.body.data()) {
        //     stmt.body->accept(*this);
        // }
        currentDepth--;
    }

    void visitReturnStmt(const ReturnStmt &stmt) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        visitIfNotNull(stmt.value);
        currentDepth--;
    }

    void visitBreakStmt(const BreakStmt &stmt) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        currentDepth--;
    }

    void visitContinueStmt(const ContinueStmt &stmt) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        currentDepth--;
    }

    void visitClassDeclStmt(const ClassDeclStmt &stmt) override {
        nodeCount++;
        currentDepth++;
        updateMaxDepth();
        // Visit class methods if needed
        currentDepth--;
    }

    int getNodeCount() const { return nodeCount; }
    int getMaxDepth() const { return maxDepth; }

private:
    int nodeCount;
    int maxDepth;
    int currentDepth;

    void updateMaxDepth() {
        if (currentDepth > maxDepth) {
            maxDepth = currentDepth;
        }
    }

    template<typename T>
    void visitIfNotNull(const std::shared_ptr<T> &node) {
        if (node) {
            node->accept(*this);
        }
    }
};

struct BenchmarkResult {
    size_t inputSize;        // Size of input in bytes
    size_t tokenCount;       // Number of tokens
    double tokenizationTime; // Time in milliseconds for lexical analysis
    double parsingTime;      // Time in milliseconds for parsing
    double totalTime;        // Total processing time
    size_t tokensPerSecond;  // Parsing speed - tokens/sec
    size_t memoryUsage;      // Memory usage in KB
    int astNodeCount;        // Total number of AST nodes
    int astMaxDepth;         // Maximum depth of AST
    bool success;            // Whether parsing completed successfully
    std::string errorMessage; // Error message if parsing failed

    // Helper method to generate CSV row
    std::string toCSV() const {
        std::stringstream ss;
        ss << inputSize << ","
           << tokenCount << ","
           << tokenizationTime << ","
           << parsingTime << ","
           << totalTime << ","
           << tokensPerSecond << ","
           << memoryUsage << ","
           << astNodeCount << ","
           << astMaxDepth << ","
           << (success ? "success" : "failure") << ","
           << "\"" << errorMessage << "\"";
        return ss.str();
    }
};

class Benchmark {
public:
    // Run benchmark on a single input
    BenchmarkResult runBenchmark(const std::string &sourceCode) {
        BenchmarkResult result = {};
        result.inputSize = sourceCode.size();
        result.success = true;

        // Record starting memory
        size_t memoryBefore = getCurrentMemoryUsage();

        // Tokenization phase
        auto tokenStart = std::chrono::high_resolution_clock::now();
        Lexer lexer(sourceCode);
        std::vector<Token> tokens;

        try {
            tokens = lexer.tokenize();
            result.tokenCount = tokens.size();

            // Check for lexical errors
            if (lexer.getErrorReporter().hasErrors()) {
                result.success = false;
                result.errorMessage = "Lexical error: ";
                // result.errorMessage.append(lexer.getErrorReporter().getErrorMessages());
                // result.errorMessage = std::string("Lexical error: ").append(lexer.getErrorReporter().getErrorMessages());
                // result.errorMessage = "Lexical error: " + (lexer.getErrorReporter().getErrorMessages());
            }
        } catch (const std::exception &e) {
            result.success = false;
            result.errorMessage = "Lexer exception: " + std::string(e.what());
        }

        auto tokenEnd = std::chrono::high_resolution_clock::now();
        result.tokenizationTime = std::chrono::duration<double, std::milli>(tokenEnd - tokenStart).count();

        // Parsing phase
        auto parseStart = std::chrono::high_resolution_clock::now();

        if (result.success) {
            try {
                Parser parser(tokens);
                std::shared_ptr<Program> program = parser.parse();

                // Calculate AST statistics
                ASTStatsVisitor statsVisitor;
                program->accept(statsVisitor);
                result.astNodeCount = statsVisitor.getNodeCount();
                result.astMaxDepth = statsVisitor.getMaxDepth();
            } catch (const std::exception &e) {
                result.success = false;
                result.errorMessage = "Parser exception: " + std::string(e.what());
            }
        }

        auto parseEnd = std::chrono::high_resolution_clock::now();
        result.parsingTime = std::chrono::duration<double, std::milli>(parseEnd - parseStart).count();

        // Calculate total time
        result.totalTime = result.tokenizationTime + result.parsingTime;

        // Calculate tokens per second for tokenization phase
        if (result.tokenizationTime > 0) {
            result.tokensPerSecond = static_cast<size_t>((result.tokenCount * 1000.0) / result.tokenizationTime);
        }

        // Calculate memory usage
        size_t memoryAfter = getCurrentMemoryUsage();
        result.memoryUsage = memoryAfter > memoryBefore ? memoryAfter - memoryBefore : 0;

        return result;
    }

    // Run benchmarks on a series of inputs with different sizes
    std::vector<BenchmarkResult> runSizeBenchmarks(const std::vector<size_t>& customSizes = {}) {
        std::vector<BenchmarkResult> results;

        // Define different input sizes for benchmarking, or use custom sizes if provided
        std::vector<size_t> sizes = customSizes.empty() ?
            std::vector<size_t>{10, 100, 1000, 10000, 100000} : customSizes;

        for (size_t size : sizes) {
            std::string input = generateTestInput(size);
            BenchmarkResult result = runBenchmark(input);
            results.push_back(result);
        }

        return results;
    }

    // Run benchmarks with specific test cases
    std::vector<BenchmarkResult> runTestCaseBenchmarks(const std::vector<std::string>& testCases) {
        std::vector<BenchmarkResult> results;

        for (const auto& testCase : testCases) {
            BenchmarkResult result = runBenchmark(testCase);
            results.push_back(result);
        }

        return results;
    }

    // Save benchmark results to CSV file
    void saveResultsToCSV(const std::vector<BenchmarkResult> &results, const std::string &filename) {
        std::ofstream file(filename);

        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + filename);
        }

        // Write header
        file << "Input Size (B),Token Count,Tokenization Time (ms),Parsing Time (ms),Total Time (ms),"
             << "Tokens Per Second,Memory Usage (KB),AST Node Count,AST Max Depth,Status,Error Message" << std::endl;

        // Write data rows
        for (const auto &result : results) {
            file << result.toCSV() << std::endl;
        }
    }

private:
    // Helper function to get current memory usage (platform-specific)
    size_t getCurrentMemoryUsage() {
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(::GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*) &pmc, sizeof(pmc))) {
            return pmc.WorkingSetSize / 1024;
        }
        return 0;
#elif defined(__linux__)
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        return usage.ru_maxrss;
#elif defined(__APPLE__)
        // macOS implementation
        task_basic_info_data_t info;
        mach_msg_type_number_t size = TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size) == KERN_SUCCESS) {
            return info.resident_size / 1024;
        }
        return 0;
#else
        return 0; // Unsupported platform
#endif
    }

    // Generate test JavaScript code of approximately the given size
    std::string generateTestInput(size_t targetSize) {
        // Base code snippet
        static const std::string baseCode = R"(
function add(a, b) {
    return a + b;
}

function subtract(a, b) {
    return a - b;
}

function multiply(a, b) {
    return a * b;
}

function divide(a, b) {
    if (b === 0) {
        throw new Error("Division by zero");
    }
    return a / b;
}

let x = 10;
let y = 20;
let result = 0;

// Perform calculations
result = add(x, y);
console.log("Addition result: " + result);

result = subtract(x, y);
console.log("Subtraction result: " + result);

result = multiply(x, y);
console.log("Multiplication result: " + result);

result = divide(x, y);
console.log("Division result: " + result);

// Conditional logic
if (x > 5) {
    console.log("x is greater than 5");
} else {
    console.log("x is not greater than 5");
}

// Loop example
for (let i = 0; i < 5; i++) {
    console.log("Loop iteration: " + i);
}
)";

        // If baseCode is already larger than targetSize, return a substring
        if (baseCode.size() >= targetSize) {
            return baseCode.substr(0, targetSize);
        }

        // Calculate how many times to repeat the base code
        size_t repeatCount = (targetSize / baseCode.size()) + 1;

        // Pre-allocate memory for the result string to avoid reallocations
        std::string result;
        result.reserve(targetSize + 100); // Add buffer for safety

        for (size_t i = 0; i < repeatCount; i++) {
            // Add function with unique name to avoid redefinition errors
            result.append("// Code block " + std::to_string(i) + "\n");
            result.append("function calculate" + std::to_string(i) + "(a, b) {\n");
            result.append("    return add(multiply(a, b), subtract(a, b));\n");
            result.append("}\n");
            result.append("result = calculate" + std::to_string(i) + "(x, y);\n");
            result.append("console.log(\"Result of calculation " + std::to_string(i) + ": \" + result);\n\n");

            // Add the base code if we need more content
            if (result.size() < targetSize) {
                result.append(baseCode);
            }

            // Stop if we've reached the target size
            if (result.size() >= targetSize) {
                break;
            }
        }

        // Trim to the target size if necessary
        if (result.size() > targetSize) {
            result.resize(targetSize);
        }

        return result;
    }
};

#endif // BENCHMARK_H
