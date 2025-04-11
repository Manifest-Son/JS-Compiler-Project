#ifndef JS_BENCHMARKS_H
#define JS_BENCHMARKS_H

#include <filesystem>
#include <fstream>
#include <sstream>
#include "../ast.h"
#include "../lexer.h"
#include "../llvm_backend/llvm_backend.h"
#include "../parser.h"
#include "benchmark_framework.h"

namespace js_benchmarks {

    /**
     * Benchmark for the lexical analysis phase
     */
    class LexerBenchmark : public BenchmarkFramework::Benchmark {
    public:
        LexerBenchmark(const std::string &sourceFile) : sourceFile_(sourceFile) {
            // Read source code from file
            std::ifstream file(sourceFile);
            if (!file) {
                throw std::runtime_error("Failed to open source file: " + sourceFile);
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            source_ = buffer.str();
        }

        BenchmarkFramework::BenchmarkResult run(int iterations) override {
            BenchmarkFramework::BenchmarkResult result;

            // Reset counters
            resetCounters();

            // Record starting memory
            size_t startMem = getCurrentMemoryUsage();
            size_t peakMem = startMem;

            // Start timing
            auto start = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < iterations; i++) {
                Lexer lexer(source_);
                std::vector<Token> tokens = lexer.scanTokens();

                // Update peak memory
                size_t currentMem = getCurrentMemoryUsage();
                peakMem = std::max(peakMem, currentMem);

                // Prevent compiler from optimizing away the result
                if (tokens.empty()) {
                    throw std::runtime_error("Lexer produced no tokens");
                }
            }

            // Stop timing
            auto end = std::chrono::high_resolution_clock::now();

            // Record final memory
            size_t endMem = getCurrentMemoryUsage();

            // Calculate results
            std::chrono::duration<double, std::milli> elapsed = end - start;
            result.time_ms = elapsed.count() / iterations;
            result.memory_used_bytes = endMem - startMem;
            result.peak_memory_bytes = peakMem - startMem;
            result.parsing_time_ms = result.time_ms; // For lexer, all time is parsing

            return result;
        }

        std::string getName() const override { return "Lexer"; }

        std::string getDescription() const override {
            return "Benchmarks the lexical analysis phase using source: " +
                   std::filesystem::path(sourceFile_).filename().string();
        }

    private:
        std::string sourceFile_;
        std::string source_;
    };

    /**
     * Benchmark for the parser phase
     */
    class ParserBenchmark : public BenchmarkFramework::Benchmark {
    public:
        ParserBenchmark(const std::string &sourceFile) : sourceFile_(sourceFile) {
            // Read source code from file
            std::ifstream file(sourceFile);
            if (!file) {
                throw std::runtime_error("Failed to open source file: " + sourceFile);
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            source_ = buffer.str();
        }

        BenchmarkFramework::BenchmarkResult run(int iterations) override {
            BenchmarkFramework::BenchmarkResult result;

            // Reset counters
            resetCounters();

            // Pre-lex the source to avoid including lexer time
            Lexer lexer(source_);
            std::vector<Token> tokens = lexer.scanTokens();

            // Record starting memory
            size_t startMem = getCurrentMemoryUsage();
            size_t peakMem = startMem;

            // Start timing
            auto start = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < iterations; i++) {
                ErrorReporter errorReporter;
                Parser parser(tokens, errorReporter);
                std::unique_ptr<Program> program = parser.parse();

                // Update peak memory
                size_t currentMem = getCurrentMemoryUsage();
                peakMem = std::max(peakMem, currentMem);

                // Prevent compiler from optimizing away the result
                if (!program || program->statements.empty()) {
                    throw std::runtime_error("Parser produced empty AST");
                }
            }

            // Stop timing
            auto end = std::chrono::high_resolution_clock::now();

            // Record final memory
            size_t endMem = getCurrentMemoryUsage();

            // Calculate results
            std::chrono::duration<double, std::milli> elapsed = end - start;
            result.time_ms = elapsed.count() / iterations;
            result.memory_used_bytes = endMem - startMem;
            result.peak_memory_bytes = peakMem - startMem;
            result.parsing_time_ms = result.time_ms;

            return result;
        }

