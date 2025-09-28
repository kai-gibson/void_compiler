#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

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

// Lexer
class Lexer {
 public:
  explicit Lexer(const std::string& source)
      : source_(source), position_(0), line_(1), column_(1) {}

  Token next_token() {
    skip_whitespace();

    if (current_char() == '\0') {
      return {TokenType::EndOfFile, "", line_, column_};
    }

    if (std::isdigit(current_char())) {
      return {TokenType::Number, read_number(), line_, column_};
    }

    if (std::isalpha(current_char()) || current_char() == '_') {
      std::string identifier = read_identifier();

      // Keywords
      if (identifier == "const") {
        return {TokenType::Const, identifier, line_, column_};
      }
      if (identifier == "fn") {
        return {TokenType::Fn, identifier, line_, column_};
      }
      if (identifier == "return") {
        return {TokenType::Return, identifier, line_, column_};
      }
      if (identifier == "i32") {
        return {TokenType::I32, identifier, line_, column_};
      }

      return {TokenType::Identifier, identifier, line_, column_};
    }

    char ch = current_char();
    int start_column = column_;
    advance();

    switch (ch) {
      case '=':
        return {TokenType::Equals, "=", line_, start_column};
      case '(':
        return {TokenType::LParen, "(", line_, start_column};
      case ')':
        return {TokenType::RParen, ")", line_, start_column};
      case '{':
        return {TokenType::LBrace, "{", line_, start_column};
      case '}':
        return {TokenType::RBrace, "}", line_, start_column};
      case '-':
        if (current_char() == '>') {
          advance();
          return {TokenType::Arrow, "->", line_, start_column};
        }
        break;
    }

    throw std::runtime_error("Unknown character: " + std::string(1, ch));
  }

 private:
  char current_char() const {
    if (position_ >= source_.length()) return '\0';
    return source_[position_];
  }

  void advance() {
    if (current_char() == '\n') {
      line_++;
      column_ = 1;
    } else {
      column_++;
    }
    position_++;
  }

  void skip_whitespace() {
    while (current_char() == ' ' || current_char() == '\t' ||
           current_char() == '\n' || current_char() == '\r') {
      advance();
    }
  }

  std::string read_identifier() {
    std::string result;
    while (std::isalnum(current_char()) || current_char() == '_') {
      result += current_char();
      advance();
    }
    return result;
  }

  std::string read_number() {
    std::string result;
    while (std::isdigit(current_char())) {
      result += current_char();
      advance();
    }
    return result;
  }

  std::string source_;
  size_t position_;
  int line_;
  int column_;
};

// Parser
class Parser {
 public:
  explicit Parser(std::vector<Token> tokens)
      : tokens_(std::move(tokens)), current_(0) {}

  std::unique_ptr<FunctionDeclaration> parse() { return parse_function(); }

 private:
  Token& peek() {
    if (current_ >= tokens_.size()) {
      throw std::runtime_error("Unexpected end of input");
    }
    return tokens_[current_];
  }

  Token consume(TokenType expected) {
    if (peek().type != expected) {
      throw std::runtime_error("Expected token type, got: " + peek().value);
    }
    return tokens_[current_++];
  }

  bool match(TokenType type) const {
    if (current_ >= tokens_.size()) return false;
    return tokens_[current_].type == type;
  }

  std::unique_ptr<ASTNode> parse_expression() {
    if (match(TokenType::Number)) {
      int value = std::stoi(consume(TokenType::Number).value);
      return std::make_unique<NumberLiteral>(value);
    }

    throw std::runtime_error("Expected expression");
  }

  std::unique_ptr<ASTNode> parse_statement() {
    if (match(TokenType::Return)) {
      consume(TokenType::Return);
      auto expr = parse_expression();
      return std::make_unique<ReturnStatement>(std::move(expr));
    }

    throw std::runtime_error("Expected statement");
  }

  std::unique_ptr<FunctionDeclaration> parse_function() {
    consume(TokenType::Const);
    std::string name = consume(TokenType::Identifier).value;
    consume(TokenType::Equals);
    consume(TokenType::Fn);
    consume(TokenType::LParen);
    consume(TokenType::RParen);
    consume(TokenType::Arrow);
    std::string return_type = consume(TokenType::I32).value;
    consume(TokenType::LBrace);

    auto func = std::make_unique<FunctionDeclaration>(name, return_type);

    while (!match(TokenType::RBrace)) {
      func->add_statement(parse_statement());
    }

    consume(TokenType::RBrace);
    return func;
  }

