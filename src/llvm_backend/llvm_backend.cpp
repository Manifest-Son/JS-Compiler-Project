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

// Function declarations for runtime functions
static std::unordered_map<std::string, llvm::Function *> runtimeFunctions;

// Helper macros for building runtime function declarations
#define DECLARE_RUNTIME_FUNC(name, returnType, ...)                                                                    \
    std::vector<llvm::Type *> name##Args = {__VA_ARGS__};                                                              \
    llvm::FunctionType *name##Ty = llvm::FunctionType::get(returnType, name##Args, false);                             \
    llvm::Function *name##Func = llvm::Function::Create(name##Ty, llvm::Function::ExternalLinkage, #name, *module);    \
    runtimeFunctions[#name] = name##Func;

// LLVMBackend implementation
LLVMBackend::LLVMBackend(const std::string &moduleName) : currentFunction(nullptr), returnValue(nullptr) {
    // Initialize LLVM
    static bool isLLVMInitialized = false;
    if (!isLLVMInitialized) {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();
        isLLVMInitialized = true;
    }

    // Create LLVM context and module
    context = std::make_unique<llvm::LLVMContext>();

    // Initialize the type mapping with the created context
    typeMapping = JSTypeMapping(*context);

    // Initialize the module
    initializeModule(moduleName);
}

LLVMBackend::~LLVMBackend() {
    // Cleanup is handled by smart pointers
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
    // Create basic block inside the main function
    llvm::Function *mainFunc = module->getFunction("main");
    if (!mainFunc) {
        std::cerr << "Failed to find main function." << std::endl;
        return false;
    }

    // Set insert point to the entry block
    llvm::BasicBlock *entryBB = &mainFunc->getEntryBlock();
    builder->SetInsertPoint(entryBB);

    // Visit all statements in the program
    for (const auto &stmt: program.statements) {
        stmt->accept(*this);
    }

    // Return 0 from main
    builder->CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0));

    // Verify the module
    std::string errorMessage;
    llvm::raw_string_ostream errorStream(errorMessage);
    if (llvm::verifyModule(*module, &errorStream)) {
        std::cerr << "Error verifying module: " << errorMessage << std::endl;
        return false;
    }

    return true;
}

bool LLVMBackend::optimize(int level) {
    if (level <= 0)
        return true; // No optimization

    // Create the optimization pipeline
    llvm::PassBuilder passBuilder;

    // Create analysis managers
    llvm::LoopAnalysisManager lam;
    llvm::FunctionAnalysisManager fam;
    llvm::CGSCCAnalysisManager cgam;
    llvm::ModuleAnalysisManager mam;

    // Register the analysis managers
    passBuilder.registerModuleAnalyses(mam);
    passBuilder.registerCGSCCAnalyses(cgam);
    passBuilder.registerFunctionAnalyses(fam);
    passBuilder.registerLoopAnalyses(lam);
    passBuilder.crossRegisterProxies(lam, fam, cgam, mam);

    // Create the optimization pipeline based on the optimization level
    llvm::OptimizationLevel optLevel;
    switch (level) {
        case 1:
            optLevel = llvm::OptimizationLevel::O1;
            break;
        case 2:
            optLevel = llvm::OptimizationLevel::O2;
            break;
        default:
            optLevel = llvm::OptimizationLevel::O3;
            break;
    }

    // Create the pass manager
    llvm::ModulePassManager mpm = passBuilder.buildPerModuleDefaultPipeline(optLevel);

    // Run the passes
    mpm.run(*module, mam);

    return true;
}

std::string LLVMBackend::getIR() const {
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    module->print(irStream, nullptr);
    return ir;
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
    // Evaluate the callee
    llvm::Value *callee = expr.callee->accept(*this);

    // Evaluate the arguments
    std::vector<llvm::Value *> args;
    for (const auto &arg: expr.arguments) {
        args.push_back(arg->accept(*this));
    }

    // Create an array to hold the arguments
    llvm::Value *argCount = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), args.size());
    llvm::AllocaInst *argsArray = nullptr;

    if (!args.empty()) {
        // Allocate space for the arguments array
        argsArray = builder->CreateAlloca(
                types.jsValueType, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), args.size()), "args_array");

        // Store each argument in the array
        for (size_t i = 0; i < args.size(); i++) {
            llvm::Value *argPtr = builder->CreateGEP(types.jsValueType, argsArray,
                                                     llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), i),
                                                     "arg_ptr_" + std::to_string(i));
            builder->CreateStore(args[i], argPtr);
        }
    } else {
        // No arguments, just create a null pointer
        argsArray = builder->CreateAlloca(types.jsValueType,
                                          llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0), "empty_args");
    }

    // Call the function
    return builder->CreateCall(runtimeFunctions["js_call_function"], {callee, argCount, argsArray});
}

llvm::Value *LLVMBackend::visitGetExpr(const GetExpr &expr) {
    // Evaluate the object
    llvm::Value *object = expr.object->accept(*this);

    // Create a string constant for the property name
    llvm::Constant *propName = builder->CreateGlobalStringPtr(expr.name.lexeme, "prop_name");

    // Call the js_get_property runtime function
    return builder->CreateCall(runtimeFunctions["js_get_property"], {object, propName});
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
