#include "../../include/llvm_backend/llvm_backend.h"
#include "../../include/error_reporter.h"
#include "../../include/llvm_backend/js_runtime.h"

// LLVM includes
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <chrono>
#include <iostream>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>

// Performance tracking variables for benchmarking
namespace {
    // Timing statistics
    std::chrono::milliseconds totalCompileTime(0);
    std::chrono::milliseconds totalOptimizationTime(0);
    std::chrono::milliseconds totalCodegenTime(0);
    std::chrono::milliseconds totalJITTime(0);

    // Memory statistics
    size_t peakMemoryUsage = 0;
    size_t cumulativeMemoryDelta = 0;

    // Counter for number of compilations
    size_t compilationCount = 0;

    // Helper function to get current memory usage
    size_t getCurrentMemoryUsage() {
#ifdef _WIN32
        // Windows implementation
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *) &pmc, sizeof(pmc))) {
            return static_cast<size_t>(pmc.WorkingSetSize);
        }
        return 0;
#elif defined(__APPLE__) && defined(__MACH__)
        // MacOS implementation
        struct mach_task_basic_info info;
        mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t) &info, &infoCount) == KERN_SUCCESS) {
            return static_cast<size_t>(info.resident_size);
        }
        return 0;
#elif defined(__linux__) || defined(__linux) || defined(linux)
        // Linux implementation
        long rss = 0L;
        FILE *fp = nullptr;
        if ((fp = fopen("/proc/self/statm", "r")) == nullptr) {
            return 0;
        }
        if (fscanf(fp, "%*s%ld", &rss) != 1) {
            fclose(fp);
            return 0;
        }
        fclose(fp);
        return static_cast<size_t>(rss * sysconf(_SC_PAGESIZE));
#else
        // Unsupported platform
        return 0;
#endif
    }

    // Update peak memory usage
    void updatePeakMemory() {
        size_t currentMemory = getCurrentMemoryUsage();
        peakMemoryUsage = std::max(peakMemoryUsage, currentMemory);
    }
} // namespace

// Reset performance tracking for a new run
void resetPerformanceTracking() {
    totalCompileTime = std::chrono::milliseconds(0);
    totalOptimizationTime = std::chrono::milliseconds(0);
    totalCodegenTime = std::chrono::milliseconds(0);
    totalJITTime = std::chrono::milliseconds(0);
    peakMemoryUsage = getCurrentMemoryUsage();
    cumulativeMemoryDelta = 0;
    compilationCount = 0;
}

// Get performance statistics as a formatted string
std::string getPerformanceStatistics() {
    std::stringstream ss;
    ss << "Performance Statistics:\n"
       << "  Compilation Count: " << compilationCount << "\n"
       << "  Average Compile Time: " << (compilationCount > 0 ? totalCompileTime.count() / compilationCount : 0)
       << " ms\n"
       << "  Average Optimization Time: "
       << (compilationCount > 0 ? totalOptimizationTime.count() / compilationCount : 0) << " ms\n"
       << "  Average Codegen Time: " << (compilationCount > 0 ? totalCodegenTime.count() / compilationCount : 0)
       << " ms\n"
       << "  Average JIT Time: " << (compilationCount > 0 ? totalJITTime.count() / compilationCount : 0) << " ms\n"
       << "  Peak Memory Usage: " << (peakMemoryUsage / 1024) << " KB\n"
       << "  Average Memory Delta: " << (compilationCount > 0 ? cumulativeMemoryDelta / compilationCount / 1024 : 0)
       << " KB\n";
    return ss.str();
}

// Function declarations for runtime functions
static std::unordered_map<std::string, llvm::Function *> runtimeFunctions;

