#include "code_generation.h"

#include <iostream>

namespace void_compiler {
CodeGenerator::CodeGenerator() {
  context_ = std::make_unique<llvm::LLVMContext>();
  module_ = std::make_unique<llvm::Module>("void_module", *context_);
  builder_ = std::make_unique<llvm::IRBuilder<>>(*context_);
}

void CodeGenerator::generate_program(const Program* program) {
  for (const auto& func : program->functions()) {
    generate_function(func.get());
  }
}

void CodeGenerator::generate_function(const FunctionDeclaration* func_decl) {
  // Create function type
  llvm::Type* return_type = llvm::Type::getInt32Ty(*context_);
  llvm::FunctionType* func_type = llvm::FunctionType::get(return_type, false);

  // Create function
  llvm::Function* function =
      llvm::Function::Create(func_type, llvm::Function::ExternalLinkage,
                             func_decl->name(), module_.get());

  // Create basic block
  llvm::BasicBlock* entry =
      llvm::BasicBlock::Create(*context_, "entry", function);
  builder_->SetInsertPoint(entry);

  // Generate function body
  for (const auto& stmt : func_decl->body()) {
    generate_statement(stmt.get(), function);
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
    std::cerr << "Error: " << error << std::endl;
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
    std::cerr << "Could not open file: " << error_code.message() << std::endl;
    return false;
  }

  // Generate object file
  llvm::legacy::PassManager pass;
  auto file_type = llvm::CodeGenFileType::ObjectFile;

  if (target_machine->addPassesToEmitFile(pass, dest, nullptr, file_type)) {
    std::cerr << "TargetMachine can't emit a file of this type" << std::endl;
    return false;
  }

  pass.run(*module_);
  dest.flush();

  std::cout << "Object file written to " << filename << std::endl;
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

  throw std::runtime_error("Unknown expression type");
}

void CodeGenerator::generate_statement(const ASTNode* node,
                                       llvm::Function* function) {
  (void)function;
  if (const auto* ret = dynamic_cast<const ReturnStatement*>(node)) {
    llvm::Value* ret_val = generate_expression(ret->expression());
    builder_->CreateRet(ret_val);
  }
}

}  // namespace void_compiler
