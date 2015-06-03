B+Tree
======

A minimal B+Tree implementation for key-value storage.

General Test
------------

```shell
mkdir build
cd build
cmake ..
make
./bin/bplustree_test
```

Code Coverage Test
------------------

```shell
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Coverage ..
make coverage
```

And the code coverage report HTML file is in `./build/coverage/index.html`.
