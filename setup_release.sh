#!/bin/bash
conan install . -if builddir --build=missing
CXX_LD=lld CC_LD=lld LDFLAGS=-static-libstdc++ PKG_CONFIG_PATH=builddir CXX=clang++ CC=clang meson setup --buildtype=release -Db_lto=true -Db_ndebug=true builddir