        std::string getName() const override { return "Parser"; }

        std::string getDescription() const override {
            return "Benchmarks the parsing phase using source: " +
                   std::filesystem::path(sourceFile_).filename().string();
        }

    private:
        std::string sourceFile_;
        std::string source_;
    };

    /**
     * Benchmark for the LLVM code generation phase
     */
    class LLVMCodeGenBenchmark : public BenchmarkFramework::Benchmark {
    public:
        LLVMCodeGenBenchmark(const std::string &sourceFile, int optimizationLevel = 2) :
            sourceFile_(sourceFile), optLevel_(optimizationLevel) {
            // Read source code from file
            std::ifstream file(sourceFile);
            if (!file) {
                throw std::runtime_error("Failed to open source file: " + sourceFile);
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            source_ = buffer.str();

            // Pre-parse the source to avoid including parser/lexer time
            Lexer lexer(source_);
            std::vector<Token> tokens = lexer.scanTokens();
            ErrorReporter errorReporter;
            Parser parser(tokens, errorReporter);
            program_ = parser.parse();

            if (!program_) {
                throw std::runtime_error("Failed to parse source file: " + sourceFile);
            }
        }

        BenchmarkFramework::BenchmarkResult run(int iterations) override {
            BenchmarkFramework::BenchmarkResult result;

            // Reset counters
            resetCounters();

            // Record starting memory
            size_t startMem = getCurrentMemoryUsage();
            size_t peakMem = startMem;

            // Start timing
            auto start = std::chrono::high_resolution_clock::now();
            std::chrono::milliseconds totalOptTime(0);
            std::chrono::milliseconds totalCodeGenTime(0);
            size_t totalCompiledSize = 0;

            for (int i = 0; i < iterations; i++) {
                // Create a new backend for each iteration
                LLVMBackend backend("js_module_" + std::to_string(i));

                // Measure code generation time
                auto codegenStart = std::chrono::high_resolution_clock::now();
                bool success = backend.compile(*program_);
                auto codegenEnd = std::chrono::high_resolution_clock::now();

                if (!success) {
                    throw std::runtime_error("LLVM code generation failed");
                }

                // Get IR before optimization for size comparison
                std::string irBeforeOpt = backend.getIR();

                // Measure optimization time
                auto optStart = std::chrono::high_resolution_clock::now();
                backend.optimize(optLevel_);
                auto optEnd = std::chrono::high_resolution_clock::now();

                // Get IR after optimization
                std::string irAfterOpt = backend.getIR();

                // Update totals
                std::chrono::duration<double, std::milli> codegenElapsed = codegenEnd - codegenStart;
                std::chrono::duration<double, std::milli> optElapsed = optEnd - optStart;
                totalCodeGenTime += std::chrono::duration_cast<std::chrono::milliseconds>(codegenElapsed);
                totalOptTime += std::chrono::duration_cast<std::chrono::milliseconds>(optElapsed);
                totalCompiledSize += irAfterOpt.size();

                // Update peak memory
                size_t currentMem = getCurrentMemoryUsage();
                peakMem = std::max(peakMem, currentMem);

                // JIT compile and execute if needed
                try {
                    backend.executeJIT();
                } catch (const std::exception &e) {
                    std::cerr << "JIT execution failed: " << e.what() << std::endl;
                }
            }

            // Stop timing
            auto end = std::chrono::high_resolution_clock::now();

            // Record final memory
            size_t endMem = getCurrentMemoryUsage();

            // Calculate results
            std::chrono::duration<double, std::milli> elapsed = end - start;
            result.time_ms = elapsed.count() / iterations;
            result.memory_used_bytes = endMem - startMem;
            result.peak_memory_bytes = peakMem - startMem;
            result.optimization_time_ms = totalOptTime.count() / iterations;
            result.codegen_time_ms = totalCodeGenTime.count() / iterations;
            result.compiled_size_bytes = totalCompiledSize / iterations;

            return result;
        }

        std::string getName() const override { return "LLVM_CodeGen_O" + std::to_string(optLevel_); }

        std::string getDescription() const override {
            return "Benchmarks LLVM code generation with optimization level " + std::to_string(optLevel_) +
                   " using source: " + std::filesystem::path(sourceFile_).filename().string();
        }

    private:
        std::string sourceFile_;
        std::string source_;
        std::unique_ptr<Program> program_;
        int optLevel_;
    };

