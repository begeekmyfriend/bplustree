set(LIB_BPLUSTREE_SRC bplustree.c)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_library(bplustree SHARED ${LIB_BPLUSTREE_SRC})
add_library(bplustree_static STATIC ${LIB_BPLUSTREE_SRC})

set_target_properties(bplustree_static PROPERTIES OUTPUT_NAME "bplustree")
get_target_property(OUTPUT_VALUE bplustree_static OUTPUT_NAME)

set_target_properties(bplustree PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(bplustree_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

set_target_properties(bplustree PROPERTIES VERSION 1 SOVERSION 1)
