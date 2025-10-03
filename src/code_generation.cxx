#include "code_generation.h"

#include <iostream>
#include <string>

namespace void_compiler {
CodeGenerator::CodeGenerator() {
  context_ = std::make_unique<llvm::LLVMContext>();
  module_ = std::make_unique<llvm::Module>("void_module", *context_);
  builder_ = std::make_unique<llvm::IRBuilder<>>(*context_);
}

void CodeGenerator::generate_program(const Program* program) {
  // Process imports (for now, we'll just ignore them since fmt is built-in)
  for (const auto& import : program->imports()) {
    (void)import;  // Mark as used - imports are implicitly handled
  }
  
  for (const auto& func : program->functions()) {
    generate_function(func.get());
  }
}

void CodeGenerator::generate_function(const FunctionDeclaration* func_decl) {
  // Create parameter types
  std::vector<llvm::Type*> param_types;
  for (const auto& param : func_decl->parameters()) {
    (void)param;  // Mark as used
    param_types.push_back(llvm::Type::getInt32Ty(*context_));
  }

  // Create function type
  llvm::Type* return_type;
  if (func_decl->return_type() == "void") {
    return_type = llvm::Type::getVoidTy(*context_);
  } else {
    return_type = llvm::Type::getInt32Ty(*context_);
  }
  
  llvm::FunctionType* func_type =
      llvm::FunctionType::get(return_type, param_types, false);

  // Create function
  llvm::Function* function =
      llvm::Function::Create(func_type, llvm::Function::ExternalLinkage,
                             func_decl->name(), module_.get());

  // Set parameter names
  size_t idx = 0;
  for (auto& arg : function->args()) {
    arg.setName(func_decl->parameters()[idx++]->name());
  }

  // Create basic block
  llvm::BasicBlock* entry =
      llvm::BasicBlock::Create(*context_, "entry", function);
  builder_->SetInsertPoint(entry);

  // Store parameter values in allocas for later reference
  function_params_.clear();
  local_variables_.clear();
  for (auto& arg : function->args()) {
    llvm::AllocaInst* alloca = builder_->CreateAlloca(
        llvm::Type::getInt32Ty(*context_), nullptr, arg.getName());
    builder_->CreateStore(&arg, alloca);
    function_params_[std::string(arg.getName())] = alloca;
  }

  // Generate function body
  for (const auto& stmt : func_decl->body()) {
    generate_statement(stmt.get(), function);
  }
  
  // If this is a void function and there's no terminator, add a void return
  if (func_decl->return_type() == "void" && !builder_->GetInsertBlock()->getTerminator()) {
    builder_->CreateRetVoid();
  }
}

void CodeGenerator::print_ir() const { module_->print(llvm::outs(), nullptr); }

bool CodeGenerator::compile_to_object(const std::string& filename) {
  // Initialize only native target (much simpler and smaller)
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  // Get target triple
  auto target_triple = llvm::sys::getDefaultTargetTriple();
  module_->setTargetTriple(target_triple);

  std::string error;
  auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);
  if (!target) {
    std::cerr << "Error: " << error << '\n';
    return false;
  }

  auto cpu = "generic";
  auto features = "";

  llvm::TargetOptions opt;
  std::optional<llvm::Reloc::Model> relocModel;
  auto target_machine = target->createTargetMachine(target_triple, cpu,
                                                    features, opt, relocModel);

  module_->setDataLayout(target_machine->createDataLayout());

  // Open output file
  std::error_code error_code;
  llvm::raw_fd_ostream dest(filename, error_code, llvm::sys::fs::OF_None);
  if (error_code) {
    std::cerr << "Could not open file: " << error_code.message() << '\n';
    return false;
  }

  // Generate object file
  llvm::legacy::PassManager pass;
  auto file_type = llvm::CodeGenFileType::ObjectFile;

