#!/bin/bash
conan install . -if builddir_debug --build -pr profile_debug
CXX=clang++ CC=clang cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -D_GLIBCXX_DEBUG" -DCMAKE_C_FLAGS="-fsanitize=address,undefined" -S . -B builddir_debug
