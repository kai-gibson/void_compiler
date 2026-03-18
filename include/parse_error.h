#ifndef PARSE_EXCEPTION
#define PARSE_EXCEPTION

#include <string>

#include "types.h"

namespace void_compiler {

/*
   This is an exception to be thrown if an error is found while processing
   void source code. It should not be caught; its sole purpose is to stop the
   application and return a friendly message to the user.
*/
class ParseError : public std::runtime_error {
 public:
  ParseError(std::string_view message)
      : std::runtime_error("Error: " + std::string(message)) {}

  ParseError(std::string_view message, const SourceLocation& loc)
      : std::runtime_error("Error: " + std::string(message) +
                           " at line: " + std::to_string(loc.line) +
                           ", column: " + std::to_string(loc.column)) {}

  ParseError(std::string_view message, const Token& token)
      : std::runtime_error("Error: " + std::string(message) +
                           " at line: " + std::to_string(token.line) +
                           ", column: " + std::to_string(token.column)) {}
};
}  // namespace void_compiler
#endif  // PARSE_EXCEPTION