  if (target_machine->addPassesToEmitFile(pass, dest, nullptr, file_type)) {
    std::cerr << "TargetMachine can't emit a file of this type" << '\n';
    return false;
  }

  pass.run(*module_);
  dest.flush();

  std::cout << "Object file written to " << filename << '\n';
  return true;
}

int CodeGenerator::run_jit() {
  // Initialize LLVM
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  // Create execution engine
  std::string error_str;
  llvm::ExecutionEngine* ee =
      llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module_.release()))
          .setErrorStr(&error_str)
          .create();

  if (!ee) {
    throw std::runtime_error("Failed to create execution engine: " + error_str);
  }

  // Get the main function
  llvm::Function* main_func = ee->FindFunctionNamed("main");
  if (!main_func) {
    throw std::runtime_error("Main function not found");
  }

  // Execute
  llvm::GenericValue result = ee->runFunction(main_func, {});
  return static_cast<int>(result.IntVal.getSExtValue());
}

llvm::Value* CodeGenerator::generate_expression(const ASTNode* node) {
  if (const auto* num = dynamic_cast<const NumberLiteral*>(node)) {
    return llvm::ConstantInt::get(*context_,
                                  llvm::APInt(32, num->value(), true));
  }

  if (const auto* str = dynamic_cast<const StringLiteral*>(node)) {
    return builder_->CreateGlobalStringPtr(str->value());
  }

  if (const auto* var = dynamic_cast<const VariableReference*>(node)) {
    // Check function parameters first
    auto it = function_params_.find(var->name());
    if (it != function_params_.end()) {
      return builder_->CreateLoad(llvm::Type::getInt32Ty(*context_), it->second,
                                  var->name());
    }
    
    // Check local variables
    auto local_it = local_variables_.find(var->name());
    if (local_it != local_variables_.end()) {
      return builder_->CreateLoad(llvm::Type::getInt32Ty(*context_), local_it->second,
                                  var->name());
    }
    
    throw std::runtime_error("Unknown variable: " + var->name());
  }

  if (const auto* binop = dynamic_cast<const BinaryOperation*>(node)) {
    llvm::Value* left = generate_expression(binop->left());
    llvm::Value* right = generate_expression(binop->right());

    switch (binop->operator_type()) {
      case TokenType::Plus:
        return builder_->CreateAdd(left, right, "addtmp");
      case TokenType::Minus:
        return builder_->CreateSub(left, right, "subtmp");
      case TokenType::Multiply:
        return builder_->CreateMul(left, right, "multmp");
      case TokenType::Divide:
        return builder_->CreateSDiv(left, right, "divtmp");
      default:
        throw std::runtime_error("Unknown binary operator");
    }
  }

  if (const auto* call = dynamic_cast<const FunctionCall*>(node)) {
    llvm::Function* func = module_->getFunction(call->function_name());
    if (!func) {
      throw std::runtime_error("Unknown function: " + call->function_name());
    }

    // Validate argument count matches parameter count
    size_t expected_args = func->arg_size();
    size_t provided_args = call->arguments().size();
    if (provided_args != expected_args) {
      throw std::runtime_error("Function '" + call->function_name() + 
                              "' expects " + std::to_string(expected_args) + 
                              " arguments, but " + std::to_string(provided_args) + 
                              " were provided");
    }

    // Generate arguments
    std::vector<llvm::Value*> args;
    for (const auto& arg : call->arguments()) {
      args.push_back(generate_expression(arg.get()));
    }

    return builder_->CreateCall(func, args);
  }

  if (const auto* member = dynamic_cast<const MemberAccess*>(node)) {
    // Handle fmt.println specifically
    if (member->object_name() == "fmt" && member->member_name() == "println") {
      // Get or create printf function
      llvm::Function* printf_func = module_->getFunction("printf");
      if (!printf_func) {
        // Create printf function type: int printf(char*, ...)
        llvm::Type* char_ptr_type = llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0);
        llvm::FunctionType* printf_type = 
            llvm::FunctionType::get(llvm::Type::getInt32Ty(*context_), 
                                  {char_ptr_type}, true);
        printf_func = llvm::Function::Create(printf_type, 
                                           llvm::Function::ExternalLinkage, 
                                           "printf", module_.get());
      }

      // Generate arguments for printf
      std::vector<llvm::Value*> printf_args;
      
      if (member->arguments().size() >= 1) {
        // First argument should be the format string - convert from Rust-style to C-style
        if (const auto* format_str_node = dynamic_cast<const StringLiteral*>(member->arguments()[0].get())) {
          std::string format_str = format_str_node->value();
          
          // Convert Rust-style format placeholders to C-style
          // {:d} -> %d, {:s} -> %s, etc.
          size_t pos = 0;
          while ((pos = format_str.find("{:d}", pos)) != std::string::npos) {
            format_str.replace(pos, 4, "%d");
            pos += 2;
          }
          while ((pos = format_str.find("{:s}", pos)) != std::string::npos) {
            format_str.replace(pos, 4, "%s");
            pos += 2;
          }
          
          // Add newline for println
          format_str += "\n";
          
          // Create the modified format string
          llvm::Value* c_format_str = builder_->CreateGlobalStringPtr(format_str);
          printf_args.push_back(c_format_str);
        } else {
          // If it's not a string literal, use it as-is
          printf_args.push_back(generate_expression(member->arguments()[0].get()));
        }
        
        // Process additional arguments
        for (size_t i = 1; i < member->arguments().size(); ++i) {
          printf_args.push_back(generate_expression(member->arguments()[i].get()));
        }
      }
      
      // Add newline to format string (modify the format string to include \n)
      // For now, we'll assume the format string already includes formatting
      
      return builder_->CreateCall(printf_func, printf_args);
    }
    
    throw std::runtime_error("Unknown member access: " + member->object_name() + 
                            "." + member->member_name());
  }

  throw std::runtime_error("Unknown expression type");
}

