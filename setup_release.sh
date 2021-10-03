#!/bin/bash
conan install . -if builddir --build=missing
cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS="-flto=full -march=native" -DCMAKE_C_FLAGS="-flto=full -march=native" -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=lld" -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" -S . -B builddir