  std::vector<Token> tokens_;
  size_t current_;
};

// Code Generator
class CodeGenerator {
 public:
  CodeGenerator() {
    context_ = std::make_unique<llvm::LLVMContext>();
    module_ = std::make_unique<llvm::Module>("void_module", *context_);
    builder_ = std::make_unique<llvm::IRBuilder<>>(*context_);
  }

  void generate_function(const FunctionDeclaration* func_decl) {
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

  void print_ir() const { module_->print(llvm::outs(), nullptr); }

  bool compile_to_object(const std::string& filename) {
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
    auto target_machine = target->createTargetMachine(
        target_triple, cpu, features, opt, relocModel);

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

  int run_jit() {
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
      throw std::runtime_error("Failed to create execution engine: " +
                               error_str);
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

 private:
  llvm::Value* generate_expression(const ASTNode* node) {
    if (const auto* num = dynamic_cast<const NumberLiteral*>(node)) {
      return llvm::ConstantInt::get(*context_,
                                    llvm::APInt(32, num->value(), true));
    }

    throw std::runtime_error("Unknown expression type");
  }

  void generate_statement(const ASTNode* node, llvm::Function* function) {
    (void)function;
    if (const auto* ret = dynamic_cast<const ReturnStatement*>(node)) {
      llvm::Value* ret_val = generate_expression(ret->expression());
      builder_->CreateRet(ret_val);
    }
  }

  std::unique_ptr<llvm::LLVMContext> context_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilder<>> builder_;
};

// Compiler class that ties everything together
class VoidCompiler {
 public:
  int compile_and_run(const std::string& source) {
    try {
      auto ast = compile_source(source);

      // Generate code
      CodeGenerator codegen;
      codegen.generate_function(ast.get());

      std::cout << "Generated LLVM IR:" << std::endl;
      codegen.print_ir();
      std::cout << std::endl;

      // Run with JIT
      return codegen.run_jit();

    } catch (const std::exception& e) {
      std::cerr << "Error: " << e.what() << std::endl;
      return -1;
    }
  }

  bool compile_to_executable(const std::string& source,
                             const std::string& output_name) {
    try {
      auto ast = compile_source(source);

      // Generate code
      CodeGenerator codegen;
      codegen.generate_function(ast.get());

      std::cout << "Generated LLVM IR:" << std::endl;
      codegen.print_ir();
      std::cout << std::endl;

      // Compile to object file
      std::string obj_file = output_name + ".o";
      if (!codegen.compile_to_object(obj_file)) {
        return false;
      }

      // Link to executable
      std::string link_cmd = "clang " + obj_file + " -o " + output_name;
      std::cout << "Linking: " << link_cmd << std::endl;

      int result = system(link_cmd.c_str());
      if (result != 0) {
        std::cerr << "Linking failed" << std::endl;
        return false;
      }

      // Clean up object file
      std::remove(obj_file.c_str());

      std::cout << "Executable created: " << output_name << std::endl;
      return true;

    } catch (const std::exception& e) {
      std::cerr << "Error: " << e.what() << std::endl;
      return false;
    }
  }

 private:
  std::unique_ptr<FunctionDeclaration> compile_source(
      const std::string& source) {
    // Lex
    Lexer lexer(source);
    std::vector<Token> tokens;

    Token token;
    do {
      token = lexer.next_token();
      tokens.push_back(token);
    } while (token.type != TokenType::EndOfFile);

    // Parse
    Parser parser(std::move(tokens));
    return parser.parse();
  }
};

}  // namespace void_compiler

int main() {
  std::string source = R"(
const main = fn() -> i32 {
    return 1
}
)";

  void_compiler::VoidCompiler compiler;

  // Compile and run with JIT
  std::cout << "=== JIT Execution ===" << std::endl;
  int result = compiler.compile_and_run(source);
  std::cout << "Program returned: " << result << std::endl;
  std::cout << std::endl;

  // Compile to executable
  std::cout << "=== Compiling to Executable ===" << std::endl;
  if (compiler.compile_to_executable(source, "hello_void")) {
    std::cout << "Success! Run with: ./hello_void" << std::endl;
  }

  return 0;
}
