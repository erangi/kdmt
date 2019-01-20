project(Kdmt_lib)

add_library(kdmt_lib INTERFACE)

target_sources(kdmt_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/KeyDomet.h)
target_include_directories(kdmt_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})