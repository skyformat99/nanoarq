cmake_minimum_required(VERSION 3.4)
project(functional_tests C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED on)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

include_directories(${CPPUTEST_INCLUDE_DIR})

set(NANOARQ_FUNCTIONAL_TEST_SOURCES main.cpp
                                    nanoarq_in_test_project.c
                                    write_and_read_frames.cpp
                                    manual_decode_full_send_window.cpp)

string(REPLACE ";" " " NANOARQ_FLAGS_STR "${NANOARQ_FLAGS}")
set_source_files_properties(nanoarq_in_test_project.c PROPERTIES COMPILE_FLAGS "${NANOARQ_FLAGS_STR}")

add_executable(nanoarq_functional_tests ${NANOARQ_FUNCTIONAL_TEST_SOURCES})
target_compile_options(nanoarq_functional_tests PRIVATE ${NANOARQ_COMMON_FLAGS})
target_link_libraries(nanoarq_functional_tests libCppUTest libCppUTestExt)
nanoarq_add_test(nanoarq_functional_tests nanoarq_functional_tests)

