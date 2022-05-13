# B+Tree
A minimal B+Tree implementation for millions (even billions) of key-value storage based on Posix.

## Branch
[in-memory](https://github.com/begeekmyfriend/bplustree/tree/in-memory) for learning and debugging.

## Demo
```shell
./demo_build.sh
```

## Code Coverage Test

**Note:** You need to `rm /tmp/coverage.index*` for this testing every time because the configuration (i.e block size and order etc.) in those index files is immutable!

```shell
./coverage_build.sh
```
