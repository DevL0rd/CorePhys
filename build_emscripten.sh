#!/usr/bin/env bash

# source emsdk_env.sh

# Use this to build corephys on any system with a bash shell
rm -rf build
mkdir build
cd build
emcmake cmake -DCOREPHYS_VALIDATE=OFF -DCOREPHYS_UNIT_TESTS=ON -DCOREPHYS_SAMPLES=OFF -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
