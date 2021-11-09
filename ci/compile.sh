#!/bin/bash
export HOME=/root &&\
ls -l / &&\
conan install . -if builddir --build outdated -pr=ci/profile_ci &&\
mkdir install &&\
CC=clang CXX=clang++ cmake -G Ninja -DCMAKE_INSTALL_PREFIX=$PWD/install -DUSE_JEMALLOC=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-flto=full" -DCMAKE_C_FLAGS="-flto=full" -S . -B builddir &&\
ninja -C builddir install/strip
