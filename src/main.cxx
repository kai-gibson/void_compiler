#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "code_generation.h"
#include "lexer.h"
#include "parser.h"
#include "types.h"

namespace void_compiler {

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
