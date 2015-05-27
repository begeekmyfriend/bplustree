set(SRC_LIST bplustree_test.c)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

set(INC_DIR "../lib")
include_directories(${INC_DIR})

message(STATUS "This is SOURCE dir" ${PROJECT_SOURCE_DIR})
message(STATUS "This is BINARY dir" ${PROJECT_BINARY_DIR})

add_executable(test ${SRC_LIST})
target_link_libraries(test bplustree)
