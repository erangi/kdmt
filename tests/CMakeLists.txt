project(kdmt_tests)

set(SOURCE_FILES TestsMain.cpp KeyDometTests.cpp)

add_executable(tests ${SOURCE_FILES})

target_link_libraries(tests PRIVATE kdmt_lib catch)