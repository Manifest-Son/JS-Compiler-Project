#include <iostream>
#include <chrono>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/ast_printer.h"
#include "../include/error_reporter.h"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#elif __linux__
#include <unistd.h>
#include <sys/resource.h>
#endif

// Function to get current memory usage in KB
size_t getCurrentMemoryUsage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / 1024;
    }
    return 0;
#elif __linux__
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
#else
    return 0; // Unsupported platform
#endif
}

// Helper to display parsing results or errors
void displayResults(const std::string& source_code, bool verbose = false) {
  // Record start time and memory
  auto start_time = std::chrono::high_resolution_clock::now();
  size_t memory_before = getCurrentMemoryUsage();

  // Lexical analysis
  Lexer lexer(source_code);
  std::vector<Token> tokens = lexer.tokenize();

  // Check for lexical errors
  if (lexer.getErrorReporter().hasErrors()) {
    std::cout << "\n===== Lexical Errors =====\n";
    lexer.getErrorReporter().displayErrors();
    std::cout << "===========================\n\n";
    return;
  }

  // Parsing
  try {
    Parser parser(tokens);
    std::shared_ptr<Program> program = parser.parse();

    // Print the AST
    ASTPrinter printer;
    std::string ast_output;
    ast_output = printer.print(program);

    std::cout << "\n===== Abstract Syntax Tree =====\n";
    std::cout << ast_output;
    std::cout << "================================\n\n";

  } catch (const ParserError& error) {
    std::cout << "\n===== Parser Error =====\n";
    std::cout << "Error: " << error.what() << std::endl;
    
    // Get line information from the error message and display context
    int errorLine = 0;
    if (error.line > 0) {
      errorLine = error.line;
      std::string context = lexer.getErrorReporter().getSourceLine(errorLine);
      if (!context.empty()) {
        std::cout << "Line " << errorLine << ": " << context << std::endl;
        std::cout << std::string(7 + error.column, ' ') << "^\n";
        
        if (!error.suggestion.empty()) {
          std::cout << "Suggestion: " << error.suggestion << std::endl;
        }
      }
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
    for (const Token& token : tokens) {
      std::cout << "Token Type: " << token.type << ", Lexeme: '" << token.lexeme << "', ";
      std::cout << "Value: ";
      std::visit([](const auto& v) {
        if constexpr (!std::is_same_v<std::decay_t<decltype(v)>, std::monostate>) {
          std::cout << v;
        } else {
          std::cout << "null";
        }
      }, token.value);

      std::cout << ", Line: " << token.line << std::endl;
    }
  }
}

int main(int argc, char* argv[]) {
  bool verbose = false;
  
  // Check for command line args
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-v" || arg == "--verbose") {
      verbose = true;
    }
  }

  const std::string source_code = R"(
    // This is a test function
    function add(a, b){
      return a + b;
    };
    let result = add(10, 20);
    print(result);

    // This code has an error - unterminated string
    let message = "Hello, world;
  )";

  displayResults(source_code, verbose);
  return 0;
}
