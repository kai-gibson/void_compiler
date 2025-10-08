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
  if (func_decl->return_type() == "void" || func_decl->return_type() == "nil") {
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
  
  // Track current function's return type for validation
  current_function_return_type_ = func_decl->return_type();
  
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
  
  // If this is a void/nil function and there's no terminator, add a void return
  if ((func_decl->return_type() == "void" || func_decl->return_type() == "nil") && !builder_->GetInsertBlock()->getTerminator()) {
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

  if (const auto* boolean = dynamic_cast<const BooleanLiteral*>(node)) {
    return llvm::ConstantInt::get(*context_,
                                  llvm::APInt(1, boolean->value() ? 1 : 0, false));
  }

  if (const auto* var = dynamic_cast<const VariableReference*>(node)) {
    // Check function parameters first
    auto it = function_params_.find(var->name());
    if (it != function_params_.end()) {
      // For now, assume function parameters are i32 (TODO: track parameter types)
      return builder_->CreateLoad(llvm::Type::getInt32Ty(*context_), it->second,
                                  var->name());
    }
    
    // Check local variables
    auto local_it = local_variables_.find(var->name());
    if (local_it != local_variables_.end()) {
      // Get the variable type and use correct load type
      auto type_it = variable_types_.find(var->name());
      if (type_it != variable_types_.end()) {
        llvm::Type* load_type = get_llvm_type_from_string(type_it->second);
        return builder_->CreateLoad(load_type, local_it->second, var->name());
      } else {
        // Fallback to i32 if type not found (shouldn't happen)
        return builder_->CreateLoad(llvm::Type::getInt32Ty(*context_), local_it->second,
                                    var->name());
      }
    }
    
    // Check if it's a function name (for function pointer assignment)
    llvm::Function* func = module_->getFunction(var->name());
    if (func) {
      // Return the function as a value (function pointer)
      return func;
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
      case TokenType::GreaterThan:
        return builder_->CreateICmpSGT(left, right, "gttmp");
      case TokenType::LessThan:
        return builder_->CreateICmpSLT(left, right, "lttmp");
      case TokenType::GreaterEqual:
        return builder_->CreateICmpSGE(left, right, "getmp");
      case TokenType::LessEqual:
        return builder_->CreateICmpSLE(left, right, "letmp");
      case TokenType::EqualEqual:
        return builder_->CreateICmpEQ(left, right, "eqtmp");
      case TokenType::NotEqual:
        return builder_->CreateICmpNE(left, right, "netmp");
      case TokenType::And:
        return builder_->CreateAnd(left, right, "andtmp");
      case TokenType::Or:
        return builder_->CreateOr(left, right, "ortmp");
      default:
        throw std::runtime_error("Unknown binary operator");
    }
  }

  if (const auto* unary = dynamic_cast<const UnaryOperation*>(node)) {
    llvm::Value* operand = generate_expression(unary->operand());

    switch (unary->operator_type()) {
      case TokenType::Not:
        return builder_->CreateNot(operand, "nottmp");
      default:
        throw std::runtime_error("Unknown unary operator");
    }
  }

  if (const auto* call = dynamic_cast<const FunctionCall*>(node)) {
    // Check if this is a function pointer call first
    auto local_it = local_variables_.find(call->function_name());
    if (local_it != local_variables_.end()) {
      // Check if it's a function pointer variable
      auto type_it = variable_types_.find(call->function_name());
      if (type_it != variable_types_.end() && is_function_pointer_type(type_it->second)) {
        // This is a function pointer call
        // Load the function pointer from the variable
        llvm::Type* func_ptr_type = get_llvm_type_from_string(type_it->second);
        llvm::Value* func_ptr = builder_->CreateLoad(func_ptr_type, local_it->second, 
                                                     call->function_name());
        
        // Parse function type to get parameter and return types
        FunctionType func_type = parse_function_type(type_it->second);
        
        // Validate argument count
        size_t expected_args = func_type.param_types().size();
        size_t provided_args = call->arguments().size();
        if (provided_args != expected_args) {
          throw std::runtime_error("Function pointer '" + call->function_name() + 
                                  "' expects " + std::to_string(expected_args) + 
                                  " arguments, but " + std::to_string(provided_args) + 
                                  " were provided");
        }
        
        // Generate arguments
        std::vector<llvm::Value*> args;
        for (const auto& arg : call->arguments()) {
          args.push_back(generate_expression(arg.get()));
        }
        
        // Get the LLVM function type for the indirect call
        std::vector<llvm::Type*> llvm_param_types;
        for (const auto& param_type : func_type.param_types()) {
          llvm_param_types.push_back(get_llvm_type_from_string(param_type));
        }
        llvm::Type* llvm_return_type = get_llvm_type_from_string(func_type.return_type());
        llvm::FunctionType* function_type = llvm::FunctionType::get(llvm_return_type, 
                                                                    llvm_param_types, false);
        
        // Create indirect call through function pointer
        return builder_->CreateCall(function_type, func_ptr, args);
      }
    }
    
    // Fall back to direct function call
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

  if (const auto* anon_func = dynamic_cast<const AnonymousFunction*>(node)) {
    return generate_anonymous_function(anon_func);
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
            pos += 2;  // Move past the replacement
          }
          pos = 0;  // Reset position for next replacement
          while ((pos = format_str.find("{:s}", pos)) != std::string::npos) {
            format_str.replace(pos, 4, "%s");
            pos += 2;  // Move past the replacement
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
    if (ret->expression() == nullptr) {
      // Return without value - only allowed for nil functions
      if (current_function_return_type_ != "nil") {
        throw std::runtime_error("Cannot use 'return' without value in non-nil function");
      }
      builder_->CreateRetVoid();
    } else {
      // Return with value - not allowed for nil functions
      if (current_function_return_type_ == "nil") {
        throw std::runtime_error("Cannot return a value from a nil function");
      }
      llvm::Value* ret_val = generate_expression(ret->expression());
      builder_->CreateRet(ret_val);
    }
    return;
  }
  
  if (const auto* var_decl = dynamic_cast<const VariableDeclaration*>(node)) {
    // Generate the initial value
    llvm::Value* init_value = generate_expression(var_decl->value());
    
    // Determine the LLVM type based on the variable type using helper method
    llvm::Type* var_type = get_llvm_type_from_string(var_decl->type());
    
    // Create local variable (alloca)
    llvm::AllocaInst* alloca = builder_->CreateAlloca(
        var_type, nullptr, var_decl->name());
    
    // Store the initial value
    builder_->CreateStore(init_value, alloca);
    
    // Add to local variables map for later reference
    local_variables_[var_decl->name()] = alloca;
    variable_types_[var_decl->name()] = var_decl->type();  // Track the type
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
  
  // Handle function call as a statement (e.g., nil function calls)
  if (const auto* func_call = dynamic_cast<const FunctionCall*>(node)) {
    generate_expression(func_call);  // Generate the call and discard the return value
    return;
  }
  
  // Handle if statements
  if (const auto* if_stmt = dynamic_cast<const IfStatement*>(node)) {
    // Generate condition expression
    llvm::Value* condition = generate_expression(if_stmt->condition());
    
    // Create basic blocks for then, else, and merge
    llvm::BasicBlock* then_block = llvm::BasicBlock::Create(*context_, "then", function);
    llvm::BasicBlock* else_block = llvm::BasicBlock::Create(*context_, "else", function);
    llvm::BasicBlock* merge_block = llvm::BasicBlock::Create(*context_, "ifcont", function);
    
    // Create conditional branch
    builder_->CreateCondBr(condition, then_block, else_block);
    
    // Generate then block
    builder_->SetInsertPoint(then_block);
    for (const auto& stmt : if_stmt->then_body()) {
      generate_statement(stmt.get(), function);
    }
    // Only add branch if the block doesn't already have a terminator (e.g., return)
    if (!builder_->GetInsertBlock()->getTerminator()) {
      builder_->CreateBr(merge_block);
    }
    
    // Generate else block
    builder_->SetInsertPoint(else_block);
    for (const auto& stmt : if_stmt->else_body()) {
      generate_statement(stmt.get(), function);
    }
    // Only add branch if the block doesn't already have a terminator
    if (!builder_->GetInsertBlock()->getTerminator()) {
      builder_->CreateBr(merge_block);
    }
    
    // Continue with merge block
    builder_->SetInsertPoint(merge_block);
    return;
  }
  
  // Handle loop statements
  if (const auto* loop_stmt = dynamic_cast<const LoopStatement*>(node)) {
    if (loop_stmt->is_range_loop()) {
      generate_range_loop(loop_stmt, function);
    } else {
      generate_conditional_loop(loop_stmt, function);
    }
    return;
  }
  
  throw std::runtime_error("Unknown statement type");
}

void CodeGenerator::generate_range_loop(const LoopStatement* loop_stmt, llvm::Function* function) {
  // Get the range expression
  const auto* range = dynamic_cast<const RangeExpression*>(loop_stmt->range());
  if (!range) {
    throw std::runtime_error("Expected range expression in range loop");
  }
  
  // Generate start and end values
  llvm::Value* start_val = generate_expression(range->start());
  llvm::Value* end_val = generate_expression(range->end());
  
  // Create basic blocks
  llvm::BasicBlock* loop_cond = llvm::BasicBlock::Create(*context_, "loop.cond", function);
  llvm::BasicBlock* loop_body = llvm::BasicBlock::Create(*context_, "loop.body", function);
  llvm::BasicBlock* loop_end = llvm::BasicBlock::Create(*context_, "loop.end", function);
  
  // Create loop variable (allocate space for iterator)
  llvm::AllocaInst* loop_var = builder_->CreateAlloca(
      llvm::Type::getInt32Ty(*context_), nullptr, loop_stmt->variable_name());
  
  // Initialize loop variable with start value
  builder_->CreateStore(start_val, loop_var);
  
  // Store the loop variable for use in the loop body
  local_variables_[loop_stmt->variable_name()] = loop_var;
  
  // Jump to condition check
  builder_->CreateBr(loop_cond);
  
  // Generate condition block: check if i < end
  builder_->SetInsertPoint(loop_cond);
  llvm::Value* current_val = builder_->CreateLoad(llvm::Type::getInt32Ty(*context_), loop_var);
  llvm::Value* condition = builder_->CreateICmpSLT(current_val, end_val, "loopcond");
  builder_->CreateCondBr(condition, loop_body, loop_end);
  
  // Generate loop body
  builder_->SetInsertPoint(loop_body);
  for (const auto& stmt : loop_stmt->body()) {
    generate_statement(stmt.get(), function);
  }
  
  // Increment loop variable: i = i + 1
  llvm::Value* current_val_body = builder_->CreateLoad(llvm::Type::getInt32Ty(*context_), loop_var);
  llvm::Value* one = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context_), 1);
  llvm::Value* incremented = builder_->CreateAdd(current_val_body, one, "inc");
  builder_->CreateStore(incremented, loop_var);
  
  // Jump back to condition
  builder_->CreateBr(loop_cond);
  
  // Continue after loop
  builder_->SetInsertPoint(loop_end);
  
  // Remove loop variable from scope
  local_variables_.erase(loop_stmt->variable_name());
}

void CodeGenerator::generate_conditional_loop(const LoopStatement* loop_stmt, llvm::Function* function) {
  // Create basic blocks
  llvm::BasicBlock* loop_cond = llvm::BasicBlock::Create(*context_, "loop.cond", function);
  llvm::BasicBlock* loop_body = llvm::BasicBlock::Create(*context_, "loop.body", function);
  llvm::BasicBlock* loop_end = llvm::BasicBlock::Create(*context_, "loop.end", function);
  
  // Jump to condition check
  builder_->CreateBr(loop_cond);
  
  // Generate condition block
  builder_->SetInsertPoint(loop_cond);
  llvm::Value* condition = generate_expression(loop_stmt->condition());
  builder_->CreateCondBr(condition, loop_body, loop_end);
  
  // Generate loop body
  builder_->SetInsertPoint(loop_body);
  for (const auto& stmt : loop_stmt->body()) {
    generate_statement(stmt.get(), function);
  }
  
  // Jump back to condition
  builder_->CreateBr(loop_cond);
  
  // Continue after loop
  builder_->SetInsertPoint(loop_end);
}

bool CodeGenerator::is_function_pointer_type(const std::string& type_str) {
  return type_str.starts_with("fn(");
}

llvm::Type* CodeGenerator::get_llvm_type_from_string(const std::string& type_str) {
  if (type_str == "i8") {
    return llvm::Type::getInt8Ty(*context_);
  } else if (type_str == "i16") {
    return llvm::Type::getInt16Ty(*context_);
  } else if (type_str == "i32") {
    return llvm::Type::getInt32Ty(*context_);
  } else if (type_str == "i64") {
    return llvm::Type::getInt64Ty(*context_);
  } else if (type_str == "u8") {
    return llvm::Type::getInt8Ty(*context_);
  } else if (type_str == "u16") {
    return llvm::Type::getInt16Ty(*context_);
  } else if (type_str == "u32") {
    return llvm::Type::getInt32Ty(*context_);
  } else if (type_str == "u64") {
    return llvm::Type::getInt64Ty(*context_);
  } else if (type_str == "bool") {
    return llvm::Type::getInt1Ty(*context_);
  } else if (type_str == "string" || type_str == "const string") {
    return llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0);
  } else if (is_function_pointer_type(type_str)) {
    // Parse function pointer type and create LLVM function pointer type
    FunctionType func_type = parse_function_type(type_str);
    
    // Convert parameter types
    std::vector<llvm::Type*> param_types;
    for (const auto& param_type : func_type.param_types()) {
      param_types.push_back(get_llvm_type_from_string(param_type));
    }
    
    // Convert return type
    llvm::Type* return_type = get_llvm_type_from_string(func_type.return_type());
    
    // Create function type and return pointer to it
    llvm::FunctionType* llvm_func_type = llvm::FunctionType::get(return_type, param_types, false);
    return llvm::PointerType::get(llvm_func_type, 0);
  } else {
    throw std::runtime_error("Unsupported type: " + type_str);
  }
}