// Helper macros for building runtime function declarations
#define DECLARE_RUNTIME_FUNC(name, returnType, ...)                                                                    \
    std::vector<llvm::Type *> name##Args = {__VA_ARGS__};                                                              \
    llvm::FunctionType *name##Ty = llvm::FunctionType::get(returnType, name##Args, false);                             \
    llvm::Function *name##Func = llvm::Function::Create(name##Ty, llvm::Function::ExternalLinkage, #name, *module);    \
    runtimeFunctions[#name] = name##Func;

// LLVMBackend implementation
LLVMBackend::LLVMBackend(const std::string &moduleName) :
    context(std::make_unique<llvm::LLVMContext>()), module(std::make_unique<llvm::Module>(moduleName, *context)),
    builder(std::make_unique<llvm::IRBuilder<>>(*context)), typeMapping(*context) {

    // Initialize the runtime
    runtime = std::make_unique<JSRuntime>(*context, module.get());
    runtime->declareAll();

    // Get references to runtime functions
    auto &funcs = runtime->getAllFunctions();
    runtimeFunctions.insert(funcs.begin(), funcs.end());

    // Update memory tracking
    updatePeakMemory();
}

LLVMBackend::~LLVMBackend() {
    // LLVM classes have proper destructors, nothing special needed here
    updatePeakMemory();
}

void LLVMBackend::initializeModule(const std::string &moduleName) {
    // Create a new LLVM module
    module = std::make_unique<llvm::Module>(moduleName, *context);

    // Create a builder for generating instructions
    builder = std::make_unique<llvm::IRBuilder<>>(*context);

    // Initialize JavaScript value types
    initializeTypes();

    // Declare runtime functions
    declareRuntimeFunctions();

    // Create main function
    createMainFunction();
}

void LLVMBackend::initializeTypes() {
    // JavaScript values are represented as 64-bit integers (for NaN-boxing)
    types.jsValueType = llvm::Type::getInt64Ty(*context);

    // String pointers are represented as char*
    types.jsStringPtrType = llvm::Type::getInt8PtrTy(*context);

    // JavaScript function type: jsValue(*)(int argc, jsValue* argv)
    std::vector<llvm::Type *> jsFunctionParams = {llvm::Type::getInt32Ty(*context),
                                                  llvm::PointerType::getUnqual(types.jsValueType)};
    types.jsFunctionType = llvm::FunctionType::get(types.jsValueType, jsFunctionParams, false);
}

void LLVMBackend::declareRuntimeFunctions() {
    // Type conversion functions
    DECLARE_RUNTIME_FUNC(js_get_type, llvm::Type::getInt32Ty(*context), types.jsValueType);
    DECLARE_RUNTIME_FUNC(js_value_to_number, llvm::Type::getDoubleTy(*context), types.jsValueType);
    DECLARE_RUNTIME_FUNC(js_value_to_boolean, llvm::Type::getInt1Ty(*context), types.jsValueType);
    DECLARE_RUNTIME_FUNC(js_typeof, types.jsValueType, types.jsValueType);

    // String handling
    DECLARE_RUNTIME_FUNC(js_make_string, types.jsValueType, types.jsStringPtrType);
    DECLARE_RUNTIME_FUNC(js_get_string_ptr, types.jsStringPtrType, types.jsValueType);

    // Object creation and property access
    DECLARE_RUNTIME_FUNC(js_create_object, types.jsValueType);
    DECLARE_RUNTIME_FUNC(js_create_array, types.jsValueType);
    DECLARE_RUNTIME_FUNC(js_set_property, llvm::Type::getVoidTy(*context), types.jsValueType, types.jsStringPtrType,
                         types.jsValueType);
    DECLARE_RUNTIME_FUNC(js_get_property, types.jsValueType, types.jsValueType, types.jsStringPtrType);

    // Array operations
    DECLARE_RUNTIME_FUNC(js_array_get, types.jsValueType, types.jsValueType, llvm::Type::getInt32Ty(*context));
    DECLARE_RUNTIME_FUNC(js_array_set, llvm::Type::getVoidTy(*context), types.jsValueType,
                         llvm::Type::getInt32Ty(*context), types.jsValueType);
    DECLARE_RUNTIME_FUNC(js_array_length, llvm::Type::getInt32Ty(*context), types.jsValueType);
    DECLARE_RUNTIME_FUNC(js_create_array_with_elements, types.jsValueType, llvm::Type::getInt32Ty(*context),
                         llvm::PointerType::getUnqual(types.jsValueType));

    // Function handling
    DECLARE_RUNTIME_FUNC(js_create_closure, types.jsValueType,
                         llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*context)));
    DECLARE_RUNTIME_FUNC(js_call_function, types.jsValueType, types.jsValueType, llvm::Type::getInt32Ty(*context),
                         llvm::PointerType::getUnqual(types.jsValueType));

    // Operators
    DECLARE_RUNTIME_FUNC(js_add, types.jsValueType, types.jsValueType, types.jsValueType);
    DECLARE_RUNTIME_FUNC(js_subtract, types.jsValueType, types.jsValueType, types.jsValueType);
    DECLARE_RUNTIME_FUNC(js_multiply, types.jsValueType, types.jsValueType, types.jsValueType);
    DECLARE_RUNTIME_FUNC(js_divide, types.jsValueType, types.jsValueType, types.jsValueType);
    DECLARE_RUNTIME_FUNC(js_equal, types.jsValueType, types.jsValueType, types.jsValueType);
    DECLARE_RUNTIME_FUNC(js_strict_equal, types.jsValueType, types.jsValueType, types.jsValueType);
    DECLARE_RUNTIME_FUNC(js_less_than, types.jsValueType, types.jsValueType, types.jsValueType);
    DECLARE_RUNTIME_FUNC(js_greater_than, types.jsValueType, types.jsValueType, types.jsValueType);

    // I/O functions
    DECLARE_RUNTIME_FUNC(js_print, llvm::Type::getVoidTy(*context), types.jsValueType);
    DECLARE_RUNTIME_FUNC(js_read_line, types.jsValueType);

    // Memory management
    llvm::Type *sizeTy = llvm::Type::getInt64Ty(*context);
    DECLARE_RUNTIME_FUNC(js_alloc, llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*context)), sizeTy);
    DECLARE_RUNTIME_FUNC(js_free, llvm::Type::getVoidTy(*context),
                         llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*context)));
}

void LLVMBackend::createMainFunction() {
    // Create main function type: int main(int argc, char** argv)
    llvm::Type *intTy = llvm::Type::getInt32Ty(*context);
    llvm::Type *charPtrPtrTy = llvm::Type::getInt8PtrTy(*context)->getPointerTo();

    std::vector<llvm::Type *> mainParams = {intTy, charPtrPtrTy};
    llvm::FunctionType *mainFuncTy = llvm::FunctionType::get(intTy, mainParams, false);

    // Create main function
    llvm::Function *mainFunc = llvm::Function::Create(mainFuncTy, llvm::Function::ExternalLinkage, "main", *module);

    // Create entry basic block
    llvm::BasicBlock *entryBB = llvm::BasicBlock::Create(*context, "entry", mainFunc);
    builder->SetInsertPoint(entryBB);

    // Store the current function
    currentFunction = mainFunc;
}

bool LLVMBackend::compile(const Program &program) {
    // Measure memory before compilation
    size_t memBefore = getCurrentMemoryUsage();

    // Start timing
    auto startTime = std::chrono::high_resolution_clock::now();
    auto codegenStartTime = startTime;

    try {
        compilationCount++;

        // Declare the main function
        llvm::FunctionType *mainType = llvm::FunctionType::get(typeMapping.getJSValueType(), // Return type is JS value
                                                               false // Not varargs
        );

        currentFunction = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", module.get());

        // Create a basic block for the entry point
        llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(*context, "entry", currentFunction);
        builder->SetInsertPoint(entryBlock);

        // Visit the program statements
        visitProgram(program);

        // If the last statement is not a return, add an implicit return undefined
        llvm::BasicBlock *currentBlock = builder->GetInsertBlock();
        if (currentBlock->empty() || !currentBlock->back().isTerminator()) {
            builder->CreateRet(createJSUndefined());
        }

        // Verify the function
        if (llvm::verifyFunction(*currentFunction, &llvm::errs())) {
            std::cerr << "Error in function verification" << std::endl;
            return false;
        }

        // Update codegen time
        auto codegenEndTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> codegenElapsed = codegenEndTime - codegenStartTime;
        totalCodegenTime += std::chrono::duration_cast<std::chrono::milliseconds>(codegenElapsed);

        // Update total time
        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = endTime - startTime;
        totalCompileTime += std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

        // Update memory statistics
        size_t memAfter = getCurrentMemoryUsage();
        cumulativeMemoryDelta += (memAfter > memBefore) ? (memAfter - memBefore) : 0;
        updatePeakMemory();

        return true;
    } catch (const std::exception &e) {
        std::cerr << "Exception during compilation: " << e.what() << std::endl;

        // Update memory statistics even on failure
        updatePeakMemory();

        return false;
    }
}

