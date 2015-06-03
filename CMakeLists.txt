cmake_minimum_required (VERSION 2.6)

project (bplustree)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(LIB_BPLUSTREE_NAME bplustree)

add_subdirectory(lib)
add_subdirectory(tests)
