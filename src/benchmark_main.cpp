#include <filesystem>
#include <iomanip>
#include <iostream>
#include "../include/benchmark.h"

void printResult(const BenchmarkResult &result) {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Input Size:         " << std::setw(8) << result.inputSize << " bytes" << std::endl;
    std::cout << "Token Count:        " << std::setw(8) << result.tokenCount << std::endl;
    std::cout << "Tokenization Time:  " << std::setw(8) << std::fixed << std::setprecision(2) << result.tokenizationTime
              << " ms" << std::endl;
    std::cout << "Parsing Time:       " << std::setw(8) << std::fixed << std::setprecision(2) << result.parsingTime
              << " ms" << std::endl;
    std::cout << "Total Time:         " << std::setw(8) << std::fixed << std::setprecision(2) << result.totalTime
              << " ms" << std::endl;
    std::cout << "Tokens Per Second:  " << std::setw(8) << result.tokensPerSecond << std::endl;
    std::cout << "Memory Usage:       " << std::setw(8) << result.memoryUsage << " KB" << std::endl;
    std::cout << "AST Node Count:     " << std::setw(8) << result.astNodeCount << std::endl;
    std::cout << "AST Max Depth:      " << std::setw(8) << result.astMaxDepth << std::endl;
    std::cout << "Status:             " << (result.success ? "Success" : "Failure") << std::endl;
    if (!result.errorMessage.empty()) {
        std::cout << "Error Message:      " << result.errorMessage << std::endl;
    }
}

void runAndPrintSizeBenchmark() {
    std::cout << "Running benchmarks with different input sizes..." << std::endl;

    Benchmark benchmark;
    auto results = benchmark.runSizeBenchmarks();

    // Print results to console
    for (const auto &result: results) {
        printResult(result);
    }

    // Create results directory if it doesn't exist
    std::filesystem::create_directories("benchmark_results");

    // Save results to CSV file with timestamp
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    localtime_s(&tm_now, &now_time_t);

    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &tm_now);

    std::string filename = "benchmark_results/size_benchmark_" + std::string(buffer) + ".csv";
    benchmark.saveResultsToCSV(results, filename);

    std::cout << "Results saved to: " << filename << std::endl;
}

void runComplexityBenchmark() {
    std::cout << "Running complexity benchmark..." << std::endl;

    Benchmark benchmark;
    std::vector<BenchmarkResult> results;

    // Generate deeply nested code for complexity testing
    std::vector<int> nesting_depths = {5, 10, 15, 20, 25};

    for (int depth: nesting_depths) {
        std::string nestedCode = "function complexityTest() {\n";

        // Add deeply nested if statements
        std::string indent = "    ";
        for (int i = 0; i < depth; i++) {
            nestedCode += indent + "if (x > " + std::to_string(i) + ") {\n";
            indent += "    ";
        }

        // Add a simple statement at the deepest nesting level
        nestedCode += indent + "return x + " + std::to_string(depth) + ";\n";

        // Close all the if statements
        for (int i = 0; i < depth; i++) {
            indent = indent.substr(0, indent.length() - 4);
            nestedCode += indent + "}\n";
        }

        nestedCode += "}\n\nlet x = 100;\nlet result = complexityTest();\nconsole.log(result);\n";

        // Run benchmark on this nested code
        BenchmarkResult result = benchmark.runBenchmark(nestedCode);
        results.push_back(result);

        printResult(result);
    }

    // Save complexity benchmark results
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    localtime_s(&tm_now, &now_time_t);

    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &tm_now);

    std::string filename = "benchmark_results/complexity_benchmark_" + std::string(buffer) + ".csv";
    benchmark.saveResultsToCSV(results, filename);

    std::cout << "Complexity results saved to: " << filename << std::endl;
}

void runRealWorldBenchmark() {
    std::cout << "Running real-world code benchmark..." << std::endl;

    // A collection of real-world JavaScript patterns
    std::vector<std::string> realWorldSamples = {// Sample 1: Class definition and usage
                                                 R"(
class Person {
    constructor(name, age) {
        this.name = name;
        this.age = age;
    }
    
    greet() {
        return `Hello, my name is ${this.name} and I am ${this.age} years old.`;
    }
    
    static createPerson(name, age) {
        return new Person(name, age);
    }
}

const john = new Person("John", 30);
console.log(john.greet());

const jane = Person.createPerson("Jane", 25);
console.log(jane.greet());
)",

                                                 // Sample 2: Async/await pattern
                                                 R"(
async function fetchData(url) {
    try {
        const response = await fetch(url);
        const data = await response.json();
        return data;
    } catch (error) {
        console.error("Error fetching data:", error);
        return null;
    }
}

async function processUserData() {
    const userData = await fetchData('https://api.example.com/users');
    if (userData) {
        userData.forEach(user => {
            console.log(`User: ${user.name}, Email: ${user.email}`);
        });
    }
}

processUserData();
)",

                                                 // Sample 3: Event handling and DOM manipulation
                                                 R"(
document.addEventListener('DOMContentLoaded', () => {
    const form = document.getElementById('registration-form');
    const nameInput = document.getElementById('name');
    const emailInput = document.getElementById('email');
    const submitButton = document.getElementById('submit');
    const errorDiv = document.getElementById('error-messages');
    
    function validateForm() {
        const errors = [];
        
        if (nameInput.value.length < 2) {
            errors.push("Name must be at least 2 characters");
        }
        
        const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
        if (!emailRegex.test(emailInput.value)) {
            errors.push("Please enter a valid email address");
        }
        
        if (errors.length > 0) {
            errorDiv.innerHTML = '';
            errors.forEach(error => {
                const p = document.createElement('p');
                p.textContent = error;
                p.classList.add('error');
                errorDiv.appendChild(p);
            });
            return false;
        }
        
        errorDiv.innerHTML = '';
        return true;
    }
    
    form.addEventListener('submit', (event) => {
        if (!validateForm()) {
            event.preventDefault();
        }
    });
    
    // Real-time validation
    nameInput.addEventListener('input', validateForm);
    emailInput.addEventListener('input', validateForm);
});
)"};

    Benchmark benchmark;
    std::vector<BenchmarkResult> results;

    // Run benchmarks on real-world samples
    for (const auto &sample: realWorldSamples) {
        BenchmarkResult result = benchmark.runBenchmark(sample);
        results.push_back(result);
        printResult(result);
    }

    // Save real-world benchmark results
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    localtime_s(&tm_now, &now_time_t);

    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &tm_now);

    std::string filename = "benchmark_results/realworld_benchmark_" + std::string(buffer) + ".csv";
    benchmark.saveResultsToCSV(results, filename);

    std::cout << "Real-world results saved to: " << filename << std::endl;
}

int main(int argc, char *argv[]) {
    std::cout << "JavaScript Compiler Benchmark Suite" << std::endl;
    std::cout << "===============================" << std::endl;

    // Create results directory
    std::filesystem::create_directories("benchmark_results");

    // Run different benchmark types
    runAndPrintSizeBenchmark();
    std::cout << "\n\n";

    runComplexityBenchmark();
    std::cout << "\n\n";

    runRealWorldBenchmark();

    std::cout << "\nAll benchmarks completed!" << std::endl;

    return 0;
}