bool LLVMBackend::optimize(int level) {
    if (level <= 0) {
        return true; // No optimization
    }

    // Start timing
    auto startTime = std::chrono::high_resolution_clock::now();

    // Create pass managers
    llvm::legacy::FunctionPassManager funcPM(module.get());
    llvm::legacy::PassManager modulePM;

    // Add optimization passes based on level
    if (level >= 1) {
        // Basic optimizations
        funcPM.add(llvm::createInstructionCombiningPass()); // Combine similar instructions
        funcPM.add(llvm::createReassociatePass()); // Reassociate expressions
        funcPM.add(llvm::createGVNPass()); // Global value numbering
        funcPM.add(llvm::createCFGSimplificationPass()); // Simplify the control flow graph
    }

    if (level >= 2) {
        // More aggressive optimizations
        modulePM.add(llvm::createFunctionInliningPass()); // Function inlining
        modulePM.add(llvm::createGlobalDCEPass()); // Remove dead global variables/functions
        modulePM.add(llvm::createConstantMergePass()); // Merge duplicate global constants
    }

    if (level >= 3) {
        // Very aggressive optimizations
        funcPM.add(llvm::createTailCallEliminationPass()); // Eliminate tail recursion
        funcPM.add(llvm::createJumpThreadingPass()); // Thread branches through conditions
        funcPM.add(llvm::createDeadStoreEliminationPass()); // Remove dead stores
        modulePM.add(llvm::createAggressiveDCEPass()); // Aggressive dead code elimination
    }

    // Run function-level optimizations
    funcPM.doInitialization();
    for (auto &F: *module) {
        if (!F.isDeclaration()) {
            funcPM.run(F);
        }
    }
    funcPM.doFinalization();

    // Run module-level optimizations
    modulePM.run(*module);

    // Record optimization time
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = endTime - startTime;
    totalOptimizationTime += std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

    // Update memory statistics
    updatePeakMemory();

    return true;
}

std::string LLVMBackend::getIR() const {
    std::string output;
    llvm::raw_string_ostream stream(output);
    module->print(stream, nullptr);
    return output;
}

bool LLVMBackend::writeIR(const std::string &filename) const {
    std::error_code ec;
    llvm::raw_fd_ostream fileStream(filename, ec, llvm::sys::fs::OF_None);
    if (ec) {
        std::cerr << "Could not open file: " << ec.message() << std::endl;
        return false;
    }

    module->print(fileStream, nullptr);
    return true;
}

bool LLVMBackend::initializeJIT() {
    static bool initialized = false;

    if (!initialized) {
        // Initialize LLVM targets
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();

        initialized = true;
    }

    // Create a new JIT compiler instance
    auto jitBuilder = llvm::orc::LLJITBuilder();
    auto jitResult = jitBuilder.create();

    if (!jitResult) {
        std::cerr << "Failed to create JIT: " << toString(jitResult.takeError()) << std::endl;
        return false;
    }

    jit = std::move(*jitResult);
    return true;
}

double LLVMBackend::executeJIT() {
    // Start timing
    auto startTime = std::chrono::high_resolution_clock::now();

    if (!initializeJIT()) {
        return 0.0;
    }

    try {
        // Get a thread-safe module
        auto threadSafeModule = llvm::orc::ThreadSafeModule(std::move(module), std::move(context));

        // Add the module to the JIT
        if (auto err = jit->addIRModule(std::move(threadSafeModule))) {
            std::cerr << "Error adding module to JIT: " << toString(std::move(err)) << std::endl;
            return 0.0;
        }

        // Look up the main function
        auto mainSymbol = jit->lookup("main");
        if (!mainSymbol) {
            std::cerr << "Could not find main function: " << toString(mainSymbol.takeError()) << std::endl;
            return 0.0;
        }

        // Get function pointer to main
        auto mainFn = reinterpret_cast<uint64_t (*)()>(mainSymbol->getValue());

        // Execute the function
        uint64_t result = mainFn();

        // Record JIT execution time
        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = endTime - startTime;
        totalJITTime += std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

        // Update memory statistics
        updatePeakMemory();

        // Convert the result back based on the type
        if ((result & js_value::TAG_MASK) != js_value::QUIET_NAN) {
            // It's a double (not boxed)
            return *reinterpret_cast<double *>(&result);
        } else {
            // It's a boxed value, we'll just return the payload as double
            js_value::JSValueTag tag = js_value::getTag(result);
            uint64_t payload = js_value::getPayload(result);

            switch (tag) {
                case js_value::JS_TAG_BOOLEAN:
                    return payload ? 1.0 : 0.0;
                case js_value::JS_TAG_NUMBER:
                    // Should not happen as numbers aren't boxed
                    return 0.0;
                default:
                    // For other types, just return the payload as double
                    return static_cast<double>(payload);
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception during JIT execution: " << e.what() << std::endl;

        // Update memory statistics even on failure
        updatePeakMemory();

        return 0.0;
    }
}

bool LLVMBackend::createExecutable(const std::string &filename) {
    // Not implemented yet - this would require additional code to emit
    // an object file and link it into an executable.
    std::cerr << "createExecutable not implemented yet" << std::endl;
    return false;
}

llvm::AllocaInst *LLVMBackend::createEntryBlockAlloca(llvm::Function *func, const std::string &name) {
    // Create allocas in the entry block for better optimization
    llvm::IRBuilder<> tempBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
    return tempBuilder.CreateAlloca(typeMapping.getJSValueType(), nullptr, name);
}

// Helper methods for code generation
llvm::Value *LLVMBackend::createJSValueToDouble(llvm::Value *value) {
    return builder->CreateCall(runtimeFunctions["js_value_to_number"], {value});
}

llvm::Value *LLVMBackend::createDoubleToJSValue(llvm::Value *value) {
    // Create a temporary alloca for the double value
    llvm::AllocaInst *tempAlloca = builder->CreateAlloca(llvm::Type::getDoubleTy(*context), nullptr, "temp_double");
    builder->CreateStore(value, tempAlloca);

    // Bitcast to i64*
    llvm::Value *tempI64Ptr = builder->CreateBitCast(tempAlloca, llvm::Type::getInt64PtrTy(*context), "temp_i64_ptr");

    // Load the i64 value
    return builder->CreateLoad(llvm::Type::getInt64Ty(*context), tempI64Ptr, "double_as_i64");
}

llvm::Value *LLVMBackend::createJSValueToBoolean(llvm::Value *value) {
    return builder->CreateCall(runtimeFunctions["js_value_to_boolean"], {value});
}

llvm::Value *LLVMBackend::createBooleanToJSValue(llvm::Value *value) {
    // Convert the boolean to uint64_t (1 or 0)
    llvm::Value *intValue = builder->CreateZExt(value, llvm::Type::getInt64Ty(*context), "bool_to_int");

    // Create a NaN-boxed boolean value using a constant for the tag
    llvm::Value *tag = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), JS_TAG_BOOLEAN << TAG_SHIFT);
    llvm::Value *mask = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), PAYLOAD_MASK);
    llvm::Value *nanBase = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), QUIET_NAN);

    llvm::Value *masked = builder->CreateAnd(intValue, mask, "masked_payload");
    llvm::Value *withTag = builder->CreateOr(masked, tag, "with_tag");
    return builder->CreateOr(withTag, nanBase, "nan_boxed_bool");
}

