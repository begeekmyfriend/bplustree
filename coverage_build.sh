#!/bin/sh
rm -rf build
mkdir build
echo "Please wait about 10 seconds for test case generation..."
python ./tests/testcase_generator.py
mv testcase build
cd build
cmake -DCMAKE_BUILD_TYPE=Coverage ..
make coverage
firefox coverage/index.html
