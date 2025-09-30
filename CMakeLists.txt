cmake_minimum_required(VERSION 3.16)
project(VoidCompiler VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Suppress deprecation warnings from LLVM
add_compile_definitions(_LIBCPP_ENABLE_CXX17_REMOVED_FEATURES)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -Wno-unused-parameter")

# Find LLVM
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

# Include LLVM headers and definitions
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST UNIX_COMMAND "${LLVM_DEFINITIONS}")
add_definitions(${LLVM_DEFINITIONS_LIST})

# Find required LLVM components (simplified - only what we actually need)
llvm_map_components_to_libnames(llvm_libs 
    core 
    executionengine 
    mcjit 
    native
    x86codegen
)

# Remove duplicates
list(REMOVE_DUPLICATES llvm_libs)

# Create the executable
add_executable(void_compiler 
  src/main.cxx 
  src/lexer.cxx
  src/parser.cxx
  src/code_generation.cxx
  src/compiler.cxx
)

# Link LLVM libraries
target_link_libraries(void_compiler ${llvm_libs})

target_include_directories(void_compiler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Compiler flags
target_compile_options(void_compiler PRIVATE -Wall -Wextra -Wpedantic)

# Google Test setup
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()
add_compile_options(-fprofile-instr-generate -fcoverage-mapping -O0 -g)
add_link_options(-fprofile-instr-generate -fcoverage-mapping)

# Add tests subdirectory
add_subdirectory(tests)
