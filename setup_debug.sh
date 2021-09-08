#!/bin/bash
conan install . -if builddir_debug --build=missing
PKG_CONFIG_PATH=builddir_debug CXX=clang++ meson setup --buildtype=debug -Dwithsan=true builddir_debug
