#include <ctime>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include "../../include/benchmark/js_benchmarks.h"

int main(int argc, char *argv[]) {
    // Parse command line arguments
    std::string outputFile;
    int iterations = 5;
    std::vector<std::string> testFiles;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg == "-i" && i + 1 < argc) {
            iterations = std::stoi(argv[++i]);
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options] [test_files...]\n"
                      << "Options:\n"
                      << "  -o <file>      Output results to CSV file\n"
                      << "  -i <number>    Number of iterations (default: 5)\n"
                      << "  -h, --help     Show this help message\n"
                      << "\nIf no test files are provided, the default examples will be used.\n";
            return 0;
        } else {
            // Assume it's a test file
            testFiles.push_back(arg);
        }
    }

    // Use default test files if none provided
    if (testFiles.empty()) {
        testFiles.push_back("examples/dataflow_test.js");
        // Add more default test files as needed
    }

    // Setup benchmark framework
    BenchmarkFramework framework;

    // Register benchmarks for each test file
    for (const auto &file: testFiles) {
        try {
            // Lexer benchmark
            framework.registerBenchmark(std::make_shared<js_benchmarks::LexerBenchmark>(file));

            // Parser benchmark
            framework.registerBenchmark(std::make_shared<js_benchmarks::ParserBenchmark>(file));

            // LLVM CodeGen benchmarks with different optimization levels
            for (int optLevel = 0; optLevel <= 3; ++optLevel) {
                framework.registerBenchmark(std::make_shared<js_benchmarks::LLVMCodeGenBenchmark>(file, optLevel));
            }

            // End-to-end compiler benchmark with default optimization level
            framework.registerBenchmark(std::make_shared<js_benchmarks::CompilerEndToEndBenchmark>(file));
        } catch (const std::exception &e) {
            std::cerr << "Error setting up benchmark for file " << file << ": " << e.what() << std::endl;
        }
    }

    // Register string interning benchmark
    framework.registerBenchmark(std::make_shared<js_benchmarks::StringInterningBenchmark>());

    // Generate timestamped output filename if not provided
    if (outputFile.empty()) {
        std::time_t now = std::time(nullptr);
        std::tm tm = *std::localtime(&now);
        std::ostringstream oss;
        oss << "benchmark_results_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".csv";
        outputFile = oss.str();
    }

    // Run all benchmarks
    std::cout << "JS Compiler Benchmark Runner\n"
              << "===========================\n\n";
    framework.runAll(iterations, outputFile);

    return 0;
}
