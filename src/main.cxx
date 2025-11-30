#include <cstdlib>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

#include "compiler.h"
#include "lexer.h"
#include "types.h"

/* TODO: remove this from main */
// outside namespace so main can use it
std::ostream& operator<<(std::ostream& os,
                         void_compiler::TokenType token_type) {
  return os << void_compiler::STRING_TOKEN_TYPES[int(token_type)];
}

std::ostream& operator<<(std::ostream& os, void_compiler::Token& token) {
  return os << "Token { " << "  .type = " << token.type << ' '
            << "  .value = \"" << token.value << "\" "
            << "  .line = " << token.line << ' '
            << "  .column = " << token.column << ' ' << "}";
}

std::string read_file(const std::string& filename) {
  std::ifstream file(filename);
  std::ostringstream buf;
  buf << file.rdbuf();  // read everything (whitespace included)
  std::string source = buf.str();
  return source;
}

int main(int argc, char** argv) {
  std::string filename;
  enum class Command : uint8_t {
    Build,
    Tokenise,
    // Parse,
  };
  Command command;

  if (argc == 3 && std::string(argv[1]) == "build") {
    command = Command::Build;
    filename = argv[2];
  } else if (argc == 3 && std::string(argv[1]) == "tokenise") {
    command = Command::Tokenise;
    filename = argv[2];
  } else {
    std::cerr << "Usage: " << argv[0] << " build <source_file>" << '\n';
    return 1;
  }

  switch (command) {
    case Command::Build: {
      auto source = read_file(filename);
      std::cout << "source: " << source << '\n';

      void_compiler::Compiler compiler;
      if (compiler.compile_to_executable(
              void_compiler::SourcePath{.path = source},
              void_compiler::OutputPath{"a.out"})) {
        std::cout << "Success! Run with: ./a.out" << '\n';
      }
      break;
    }
    case Command::Tokenise: {
      auto source = read_file(filename);
      std::cout << "source: " << source << '\n';
      void_compiler::Lexer lexer(source);
      std::vector<void_compiler::Token> tokens;

      void_compiler::Token token;
      do {
        token = lexer.next_token();
        tokens.push_back(token);
      } while (token.type != void_compiler::TokenType::EndOfFile);

      for (auto& token : tokens) {
        std::cout << token << '\n';
      }
      break;
    }
  }
}