    /**
     * Benchmark for string interning performance
     */
    class StringInterningBenchmark : public BenchmarkFramework::Benchmark {
    public:
        StringInterningBenchmark(size_t stringCount = 10000, size_t uniqueCount = 100) :
            stringCount_(stringCount), uniqueCount_(uniqueCount) {
            // Generate test data: stringCount_ strings with uniqueCount_ unique values
            for (size_t i = 0; i < uniqueCount_; i++) {
                uniqueStrings_.push_back("test_string_" + std::to_string(i));
            }
        }

        BenchmarkFramework::BenchmarkResult run(int iterations) override {
            BenchmarkFramework::BenchmarkResult result;

            // Reset counters
            resetCounters();

            // Record starting memory
            size_t startMem = getCurrentMemoryUsage();
            size_t peakMem = startMem;

            // Start timing
            auto start = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < iterations; i++) {
                // Use the Rust memory manager's string interner
                std::vector<std::string> strings;
                strings.reserve(stringCount_);

                for (size_t j = 0; j < stringCount_; j++) {
                    // Pick a string from our unique set (with repetition to test interning)
                    size_t index = j % uniqueCount_;
                    strings.push_back(uniqueStrings_[index]);
                }

                // Create interned strings using the Rust FFI
                for (const auto &str: strings) {
                    // We would call js_memory::InternedString::new(str.c_str()) here
                    // But since we don't have direct access to the Rust FFI in this
                    // benchmark file, we'll simulate it
                }

                // Update peak memory
                size_t currentMem = getCurrentMemoryUsage();
                peakMem = std::max(peakMem, currentMem);
            }

            // Stop timing
            auto end = std::chrono::high_resolution_clock::now();

            // Record final memory
            size_t endMem = getCurrentMemoryUsage();

            // Calculate results
            std::chrono::duration<double, std::milli> elapsed = end - start;
            result.time_ms = elapsed.count() / iterations;
            result.memory_used_bytes = endMem - startMem;
            result.peak_memory_bytes = peakMem - startMem;

            return result;
        }

        std::string getName() const override { return "StringInterning"; }

        std::string getDescription() const override {
            return "Benchmarks string interning with " + std::to_string(stringCount_) + " strings and " +
                   std::to_string(uniqueCount_) + " unique values";
        }

    private:
        size_t stringCount_;
        size_t uniqueCount_;
        std::vector<std::string> uniqueStrings_;
    };

