#include <gtest/gtest.h>

// Main function for running all tests
// This file exists so we can use gtest_main and have our own main if needed
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}