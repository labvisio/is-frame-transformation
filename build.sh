#!/bin/bash

set -e 

mkdir -p build && cd build
conan install .. -s is-frame-conversion:build_type=Debug  -s compiler.libcxx=libstdc++11 \
  --build=missing -o build_tests=True
conan build ..
