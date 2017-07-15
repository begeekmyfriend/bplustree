cmake_minimum_required (VERSION 2.6)

project (bplustree)

#set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(LIB_BPLUSTREE_NAME bplustree)

add_subdirectory(lib)
add_subdirectory(tests)
