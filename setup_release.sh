#!/bin/bash
conan install . -if builddir --build=missing
PKG_CONFIG_PATH=builddir CXX=clang++ meson setup --buildtype=release -Db_lto=true builddir