// Helper methods for creating JavaScript values
llvm::Value *LLVMBackend::createJSUndefined() {
    return llvm::ConstantInt::get(typeMapping.getJSValueType(), JS_MakeUndefined());
}

llvm::Value *LLVMBackend::createJSNull() { return llvm::ConstantInt::get(typeMapping.getJSValueType(), JS_MakeNull()); }

llvm::Value *LLVMBackend::createJSBoolean(bool value) {
    llvm::Value *boolConstant = llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context), value ? 1 : 0);
    return createBooleanToJSValue(boolConstant);
}

llvm::Value *LLVMBackend::createJSNumber(double value) {
    llvm::Value *doubleConstant = llvm::ConstantFP::get(llvm::Type::getDoubleTy(*context), value);
    return createDoubleToJSValue(doubleConstant);
}

llvm::Value *LLVMBackend::createJSString(const std::string &value) {
    // Create a global string constant
    llvm::Constant *strConstant = builder->CreateGlobalStringPtr(value, "str_const");

    // Call the runtime function to create a JavaScript string
    return builder->CreateCall(runtimeFunctions["js_make_string"], {strConstant});
}

// Helper methods for extracting JavaScript values
llvm::Value *LLVMBackend::extractJSTag(llvm::Value *value) {
    llvm::Value *tagMask = llvm::ConstantInt::get(typeMapping.getJSValueType(), TAG_MASK);
    return builder->CreateAnd(value, tagMask, "js_tag");
}

llvm::Value *LLVMBackend::extractJSPayload(llvm::Value *value) {
    llvm::Value *payloadMask = llvm::ConstantInt::get(typeMapping.getJSValueType(), PAYLOAD_MASK);
    return builder->CreateAnd(value, payloadMask, "js_payload");
}

// Helper methods for method cache and expression value cache

llvm::Value *LLVMBackend::getFromMethodCache(llvm::Value *objectTypeTag, const std::string &methodName) {
    // First check if the tag is a constant
    llvm::ConstantInt *constTag = llvm::dyn_cast<llvm::ConstantInt>(objectTypeTag);
    if (!constTag) {
        // For dynamic tags, we'll need to use a runtime lookup
        return nullptr;
    }

    // For constant tags, we can do a compile-time lookup in our cache
    uint64_t tagValue = constTag->getZExtValue();
    MethodCacheKey key = {tagValue, methodName};

    auto it = methodCache.find(key);
    if (it != methodCache.end()) {
        return it->second;
    }

    return nullptr;
}

void LLVMBackend::storeInMethodCache(llvm::Value *objectTypeTag, const std::string &methodName, llvm::Value *method) {
    // Only cache methods for constant object types
    llvm::ConstantInt *constTag = llvm::dyn_cast<llvm::ConstantInt>(objectTypeTag);
    if (!constTag) {
        return;
    }

    // Store in the compile-time cache
    uint64_t tagValue = constTag->getZExtValue();
    MethodCacheKey key = {tagValue, methodName};
    methodCache[key] = method;
}

llvm::Value *LLVMBackend::getFromExprCache(const Expression *expr) {
    auto it = exprValueCache.find(expr);
    if (it != exprValueCache.end()) {
        return it->second;
    }
    return nullptr;
}

void LLVMBackend::storeInExprCache(const Expression *expr, llvm::Value *value) { exprValueCache[expr] = value; }

void LLVMBackend::clearExprCache() { exprValueCache.clear(); }

// Method lookup and dynamic dispatch implementation
llvm::Value *LLVMBackend::lookupMethod(llvm::Value *object, const std::string &methodName) {
    // Extract the object type tag for type-based method lookup
    llvm::Value *objectTypeTag = extractJSTag(object);

    // Check if the method is already in the cache
    llvm::Value *cachedMethod = getFromMethodCache(objectTypeTag, methodName);
    if (cachedMethod) {
        return cachedMethod;
    }

    // Create method name as a string constant
    llvm::Value *methodNameStr = builder->CreateGlobalStringPtr(methodName, "method_name");

    // Call runtime function to look up the method
    llvm::Function *lookupFunc = module->getFunction("js_lookup_method");
    llvm::Value *args[] = {object, methodNameStr};
    llvm::Value *method = builder->CreateCall(lookupFunc, args, "method_lookup");

    // Store the method in the cache for future lookups
    storeInMethodCache(objectTypeTag, methodName, method);

    return method;
}