FunctionType CodeGenerator::parse_function_type(const std::string& type_str) {
  // Parse "fn(param1, param2) -> return_type"
  if (!type_str.starts_with("fn(")) {
    throw std::runtime_error("Invalid function type format: " + type_str);
  }
  
  // Find the parameter list
  size_t params_start = 3; // After "fn("
  size_t params_end = type_str.find(')');
  if (params_end == std::string::npos) {
    throw std::runtime_error("Missing ')' in function type: " + type_str);
  }
  
  // Extract parameter types
  std::vector<std::string> param_types;
  if (params_end > params_start) {
    std::string params_str = type_str.substr(params_start, params_end - params_start);
    
    // Simple parsing - split by comma (assumes no nested function types for now)
    size_t start = 0;
    while (start < params_str.length()) {
      size_t comma_pos = params_str.find(", ", start);
      if (comma_pos == std::string::npos) {
        param_types.push_back(params_str.substr(start));
        break;
      } else {
        param_types.push_back(params_str.substr(start, comma_pos - start));
        start = comma_pos + 2;
      }
    }
  }
  
  // Find return type
  size_t arrow_pos = type_str.find(" -> ");
  if (arrow_pos == std::string::npos) {
    throw std::runtime_error("Missing ' -> ' in function type: " + type_str);
  }
  
  std::string return_type = type_str.substr(arrow_pos + 4);
  
  return {std::move(param_types), std::move(return_type)};
}

