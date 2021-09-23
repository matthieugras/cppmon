#!/bin/bash
conan install . -if builddir_debug --build=missing
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined" -DCMAKE_C_FLAGS="-fsanitize=address,undefined" -S . -B builddir_debug