llvm::Value *LLVMBackend::dynamicDispatch(llvm::Value *method, llvm::Value *object,
                                          const std::vector<llvm::Value *> &args) {
    // Create an array of arguments for the call
    std::vector<llvm::Value *> callArgs;

    // First argument is always the 'this' object
    callArgs.push_back(object);

    // Add the rest of the arguments
    for (auto arg: args) {
        callArgs.push_back(arg);
    }

    // Verify the method is a function
    llvm::Value *isFunction = typeMapping.isFunction(method, *builder);

    // Create basic blocks for function call and error handling
    llvm::Function *currentFunc = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *callBlock = llvm::BasicBlock::Create(*context, "method_call", currentFunc);
    llvm::BasicBlock *errorBlock = llvm::BasicBlock::Create(*context, "method_error", currentFunc);
    llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(*context, "method_merge", currentFunc);

    // Branch based on the type check
    builder->CreateCondBr(isFunction, callBlock, errorBlock);

    // Call block - function exists, make the call
    builder->SetInsertPoint(callBlock);

    // Use CallInst::Create to call the function pointer
    llvm::Value *callResult = builder->CreateCall(method, callArgs, "method_result");
    builder->CreateBr(mergeBlock);

    // Error block - method not found or not a function
    builder->SetInsertPoint(errorBlock);

    // Get runtime error function
    llvm::Function *errorFunc = module->getFunction("js_throw_type_error");
    llvm::Value *errorMsg = builder->CreateGlobalStringPtr("Method is not a function", "error_msg");
    builder->CreateCall(errorFunc, {errorMsg});

    // Return undefined in case of error
    llvm::Value *undefinedValue = createJSUndefined();
    builder->CreateBr(mergeBlock);

    // Merge block
    builder->SetInsertPoint(mergeBlock);

    // Create PHI node to merge results
    llvm::PHINode *result = builder->CreatePHI(typeMapping.getJSValueType(), 2, "dispatch_result");
    result->addIncoming(callResult, callBlock);
    result->addIncoming(undefinedValue, errorBlock);

    return result;
}

// Implementation of visitor methods for expressions
llvm::Value *LLVMBackend::visitLiteralExpr(const LiteralExpr &expr) {
    const auto &value = expr.value;

    if (std::holds_alternative<double>(value)) {
        double numValue = std::get<double>(value);
        return createDoubleToJSValue(llvm::ConstantFP::get(llvm::Type::getDoubleTy(*context), numValue));
    } else if (std::holds_alternative<bool>(value)) {
        bool boolValue = std::get<bool>(value);
        llvm::Value *boolConstant = llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context), boolValue ? 1 : 0);
        return createBooleanToJSValue(boolConstant);
    } else if (std::holds_alternative<std::string>(value)) {
        const std::string &strValue = std::get<std::string>(value);

        // Create a global string constant
        llvm::Constant *strConstant = builder->CreateGlobalStringPtr(strValue, "str_const");

        // Call the runtime function to create a JavaScript string
        return builder->CreateCall(runtimeFunctions["js_make_string"], {strConstant});
    } else if (std::holds_alternative<std::nullptr_t>(value)) {
        // Create a NaN-boxed null value
        return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), JS_MakeNull());
    } else {
        // Should be unreachable
        std::cerr << "Unsupported literal type!" << std::endl;
        return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), JS_MakeUndefined());
    }
}

llvm::Value *LLVMBackend::visitGroupingExpr(const GroupingExpr &expr) {
    // Grouping expressions just evaluate their inner expression
    return expr.expression->accept(*this);
}

llvm::Value *LLVMBackend::visitUnaryExpr(const UnaryExpr &expr) {
    llvm::Value *operand = expr.right->accept(*this);

    switch (expr.op.type) {
        case TokenType::MINUS: {
            // Negate the number value
            llvm::Value *doubleValue = createJSValueToDouble(operand);
            llvm::Value *negated = builder->CreateFNeg(doubleValue, "negated");
            return createDoubleToJSValue(negated);
        }

        case TokenType::BANG: {
            // Logical NOT
            llvm::Value *boolValue = createJSValueToBoolean(operand);
            llvm::Value *notValue = builder->CreateNot(boolValue, "logical_not");
            return createBooleanToJSValue(notValue);
        }

        default:
            std::cerr << "Unsupported unary operator: " << static_cast<int>(expr.op.type) << std::endl;
            return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), JS_MakeUndefined());
    }
}

llvm::Value *LLVMBackend::visitBinaryExpr(const BinaryExpr &expr) {
    llvm::Value *left = expr.left->accept(*this);
    llvm::Value *right = expr.right->accept(*this);

    switch (expr.op.type) {
        case TokenType::PLUS:
            // Call the js_add runtime function
            return builder->CreateCall(runtimeFunctions["js_add"], {left, right});

        case TokenType::MINUS:
            // Call the js_subtract runtime function
            return builder->CreateCall(runtimeFunctions["js_subtract"], {left, right});

        case TokenType::STAR:
            // Call the js_multiply runtime function
            return builder->CreateCall(runtimeFunctions["js_multiply"], {left, right});

        case TokenType::SLASH:
            // Call the js_divide runtime function
            return builder->CreateCall(runtimeFunctions["js_divide"], {left, right});

        case TokenType::EQUAL_EQUAL:
            // Call the js_equal runtime function
            return builder->CreateCall(runtimeFunctions["js_equal"], {left, right});

        case TokenType::BANG_EQUAL: {
            // Call the js_equal runtime function and negate the result
            llvm::Value *areEqual = builder->CreateCall(runtimeFunctions["js_equal"], {left, right});
            llvm::Value *boolValue = createJSValueToBoolean(areEqual);
            llvm::Value *notEqual = builder->CreateNot(boolValue, "not_equal");
            return createBooleanToJSValue(notEqual);
        }

        case TokenType::LESS:
            // Call the js_less_than runtime function
            return builder->CreateCall(runtimeFunctions["js_less_than"], {left, right});

        case TokenType::LESS_EQUAL: {
            // a <= b is equivalent to !(b < a)
            llvm::Value *bIsGreater = builder->CreateCall(runtimeFunctions["js_less_than"], {right, left});
            llvm::Value *boolValue = createJSValueToBoolean(bIsGreater);
            llvm::Value *aIsLessEqual = builder->CreateNot(boolValue, "less_equal");
            return createBooleanToJSValue(aIsLessEqual);
        }

        case TokenType::GREATER:
            // Call the js_greater_than runtime function
            return builder->CreateCall(runtimeFunctions["js_greater_than"], {left, right});

        case TokenType::GREATER_EQUAL: {
            // a >= b is equivalent to !(a < b)
            llvm::Value *aIsLess = builder->CreateCall(runtimeFunctions["js_less_than"], {left, right});
            llvm::Value *boolValue = createJSValueToBoolean(aIsLess);
            llvm::Value *aIsGreaterEqual = builder->CreateNot(boolValue, "greater_equal");
            return createBooleanToJSValue(aIsGreaterEqual);
        }

        default:
            std::cerr << "Unsupported binary operator: " << static_cast<int>(expr.op.type) << std::endl;
            return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), JS_MakeUndefined());
    }
}

