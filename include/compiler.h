#ifndef COMPILER_H
#define COMPILER_H
#include <string>

#include "types.h"

namespace void_compiler {

struct SourcePath {
  std::string path;
};

struct OutputPath {
  std::string path;
};

// compiler class to string together the lexer, parser, and code generator
class Compiler {
 public:
  int compile_and_run(const std::string& source);

  bool compile_to_executable(const SourcePath& source,
                             const OutputPath& output_name);

 private:
  std::unique_ptr<Program> compile_source(const std::string& source);
};
#endif  // COMPILER_H
}
