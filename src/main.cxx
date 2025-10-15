#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "compiler.h"

int main(int argc, char** argv) {
  std::string filename;

  if (argc == 3 && std::string(argv[1]) == "build") {
    filename = argv[2];
  } else {
    std::cerr << "Usage: " << argv[0] << " [build <source_file>]" << '\n';
    return 1;
  }

  std::ifstream file(filename);
  std::ostringstream buf;
  buf << file.rdbuf();  // read everything (whitespace included)
  std::string source = buf.str();

  std::cout << "source: " << source << '\n';

  void_compiler::Compiler compiler;
  if (compiler.compile_to_executable(void_compiler::SourcePath{.path = source},
                                     void_compiler::OutputPath{"a.out"})) {
    std::cout << "Success! Run with: ./a.out" << '\n';
  }
}