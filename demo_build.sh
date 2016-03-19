#!/bin/sh
rm -rf build
mkdir build
cd build
cmake ..
make
./bin/bplustree_demo
