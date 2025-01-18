#!/usr/bin/bash

BUILD_TYPE="RELEASE"
CXX_COMPILER="g++-12"

rm -fr build
cmake -S .. -B build -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" -DCMAKE_CXX_COMPILER="${CXX_COMPILER}"
cmake --build build --target perf_test
