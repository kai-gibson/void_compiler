# Test CMakeLists.txt

# Create a library with the compiler sources (excluding main.cxx)
add_library(void_compiler_lib
  ../src/lexer.cxx
  ../src/parser.cxx
  ../src/code_generation.cxx
  ../src/compiler.cxx
  ../src/runtime.cxx
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

target_include_directories(void_compiler_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(void_compiler_lib ${llvm_libs})

# Test executable
add_executable(void_compiler_tests
  lexer_test.cpp
  parser_test.cpp
  integration_test.cpp
  code_generation_test.cpp
  test_main.cpp
  slice_tests.cpp
)

target_link_libraries(void_compiler_tests
  void_compiler_lib
  gtest
  gtest_main
)

target_include_directories(void_compiler_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(void_compiler_tests)