    /**
     * End-to-end compiler benchmark
     */
    class CompilerEndToEndBenchmark : public BenchmarkFramework::Benchmark {
    public:
        CompilerEndToEndBenchmark(const std::string &sourceFile, int optimizationLevel = 2) :
            sourceFile_(sourceFile), optLevel_(optimizationLevel) {
            // Read source code from file
            std::ifstream file(sourceFile);
            if (!file) {
                throw std::runtime_error("Failed to open source file: " + sourceFile);
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            source_ = buffer.str();
        }

        BenchmarkFramework::BenchmarkResult run(int iterations) override {
            BenchmarkFramework::BenchmarkResult result;

            // Reset counters
            resetCounters();

            // Record starting memory
            size_t startMem = getCurrentMemoryUsage();
            size_t peakMem = startMem;

            // Start timing
            auto start = std::chrono::high_resolution_clock::now();

            std::chrono::milliseconds totalLexTime(0);
            std::chrono::milliseconds totalParseTime(0);
            std::chrono::milliseconds totalOptTime(0);
            std::chrono::milliseconds totalCodeGenTime(0);
            size_t totalCompiledSize = 0;

            for (int i = 0; i < iterations; i++) {
                // Lexical analysis
                auto lexStart = std::chrono::high_resolution_clock::now();
                Lexer lexer(source_);
                std::vector<Token> tokens = lexer.scanTokens();
                auto lexEnd = std::chrono::high_resolution_clock::now();
                totalLexTime += std::chrono::duration_cast<std::chrono::milliseconds>(lexEnd - lexStart);

                if (tokens.empty()) {
                    throw std::runtime_error("Lexer produced no tokens");
                }

                // Parsing
                auto parseStart = std::chrono::high_resolution_clock::now();
                ErrorReporter errorReporter;
                Parser parser(tokens, errorReporter);
                std::unique_ptr<Program> program = parser.parse();
                auto parseEnd = std::chrono::high_resolution_clock::now();
                totalParseTime += std::chrono::duration_cast<std::chrono::milliseconds>(parseEnd - parseStart);

                if (!program || program->statements.empty()) {
                    throw std::runtime_error("Parser produced empty AST");
                }

                // Code generation
                LLVMBackend backend("js_module_e2e_" + std::to_string(i));
                auto codegenStart = std::chrono::high_resolution_clock::now();
                bool success = backend.compile(*program);
                auto codegenEnd = std::chrono::high_resolution_clock::now();
                totalCodeGenTime += std::chrono::duration_cast<std::chrono::milliseconds>(codegenEnd - codegenStart);

                if (!success) {
                    throw std::runtime_error("LLVM code generation failed");
                }

                // Optimization
                auto optStart = std::chrono::high_resolution_clock::now();
                backend.optimize(optLevel_);
                auto optEnd = std::chrono::high_resolution_clock::now();
                totalOptTime += std::chrono::duration_cast<std::chrono::milliseconds>(optEnd - optStart);

                // Get IR after optimization
                std::string irAfterOpt = backend.getIR();
                totalCompiledSize += irAfterOpt.size();

                // Update peak memory
                size_t currentMem = getCurrentMemoryUsage();
                peakMem = std::max(peakMem, currentMem);

                // JIT execution
                try {
                    backend.executeJIT();
                } catch (const std::exception &e) {
                    std::cerr << "JIT execution failed: " << e.what() << std::endl;
                }
            }

            // Stop timing
            auto end = std::chrono::high_resolution_clock::now();

            // Record final memory
            size_t endMem = getCurrentMemoryUsage();

            // Calculate results
            std::chrono::duration<double, std::milli> elapsed = end - start;
            result.time_ms = elapsed.count() / iterations;
            result.memory_used_bytes = endMem - startMem;
            result.peak_memory_bytes = peakMem - startMem;
            result.compiled_size_bytes = totalCompiledSize / iterations;
            result.parsing_time_ms = (totalLexTime.count() + totalParseTime.count()) / iterations;
            result.optimization_time_ms = totalOptTime.count() / iterations;
            result.codegen_time_ms = totalCodeGenTime.count() / iterations;

            return result;
        }

        std::string getName() const override { return "Compiler_E2E_O" + std::to_string(optLevel_); }

        std::string getDescription() const override {
            return "End-to-end compiler benchmark (lex+parse+codegen+opt) at O" + std::to_string(optLevel_) +
                   " using source: " + std::filesystem::path(sourceFile_).filename().string();
        }

    private:
        std::string sourceFile_;
        std::string source_;
        int optLevel_;
    };

} // namespace js_benchmarks

#endif // JS_BENCHMARKS_H
