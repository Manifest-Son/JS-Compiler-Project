#include <chrono>
#include <iostream>
#include "../include/ast_printer.h"
#include "../include/error_reporter.h"
#include "../include/lexer.h"
#include "../include/parser.h"

#ifdef _WIN32
#include <psapi.h>
#include <windows.h>
#elif __linux__
#include <sys/resource.h>
#include <unistd.h>
#endif

// Function to get current memory usage in KB
size_t getCurrentMemoryUsage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&pmc), sizeof(pmc))) {
        return pmc.WorkingSetSize / 1024;
    }
    return 0;
#elif __linux__
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
#else
    return 0;
#endif
}

// Helper to display parsing results or errors
void displayResults(const std::string &source_code, bool verbose = false) {
    // Record start time and memory
    auto start_time = std::chrono::high_resolution_clock::now();
    size_t memory_before = getCurrentMemoryUsage();

    // Create error reporter
    ErrorReporter errorReporter;

    // Lexical analysis
    Lexer lexer(source_code, errorReporter);
    std::vector<Token> tokens = lexer.tokenize();

    // Check for lexical errors
    if (errorReporter.hasErrors()) {
        std::cout << "\n===== Lexical Errors =====\n";
        errorReporter.displayErrors();
        std::cout << "===========================\n\n";
        return;
    }

    // Parsing
    try {
        Parser parser(tokens, errorReporter);
        std::unique_ptr<Program> program = parser.parse();

        // Print the AST
        ASTPrinter printer;
        std::string ast_output = printer.print(program.get());

        std::cout << "\n===== Abstract Syntax Tree =====\n";
        std::cout << ast_output;
        std::cout << "================================\n\n";

    } catch (const std::runtime_error &error) {
        std::cout << "\n===== Parser Error =====\n";
        std::cout << "Error: " << error.what() << std::endl;

        // Display parser errors from the error reporter
        if (errorReporter.hasErrors()) {
            errorReporter.displayErrors();
        }

        std::cout << "=======================\n\n";
    }

    // Record end time and memory
    auto end_time = std::chrono::high_resolution_clock::now();
    size_t memory_after = getCurrentMemoryUsage();

    // Calculate and display execution time
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    std::cout << "\n===== Performance Metrics =====\n";
    std::cout << "Execution Time: " << duration << " microseconds" << std::endl;
    std::cout << "Memory Usage: " << (memory_after - memory_before) << " KB" << std::endl;
    std::cout << "==============================\n\n";

    if (verbose) {
        std::cout << "===========Tokens=============\n";
        for (const Token &token: tokens) {
            std::cout << "Token Type: " << token.type << ", Lexeme: '" << token.lexeme << "', ";
            std::cout << "Value: ";
            std::visit(
                    [](const auto &v) {
                        if constexpr (!std::is_same_v<std::decay_t<decltype(v)>, std::monostate>) {
                            std::cout << v;
                        } else {
                            std::cout << "null";
                        }
                    },
                    token.value);

            std::cout << ", Line: " << token.line << std::endl;
        }
    }
}

#include <cstdlib>
#include <fstream>
#include <string>
// #include "../include/compiler_driver.h"
#include "../include/version.h"

void printUsage(const char *progName) {
    std::cout << "JS Compiler " << VERSION_STRING << "\n"
              << "Usage: " << progName << " [options] input.js\n\n"
              << "Options:\n"
              << "  -o <file>     Write output to <file>\n"
              << "  -O<level>     Set optimization level (0-3)\n"
              << "  -emit-llvm    Output LLVM IR instead of executable\n"
              << "  -ast          Display the Abstract Syntax Tree (AST)\n"
              << "  -v, --version Show version information\n"
              << "  -h, --help    Show this help message\n";
}

std::string readFile(const std::string &path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Error: Cannot open file " << path << "\n";
        exit(1);
    }

    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string inputFile;
    std::string outputFile = "a.out";
    // int optLevel = 2;
    // bool emitLLVM = false;
    bool displayAst = false;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg.substr(0, 2) == "-O") {
            optLevel = std::stoi(arg.substr(2, 1));
        } else if (arg == "-emit-llvm") {
            emitLLVM = true;
        } else if (arg == "-ast") {
            displayAst = true;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "JS Compiler " << VERSION_STRING << "\n";
            return 0;
        } else if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        } else {
            inputFile = arg;
        }
    }

    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified\n";
        return 1;
    }

    // Read input file
    std::string source = readFile(inputFile);

    // Display the AST if requested
    if (displayAst) {
        displayResults(source, false);
        return 0;
    }

    // Initialize compiler
    // CompilerDriver compiler(optLevel);

    // Compile the source
    // if (!compiler.compile(source, inputFile)) {
        // std::cerr << compiler.getErrors();
        // return 1;
    }

    // Write output
    // if (emitLLVM) {
        // std::ofstream out(outputFile);
        // out << compiler.getIR();
        // std::cout << "LLVM IR written to " << outputFile << "\n";
    // } else {
        // Write executable or JIT execute
        // Implementation depends on backend capabilities
        // std::cout << "Compilation successful!\n";
    // }

    // return 0;
// }
