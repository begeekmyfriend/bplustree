B+Tree
======

A minimal B+Tree implementation for key-value storage.

Demo
----

```shell
mkdir build
cd build
cmake ..
make
./bin/bplustree_demo
```

General Test
------------

```shell
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Test ..
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

And the code coverage report HTML file will be seen in `./build/coverage/index.html`.
