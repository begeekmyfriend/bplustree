set(LIB_BPLUSTREE_SRC bplustree.c)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

message(STATUS "That is SOURCE dir" ${PROJECT_SOURCE_DIR})
message(STATUS "That is BINARY dir" ${PROJECT_BINARY_DIR})

add_library(bplustree SHARED ${LIB_BPLUSTREE_SRC})
add_library(bplustree_static STATIC ${LIB_BPLUSTREE_SRC})

set_target_properties(bplustree_static PROPERTIES OUTPUT_NAME "bplustree")
get_target_property(OUTPUT_VALUE bplustree_static OUTPUT_NAME)
message(STATUS "That is the bplustree_static output name:" ${OUTPUT_VALUE})

set_target_properties(bplustree PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(bplustree_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

set_target_properties(bplustree PROPERTIES VERSION 1 SOVERSION 1)
