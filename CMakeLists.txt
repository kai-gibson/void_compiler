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
    support 
    core 
    executionengine 
    interpreter 
    mcjit 
    native
    analysis
    x86codegen
    x86asmparser
    x86desc
    x86info
)

# Create the executable
add_executable(void_compiler main.cxx)

# Link LLVM libraries
target_link_libraries(void_compiler ${llvm_libs})

# Compiler flags
target_compile_options(void_compiler PRIVATE -Wall -Wextra -Wpedantic)
