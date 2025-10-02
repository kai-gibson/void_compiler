#include <cstdlib>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

#include "compiler.h"

int main(int argc, char** argv) {
  enum class Tool : uint8_t {
    Build,
    Dev,
  };

  Tool tool {};
  std::string filename;

  if (argc == 1) {
    tool = Tool::Dev;
  } else if (argc == 3 && std::string(argv[1]) == "build") {
    tool = Tool::Build;
    filename = argv[2];
  } else {
    std::cerr << "Usage: " << argv[0] << " [build <source_file>]" << '\n';
    return 1;
  }

  if (tool == Tool::Dev) {
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
    auto source_code = void_compiler::SourcePath{.path = source};
    if (compiler.compile_to_executable(source_code,
                                      void_compiler::OutputPath{"hello_void"})) {
      std::cout << "Success! Run with: ./hello_void" << '\n';
    }
  } else {
    std::ifstream file(filename);
    std::ostringstream buf;
    buf << file.rdbuf();          // read everything (whitespace included)
    std::string source = buf.str();

    std::cout << "source: " << source << '\n';

    void_compiler::Compiler compiler;
    if (compiler.compile_to_executable(
            void_compiler::SourcePath{.path = source},
            void_compiler::OutputPath{"a.out"})) {
      std::cout << "Success! Run with: ./a.out" << '\n';
    }
  }
}