void CodeGenerator::generate_statement(const ASTNode* node,
                                       llvm::Function* function) {
  (void)function;  // Mark as used
  if (const auto* ret = dynamic_cast<const ReturnStatement*>(node)) {
    llvm::Value* ret_val = generate_expression(ret->expression());
    builder_->CreateRet(ret_val);
    return;
  }
  
  if (const auto* var_decl = dynamic_cast<const VariableDeclaration*>(node)) {
    // Generate the initial value
    llvm::Value* init_value = generate_expression(var_decl->value());
    
    // Create local variable (alloca)
    llvm::AllocaInst* alloca = builder_->CreateAlloca(
        llvm::Type::getInt32Ty(*context_), nullptr, var_decl->name());
    
    // Store the initial value
    builder_->CreateStore(init_value, alloca);
    
    // Add to local variables map for later reference
    local_variables_[var_decl->name()] = alloca;
    return;
  }
  
  if (const auto* var_assign = dynamic_cast<const VariableAssignment*>(node)) {
    // Generate the new value
    llvm::Value* new_value = generate_expression(var_assign->value());
    
    // Find the variable (check local variables first, then function parameters)
    auto local_it = local_variables_.find(var_assign->name());
    if (local_it != local_variables_.end()) {
      builder_->CreateStore(new_value, local_it->second);
      return;
    }
    
    auto param_it = function_params_.find(var_assign->name());
    if (param_it != function_params_.end()) {
      builder_->CreateStore(new_value, param_it->second);
      return;
    }
    
    throw std::runtime_error("Unknown variable for assignment: " + var_assign->name());
  }
  
  // Handle member access as a statement (e.g., fmt.println calls)
  if (const auto* member = dynamic_cast<const MemberAccess*>(node)) {
    generate_expression(member);  // Generate the call and discard the return value
    return;
  }
  
  throw std::runtime_error("Unknown statement type");
}

}  // namespace void_compiler
