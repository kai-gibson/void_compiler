#include <iostream>
#include <string>

#include "compiler.h"

int main() {
  std::string source = R"(

const add = fn() -> i32 {
  return 3
}

const add = fn() -> i32 {
  return 2
}

const main = fn() -> i32 {
  return 1
}
)";
  void_compiler::Compiler compiler;

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
}
