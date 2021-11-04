#!/bin/bash
export HOME=/root &&\
ls -l / &&\
conan install . -if builddir --build outdated -pr=ci/profile_ci &&\
CC=clang CXX=clang++ cmake -G Ninja -DUSE_JEMALLOC=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-flto=full" -DCMAKE_C_FLAGS="-flto=full" -S . -B builddir &&\
ninja -C builddir cppmon
mkdir binaries
cp builddir/bin/cppmon binaries