llvm::Value *LLVMBackend::visitVarExpr(const VarExpr &expr) {
    auto it = namedValues.find(expr.name.lexeme);
    if (it == namedValues.end()) {
        std::cerr << "Undefined variable: " << expr.name.lexeme << std::endl;
        return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), JS_MakeUndefined());
    }

    // Load the variable value from the alloca
    return builder->CreateLoad(types.jsValueType, it->second, expr.name.lexeme);
}

llvm::Value *LLVMBackend::visitAssignExpr(const AssignExpr &expr) {
    llvm::Value *value = expr.value->accept(*this);

    auto it = namedValues.find(expr.name.lexeme);
    if (it == namedValues.end()) {
        std::cerr << "Undefined variable: " << expr.name.lexeme << std::endl;
        return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), JS_MakeUndefined());
    }

    // Store the new value into the variable
    builder->CreateStore(value, it->second);
    return value;
}

llvm::Value *LLVMBackend::visitLogicalExpr(const LogicalExpr &expr) {
    // Short-circuit evaluation for logical operators
    llvm::Function *currentFunc = builder->GetInsertBlock()->getParent();

    // Create blocks for the right operand and the merge point
    llvm::BasicBlock *rightBB = llvm::BasicBlock::Create(*context, "logical_right", currentFunc);
    llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(*context, "logical_merge", currentFunc);

    // Evaluate the left operand
    llvm::Value *left = expr.left->accept(*this);

    // Create a phi node for the result
    llvm::Value *leftBool = createJSValueToBoolean(left);

    // Branch based on the operator and the left value
    if (expr.op.type == TokenType::OR) {
        // For OR, if left is true, short-circuit to the merge block
        builder->CreateCondBr(leftBool, mergeBB, rightBB);
    } else { // AND
        // For AND, if left is false, short-circuit to the merge block
        builder->CreateCondBr(leftBool, rightBB, mergeBB);
    }

    // Right block: evaluate the right operand
    builder->SetInsertPoint(rightBB);
    llvm::Value *right = expr.right->accept(*this);
    builder->CreateBr(mergeBB);

    // Update rightBB for the phi node
    rightBB = builder->GetInsertBlock();

    // Merge block: create a phi node for the result
    builder->SetInsertPoint(mergeBB);
    llvm::PHINode *phi = builder->CreatePHI(types.jsValueType, 2, "logical_result");

    // Add incoming values to the phi node
    if (expr.op.type == TokenType::OR) {
        phi->addIncoming(left, expr.left->accept(*this)->getParent());
        phi->addIncoming(right, rightBB);
    } else { // AND
        phi->addIncoming(llvm::ConstantInt::get(types.jsValueType, JS_MakeBoolean(false)),
                         expr.left->accept(*this)->getParent());
        phi->addIncoming(right, rightBB);
    }

    return phi;
}

llvm::Value *LLVMBackend::visitCallExpr(const CallExpr &expr) {
    // Check if this is a method call (callee is a GetExpr)
    const GetExpr *getExpr = dynamic_cast<const GetExpr *>(expr.callee.get());

    if (getExpr) {
        // This is a method call: obj.method()
        llvm::Value *object = visit(*getExpr->object);

        // Look up the method on the object
        llvm::Value *method = lookupMethod(object, getExpr->name);

        // Visit all arguments
        std::vector<llvm::Value *> args;
        for (const auto &arg: expr.arguments) {
            args.push_back(visit(*arg));
        }

        // Perform dynamic dispatch
        return dynamicDispatch(method, object, args);
    } else {
        // Regular function call
        llvm::Value *callee = visit(*expr.callee);

        // Check if the callee is a function
        llvm::Value *isFunction = typeMapping.isFunction(callee, *builder);

        // Function arguments
        std::vector<llvm::Value *> args;
        for (const auto &arg: expr.arguments) {
            args.push_back(visit(*arg));
        }

        // Create blocks for function call and error handling
        llvm::Function *currentFunc = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *callBlock = llvm::BasicBlock::Create(*context, "function_call", currentFunc);
        llvm::BasicBlock *errorBlock = llvm::BasicBlock::Create(*context, "function_error", currentFunc);
        llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(*context, "function_merge", currentFunc);

        // Branch based on type check
        builder->CreateCondBr(isFunction, callBlock, errorBlock);

        // Call block
        builder->SetInsertPoint(callBlock);
        llvm::Value *undefinedThis = createJSUndefined(); // 'this' is undefined for regular calls
        std::vector<llvm::Value *> callArgs = {undefinedThis};
        callArgs.insert(callArgs.end(), args.begin(), args.end());

        llvm::Value *callResult = builder->CreateCall(callee, callArgs, "call_result");
        builder->CreateBr(mergeBlock);

        // Error block
        builder->SetInsertPoint(errorBlock);
        llvm::Function *errorFunc = module->getFunction("js_throw_type_error");
        llvm::Value *errorMsg = builder->CreateGlobalStringPtr("Not a function", "error_msg");
        builder->CreateCall(errorFunc, {errorMsg});
        llvm::Value *undefinedValue = createJSUndefined();
        builder->CreateBr(mergeBlock);

        // Merge block
        builder->SetInsertPoint(mergeBlock);
        llvm::PHINode *result = builder->CreatePHI(typeMapping.getJSValueType(), 2, "call_result_phi");
        result->addIncoming(callResult, callBlock);
        result->addIncoming(undefinedValue, errorBlock);

        return result;
    }
}

