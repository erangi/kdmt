project(min_bench)

set(SOURCE_FILES main.cpp Timer.h)

add_executable(min_bench ${SOURCE_FILES})

target_link_libraries(min_bench PRIVATE kdmt_lib)