llvm::Value* CodeGenerator::generate_anonymous_function(const AnonymousFunction* anon_func) {
  // Generate a unique name for the anonymous function
  static int anon_counter = 0;
  std::string func_name = "anon_" + std::to_string(anon_counter++);
  
  // Create parameter types
  std::vector<llvm::Type*> param_types;
  for (const auto& param : anon_func->parameters()) {
    (void)param;  // Mark as used to avoid warning
    // For now, assume all parameters are i32 (can be enhanced later with proper type system)
    param_types.push_back(llvm::Type::getInt32Ty(*context_));
  }

  // Create function type
  llvm::Type* return_type;
  if (anon_func->return_type() == "void" || anon_func->return_type() == "nil") {
    return_type = llvm::Type::getVoidTy(*context_);
  } else {
    return_type = llvm::Type::getInt32Ty(*context_);
  }
  
  llvm::FunctionType* func_type =
      llvm::FunctionType::get(return_type, param_types, false);

  // Create function
  llvm::Function* function =
      llvm::Function::Create(func_type, llvm::Function::InternalLinkage,
                             func_name, module_.get());

  // Set parameter names
  size_t idx = 0;
  for (auto& arg : function->args()) {
    arg.setName(anon_func->parameters()[idx++]->name());
  }

  // Save current insertion point and local state
  llvm::BasicBlock* current_block = builder_->GetInsertBlock();
  auto saved_params = function_params_;
  auto saved_locals = local_variables_;
  auto saved_return_type = current_function_return_type_;

  // Create basic block for anonymous function
  llvm::BasicBlock* entry =
      llvm::BasicBlock::Create(*context_, "entry", function);
  builder_->SetInsertPoint(entry);

  // Clear and set up parameter allocas for anonymous function
  function_params_.clear();
  local_variables_.clear();
  current_function_return_type_ = anon_func->return_type();
  
  for (auto& arg : function->args()) {
    llvm::AllocaInst* alloca = builder_->CreateAlloca(
        llvm::Type::getInt32Ty(*context_), nullptr, arg.getName());
    builder_->CreateStore(&arg, alloca);
    function_params_[std::string(arg.getName())] = alloca;
  }

  // Generate function body
  for (const auto& stmt : anon_func->body()) {
    generate_statement(stmt.get(), function);
  }
  
  // If this is a void/nil function and there's no terminator, add a void return
  if ((anon_func->return_type() == "void" || anon_func->return_type() == "nil") && 
      !builder_->GetInsertBlock()->getTerminator()) {
    builder_->CreateRetVoid();
  }

  // Restore previous state
  builder_->SetInsertPoint(current_block);
  function_params_ = saved_params;
  local_variables_ = saved_locals;
  current_function_return_type_ = saved_return_type;

  // Return the function as a value (function pointer)
  return function;
}

}  // namespace void_compiler
