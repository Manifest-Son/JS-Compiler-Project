#ifndef BENCHMARK_FRAMEWORK_H
#define BENCHMARK_FRAMEWORK_H

#include <chrono>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

/**
 * Framework for benchmarking various aspects of the JavaScript compiler
 */
class BenchmarkFramework {
public:
    /**
     * Benchmark result containing timing and memory information
     */
    struct BenchmarkResult {
        double time_ms; // Time in milliseconds
        size_t memory_used_bytes; // Memory usage in bytes
        size_t peak_memory_bytes; // Peak memory usage in bytes

        // Additional metrics
        size_t compiled_size_bytes; // Size of compiled output in bytes
        size_t optimization_time_ms; // Time spent in optimization in milliseconds
        size_t parsing_time_ms; // Time spent in parsing in milliseconds
        size_t codegen_time_ms; // Time spent in code generation in milliseconds

        BenchmarkResult() :
            time_ms(0), memory_used_bytes(0), peak_memory_bytes(0), compiled_size_bytes(0), optimization_time_ms(0),
            parsing_time_ms(0), codegen_time_ms(0) {}
    };

    /**
     * Base class for all benchmarks
     */
    class Benchmark {
    public:
        virtual ~Benchmark() = default;

        /**
         * Run the benchmark
         * @param iterations Number of iterations to run
         * @return Benchmark results
         */
        virtual BenchmarkResult run(int iterations) = 0;

        /**
         * Get the name of the benchmark
         * @return Benchmark name
         */
        virtual std::string getName() const = 0;

        /**
         * Get the description of the benchmark
         * @return Benchmark description
         */
        virtual std::string getDescription() const = 0;

    protected:
        /**
         * Get current process memory usage
         * @return Memory usage in bytes
         */
        size_t getCurrentMemoryUsage() const;

        /**
         * Reset all performance counters
         */
        void resetCounters();
    };

    /**
     * Register a benchmark
     * @param benchmark Benchmark to register
     */
    void registerBenchmark(std::shared_ptr<Benchmark> benchmark) { benchmarks_.push_back(benchmark); }

    /**
     * Run all registered benchmarks
     * @param iterations Number of iterations for each benchmark
     * @param outputFile Optional file to write results to in CSV format
     */
    void runAll(int iterations = 10, const std::string &outputFile = "") {
        std::ofstream csvFile;
        if (!outputFile.empty()) {
            csvFile.open(outputFile);
            if (csvFile.is_open()) {
                csvFile << "Benchmark,Time (ms),Memory (bytes),Peak Memory (bytes),"
                        << "Compiled Size (bytes),Optimization Time (ms),"
                        << "Parsing Time (ms),Codegen Time (ms)\n";
            }
        }

        std::cout << "Running " << benchmarks_.size() << " benchmarks with " << iterations << " iterations each...\n\n";

        for (const auto &benchmark: benchmarks_) {
            std::cout << "Running " << benchmark->getName() << "...\n";
            std::cout << "  " << benchmark->getDescription() << "\n";

            BenchmarkResult result = benchmark->run(iterations);

            std::cout << "  Time: " << result.time_ms << " ms\n";
            std::cout << "  Memory: " << (result.memory_used_bytes / 1024) << " KB\n";
            std::cout << "  Peak Memory: " << (result.peak_memory_bytes / 1024) << " KB\n";
            std::cout << "  Compiled Size: " << result.compiled_size_bytes << " bytes\n";
            std::cout << "  Optimization Time: " << result.optimization_time_ms << " ms\n";
            std::cout << "  Parsing Time: " << result.parsing_time_ms << " ms\n";
            std::cout << "  Codegen Time: " << result.codegen_time_ms << " ms\n\n";

            if (csvFile.is_open()) {
                csvFile << benchmark->getName() << "," << result.time_ms << "," << result.memory_used_bytes << ","
                        << result.peak_memory_bytes << "," << result.compiled_size_bytes << ","
                        << result.optimization_time_ms << "," << result.parsing_time_ms << "," << result.codegen_time_ms
                        << "\n";
            }
        }

        if (csvFile.is_open()) {
            csvFile.close();
            std::cout << "Benchmark results saved to " << outputFile << "\n";
        }
    }

private:
    std::vector<std::shared_ptr<Benchmark>> benchmarks_;
};

#endif // BENCHMARK_FRAMEWORK_H
