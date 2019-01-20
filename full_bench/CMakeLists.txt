project(Full_bench)

set(SOURCE_FILES benchmarks.cpp InputProvider.cpp InputProvider.h)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../datasets/2.5M\ keys.csv DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/datasets)

add_executable(full_bench ${SOURCE_FILES})

#find_package(Threads)
#target_link_libraries(full_bench PRIVATE kdmt_lib benchmark ${CMAKE_THREAD_LIBS_INIT} rt)
target_link_libraries(full_bench PRIVATE kdmt_lib benchmark)