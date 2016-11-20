#!/bin/sh
set -e
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DCMAKE_CXX_FLAGS="-std=c++14 ${CMAKE_CXX_FLAGS}" \
  ..
make -j
cd ..