#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H
#include <memory>

#include "types.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#pragma clang diagnostic pop

namespace void_compiler {
// Code Generator
class CodeGenerator {
 public:
  CodeGenerator();
  void generate_program(const Program* program);
  void generate_function(const FunctionDeclaration* func_decl);
  void print_ir() const;
  bool compile_to_object(const std::string& filename);
  int run_jit();

 private:
  llvm::Value* generate_expression(const ASTNode* node);
  void generate_statement(const ASTNode* node, llvm::Function* function);

  std::unique_ptr<llvm::LLVMContext> context_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilder<>> builder_;
};

}  // namespace void_compiler
#endif  // CODE_GENERATOR_H
