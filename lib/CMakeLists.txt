set(LIB_BPLUSTREE_SRC bplustree.c)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_library(bplustree SHARED ${LIB_BPLUSTREE_SRC})
set_target_properties(bplustree PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(bplustree PROPERTIES VERSION 1.0 SOVERSION 1)
install(TARGETS bplustree LIBRARY DESTINATION ${LIBRARY_OUTPUT_PATH})

add_library(bplustree_static STATIC ${LIB_BPLUSTREE_SRC})
set_target_properties(bplustree_static PROPERTIES OUTPUT_NAME "bplustree")
set_target_properties(bplustree_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
install(TARGETS bplustree_static ARCHIVE DESTINATION ${LIBRARY_OUTPUT_PATH})
