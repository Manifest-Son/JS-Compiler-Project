#include "../../include/ast.h"
#include "../../include/error_reporter.h"
#include "../../include/llvm_backend/js_runtime.h"
#include "../../include/llvm_backend/llvm_backend.h"
#include "../../include/parser.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

/**
 * LLVMCodeGenerator - A class that handles compiling JavaScript code to LLVM IR
 * and optionally to native code.
 */
class LLVMCodeGenerator {
public:
    /**
     * Constructor initializes the code generator.
     * @param reporter An error reporter instance for reporting compilation errors
     */
    LLVMCodeGenerator(ErrorReporter &reporter) : reporter(reporter) {}

    /**
     * Compile a JavaScript source file to LLVM IR.
     * @param source The JavaScript source code to compile
     * @param moduleName The name to give the LLVM module
     * @param optimizationLevel The optimization level to apply (0-3)
     * @return The generated LLVM IR as a string, or empty string on error
     */
    std::string compileToLLVMIR(const std::string &source, const std::string &moduleName = "js_module",
                                int optimizationLevel = 0) {
        // Parse the JavaScript source to AST
        Parser parser(source, reporter);
        std::unique_ptr<Program> program = parser.parse();

        if (reporter.hadError()) {
            std::cerr << "Failed to parse JavaScript source." << std::endl;
            return "";
        }

        // Create the LLVM backend
        LLVMBackend backend(moduleName);

        // Compile the AST to LLVM IR
        if (!backend.compile(*program)) {
            std::cerr << "Failed to compile AST to LLVM IR." << std::endl;
            return "";
        }

        // Apply optimizations if requested
        if (optimizationLevel > 0) {
            backend.optimize(optimizationLevel);
        }

        // Return the generated LLVM IR
        return backend.getIR();
    }

    /**
     * Compile a JavaScript source file to LLVM IR and save it to a file.
     * @param source The JavaScript source code to compile
     * @param outputPath The path to write the LLVM IR to
     * @param moduleName The name to give the LLVM module
     * @param optimizationLevel The optimization level to apply (0-3)
     * @return True if compilation was successful, false otherwise
     */
    bool compileToLLVMIRFile(const std::string &source, const std::string &outputPath,
                             const std::string &moduleName = "js_module", int optimizationLevel = 0) {
        // Parse the JavaScript source to AST
        Parser parser(source, reporter);
        std::unique_ptr<Program> program = parser.parse();

        if (reporter.hadError()) {
            std::cerr << "Failed to parse JavaScript source." << std::endl;
            return false;
        }

        // Create the LLVM backend
        LLVMBackend backend(moduleName);

        // Compile the AST to LLVM IR
        if (!backend.compile(*program)) {
            std::cerr << "Failed to compile AST to LLVM IR." << std::endl;
            return false;
        }

        // Apply optimizations if requested
        if (optimizationLevel > 0) {
            backend.optimize(optimizationLevel);
        }

        // Write the IR to the output file
        return backend.writeIR(outputPath);
    }

    /**
     * Compile a JavaScript source file from a file path to LLVM IR.
     * @param inputPath The path to the JavaScript source file
     * @param outputPath The path to write the LLVM IR to
     * @param moduleName The name to give the LLVM module
     * @param optimizationLevel The optimization level to apply (0-3)
     * @return True if compilation was successful, false otherwise
     */
    bool compileFileToLLVMIR(const std::string &inputPath, const std::string &outputPath,
                             const std::string &moduleName = "js_module", int optimizationLevel = 0) {
        // Read the input file
        std::ifstream inputFile(inputPath);
        if (!inputFile.is_open()) {
            std::cerr << "Failed to open input file: " << inputPath << std::endl;
            return false;
        }

        // Read the entire file into a string
        std::string source((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
        inputFile.close();

        // Compile the source to LLVM IR
        return compileToLLVMIRFile(source, outputPath, moduleName, optimizationLevel);
    }

private:
    ErrorReporter &reporter;
};

// Example usage of the LLVMCodeGenerator
#ifdef LLVM_CODEGEN_MAIN
int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input.js> <output.ll> [optimization_level]" << std::endl;
        return 1;
    }

    std::string inputPath = argv[1];
    std::string outputPath = argv[2];
    int optLevel = (argc >= 4) ? std::stoi(argv[3]) : 0;

    // Initialize the JavaScript runtime
    JSRuntime::initialize();

    // Create an error reporter
    ErrorReporter reporter;

    // Create the code generator
    LLVMCodeGenerator codegen(reporter);

    // Compile the file
    bool success = codegen.compileFileToLLVMIR(inputPath, outputPath, "js_module", optLevel);

    // Shutdown the JavaScript runtime
    JSRuntime::shutdown();

    return success ? 0 : 1;
}
#endif // LLVM_CODEGEN_MAIN