llvm::Value *LLVMBackend::visitGetExpr(const GetExpr &expr) {
    // Check if we have a cached value for this expression
    llvm::Value *cachedValue = getFromExprCache(&expr);
    if (cachedValue) {
        return cachedValue;
    }

    // Visit the object
    llvm::Value *object = visit(*expr.object);

    // Check if the object is valid (not null or undefined)
    llvm::Value *isNull = typeMapping.isNull(object, *builder);
    llvm::Value *isUndefined = typeMapping.isUndefined(object, *builder);
    llvm::Value *isInvalid = builder->CreateOr(isNull, isUndefined, "is_invalid");

    // Create blocks for property access and error handling
    llvm::Function *currentFunc = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *accessBlock = llvm::BasicBlock::Create(*context, "property_access", currentFunc);
    llvm::BasicBlock *errorBlock = llvm::BasicBlock::Create(*context, "property_error", currentFunc);
    llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(*context, "property_merge", currentFunc);

    // Branch based on validity check
    builder->CreateCondBr(isInvalid, errorBlock, accessBlock);

    // Access block - object is valid
    builder->SetInsertPoint(accessBlock);
    llvm::Value *propertyNameStr = builder->CreateGlobalStringPtr(expr.name, "property_name");

    // Call runtime function to get the property
    llvm::Function *getPropFunc = module->getFunction("js_get_property");
    llvm::Value *args[] = {object, propertyNameStr};
    llvm::Value *property = builder->CreateCall(getPropFunc, args, "property_value");
    builder->CreateBr(mergeBlock);

    // Error block - object is null or undefined
    builder->SetInsertPoint(errorBlock);
    llvm::Function *errorFunc = module->getFunction("js_throw_type_error");
    llvm::Value *errorMsg = builder->CreateGlobalStringPtr(
            "Cannot read property '" + expr.name + "' of null or undefined", "error_msg");
    builder->CreateCall(errorFunc, {errorMsg});
    llvm::Value *undefinedValue = createJSUndefined();
    builder->CreateBr(mergeBlock);

    // Merge block
    builder->SetInsertPoint(mergeBlock);
    llvm::PHINode *result = builder->CreatePHI(typeMapping.getJSValueType(), 2, "get_result");
    result->addIncoming(property, accessBlock);
    result->addIncoming(undefinedValue, errorBlock);

    // Cache the result
    storeInExprCache(&expr, result);

    return result;
}

llvm::Value *LLVMBackend::visitSetExpr(const SetExpr &expr) {
    // Evaluate the object
    llvm::Value *object = expr.object->accept(*this);

    // Evaluate the value to set
    llvm::Value *value = expr.value->accept(*this);

    // Create a string constant for the property name
    llvm::Constant *propName = builder->CreateGlobalStringPtr(expr.name.lexeme, "prop_name");

    // Call the js_set_property runtime function
    builder->CreateCall(runtimeFunctions["js_set_property"], {object, propName, value});

    // Return the value that was set
    return value;
}

llvm::Value *LLVMBackend::visitArrayExpr(const ArrayExpr &expr) {
    // Evaluate all elements
    std::vector<llvm::Value *> elements;
    for (const auto &element: expr.elements) {
        elements.push_back(element->accept(*this));
    }

    // Create an array to hold the elements
    llvm::Value *elemCount = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), elements.size());
    llvm::AllocaInst *elemArray = nullptr;

    if (!elements.empty()) {
        // Allocate space for the elements array
        elemArray = builder->CreateAlloca(types.jsValueType,
                                          llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), elements.size()),
                                          "elem_array");

        // Store each element in the array
        for (size_t i = 0; i < elements.size(); i++) {
            llvm::Value *elemPtr = builder->CreateGEP(types.jsValueType, elemArray,
                                                      llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), i),
                                                      "elem_ptr_" + std::to_string(i));
            builder->CreateStore(elements[i], elemPtr);
        }
    } else {
        // No elements, just create a null pointer
        elemArray = builder->CreateAlloca(types.jsValueType,
                                          llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0), "empty_elems");
    }

    // Call the js_create_array_with_elements runtime function
    return builder->CreateCall(runtimeFunctions["js_create_array_with_elements"], {elemCount, elemArray});
}

llvm::Value *LLVMBackend::visitObjectExpr(const ObjectExpr &expr) {
    // Create a new object
    llvm::Value *object = builder->CreateCall(runtimeFunctions["js_create_object"]);

    // Add all properties to the object
    for (const auto &property: expr.properties) {
        // Evaluate the property value
        llvm::Value *value = property.second->accept(*this);

        // Create a string constant for the property name
        llvm::Constant *propName = builder->CreateGlobalStringPtr(property.first.lexeme, "prop_name");

        // Set the property on the object
        builder->CreateCall(runtimeFunctions["js_set_property"], {object, propName, value});
    }

    return object;
}

llvm::Value *LLVMBackend::visitArrowFunctionExpr(const ArrowFunctionExpr &expr) {
    // Arrow function implementation is complex and would require closures
    // For simplicity, we'll return undefined for now
    std::cerr << "Arrow functions not yet implemented!" << std::endl;
    return llvm::ConstantInt::get(types.jsValueType, JS_MakeUndefined());
}

// Implementation of visitor methods for statements
void LLVMBackend::visitExpressionStmt(const ExpressionStmt &stmt) {
    // Just evaluate the expression and discard the result
    stmt.expression->accept(*this);
}

