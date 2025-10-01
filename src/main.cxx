#include <iostream>
#include <string>

#include "compiler.h"

int main() {
  std::string source = R"(
const calculate = fn(x: i32, y: i32, z: i32) -> i32 {
  return x + y * z - x / y
}

const main = fn() -> i32 {
  return calculate(10, 5, 3)
}
)";
  void_compiler::Compiler compiler;

  // Compile and run with JIT
  std::cout << "=== JIT Execution ===" << '\n';
  int result = compiler.compile_and_run(source);
  std::cout << "Program returned: " << result << '\n';
  std::cout << '\n';

  // Compile to executable
  std::cout << "=== Compiling to Executable ===" << '\n';
  if (compiler.compile_to_executable(source, "hello_void")) {
    std::cout << "Success! Run with: ./hello_void" << '\n';
  }
}
