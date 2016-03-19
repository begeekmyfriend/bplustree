#!/bin/sh
rm -rf build
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Test ..
make
./bin/bplustree_test