void LLVMBackend::visitPrintStmt(const PrintStmt &stmt) {
    // Evaluate the expression to print
    llvm::Value *value = stmt.expression->accept(*this);

    // Call the js_print runtime function
    builder->CreateCall(runtimeFunctions["js_print"], {value});
}

void LLVMBackend::visitVarStmt(const VarStmt &stmt) {
    // Evaluate the initializer expression (or use undefined if none)
    llvm::Value *value;
    if (stmt.initializer) {
        value = stmt.initializer->accept(*this);
    } else {
        value = llvm::ConstantInt::get(types.jsValueType, JS_MakeUndefined());
    }

    // Allocate space for the variable
    llvm::Function *func = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock &entryBlock = func->getEntryBlock();
    llvm::IRBuilder<> tempBuilder(&entryBlock, entryBlock.begin());
    llvm::AllocaInst *alloca = tempBuilder.CreateAlloca(types.jsValueType, nullptr, stmt.name.lexeme);

    // Store the initial value
    builder->CreateStore(value, alloca);

    // Add to symbol table
    namedValues[stmt.name.lexeme] = alloca;
}

void LLVMBackend::visitBlockStmt(const BlockStmt &stmt) {
    // Save the current named values
    std::unordered_map<std::string, llvm::Value *> savedValues = namedValues;

    // Visit all statements in the block
    for (const auto &s: stmt.statements) {
        s->accept(*this);
    }

    // Restore the named values
    namedValues = std::move(savedValues);
}

void LLVMBackend::visitIfStmt(const IfStmt &stmt) {
    // Evaluate the condition
    llvm::Value *condition = stmt.condition->accept(*this);

    // Convert the condition to a boolean
    llvm::Value *condBool = createJSValueToBoolean(condition);

    // Create basic blocks for then, else, and merge
    llvm::Function *func = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(*context, "if.then", func);
    llvm::BasicBlock *elseBB = stmt.elseBranch ? llvm::BasicBlock::Create(*context, "if.else") : nullptr;
    llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(*context, "if.merge");

    // Create the conditional branch
    if (elseBB) {
        builder->CreateCondBr(condBool, thenBB, elseBB);
    } else {
        builder->CreateCondBr(condBool, thenBB, mergeBB);
    }

    // Generate the then block
    builder->SetInsertPoint(thenBB);
    stmt.thenBranch->accept(*this);
    builder->CreateBr(mergeBB);

    // Generate the else block if it exists
    if (elseBB) {
        func->getBasicBlockList().push_back(elseBB);
        builder->SetInsertPoint(elseBB);
        stmt.elseBranch->accept(*this);
        builder->CreateBr(mergeBB);
    }

    // Generate the merge block
    func->getBasicBlockList().push_back(mergeBB);
    builder->SetInsertPoint(mergeBB);
}

void LLVMBackend::visitWhileStmt(const WhileStmt &stmt) {
    // Create basic blocks for the condition, body, and after
    llvm::Function *func = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *condBB = llvm::BasicBlock::Create(*context, "while.cond", func);
    llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(*context, "while.body", func);
    llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(*context, "while.end", func);

    // Branch to the condition block
    builder->CreateBr(condBB);

    // Generate the condition block
    builder->SetInsertPoint(condBB);
    llvm::Value *condition = stmt.condition->accept(*this);
    llvm::Value *condBool = createJSValueToBoolean(condition);
    builder->CreateCondBr(condBool, bodyBB, afterBB);

    // Generate the body block
    builder->SetInsertPoint(bodyBB);
    stmt.body->accept(*this);
    builder->CreateBr(condBB);

    // Continue from the after block
    builder->SetInsertPoint(afterBB);
}

void LLVMBackend::visitForStmt(const ForStmt &stmt) {
    // Save the current named values for restoring later
    std::unordered_map<std::string, llvm::Value *> savedValues = namedValues;

    // Create basic blocks for the setup, condition, increment, body, and after
    llvm::Function *func = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *setupBB = llvm::BasicBlock::Create(*context, "for.setup", func);
    llvm::BasicBlock *condBB = llvm::BasicBlock::Create(*context, "for.cond", func);
    llvm::BasicBlock *incBB = llvm::BasicBlock::Create(*context, "for.inc", func);
    llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(*context, "for.body", func);
    llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(*context, "for.end", func);

    // Branch to the setup block
    builder->CreateBr(setupBB);

    // Generate the setup block
    builder->SetInsertPoint(setupBB);
    if (stmt.initializer) {
        stmt.initializer->accept(*this);
    }
    builder->CreateBr(condBB);

    // Generate the condition block
    builder->SetInsertPoint(condBB);
    llvm::Value *condBool;
    if (stmt.condition) {
        llvm::Value *condition = stmt.condition->accept(*this);
        condBool = createJSValueToBoolean(condition);
    } else {
        // If no condition, it's always true
        condBool = llvm::ConstantInt::getTrue(*context);
    }
    builder->CreateCondBr(condBool, bodyBB, afterBB);

    // Generate the body block
    builder->SetInsertPoint(bodyBB);
    stmt.body->accept(*this);
    builder->CreateBr(incBB);

    // Generate the increment block
    builder->SetInsertPoint(incBB);
    if (stmt.increment) {
        stmt.increment->accept(*this);
    }
    builder->CreateBr(condBB);

    // Continue from the after block
    builder->SetInsertPoint(afterBB);

    // Restore the named values
    namedValues = std::move(savedValues);
}

void LLVMBackend::visitReturnStmt(const ReturnStmt &stmt) {
    // Evaluate the return value (or use undefined if none)
    llvm::Value *value;
    if (stmt.value) {
        value = stmt.value->accept(*this);
    } else {
        value = llvm::ConstantInt::get(types.jsValueType, JS_MakeUndefined());
    }

    // Store the return value and create a branch to the return block
    returnValue = value;
    builder->CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0));
}

void LLVMBackend::visitFunctionStmt(const FunctionStmt &stmt) {
    // Function implementation is complex and would require closures
    // For simplicity, we'll skip it for now
    std::cerr << "Function declarations not yet implemented!" << std::endl;
}

#undef DECLARE_RUNTIME_FUNC
