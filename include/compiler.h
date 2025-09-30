#ifndef COMPILER_H
#define COMPILER_H
#include <string>

#include "types.h"

namespace void_compiler {
// Compiler class that ties everything together
class Compiler {
 public:
  int compile_and_run(const std::string& source);

  bool compile_to_executable(const std::string& source,
                             const std::string& output_name);

 private:
  std::unique_ptr<FunctionDeclaration> compile_source(
      const std::string& source);
};
#endif  // COMPILER_H
}
