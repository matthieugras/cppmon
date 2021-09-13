#!/bin/bash
conan install . -if builddir --build=missing
LDFLAGS=-static-libstdc++ PKG_CONFIG_PATH=builddir CXX=clang++ CC=clang meson setup --buildtype=release -Db_lto=true -Db_ndebug=true builddir
