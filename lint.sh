#!/bin/sh

run-clang-tidy -p build/ -header-filter='^include/' src/*.cxx include/*.h -j 8
