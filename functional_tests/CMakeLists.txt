cmake_minimum_required(VERSION 3.2)
project(functional_tests C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED on)

include_directories(${CPPUTEST_INCLUDE_DIR})

set(NANOARQ_FUNCTIONAL_TEST_SOURCES main.cpp
                                    nanoarq_in_test_project.c
                                    write_and_read_frames.cpp
                                    decode_sent_frames.cpp)

set_source_files_properties(nanoarq_in_test_project.c PROPERTIES COMPILE_FLAGS ${NANOARQ_FLAGS})

add_executable(nanoarq_functional_tests ${NANOARQ_FUNCTIONAL_TEST_SOURCES})
target_compile_options(nanoarq_functional_tests PRIVATE ${NANOARQ_COMMON_FLAGS})
target_link_libraries(nanoarq_functional_tests libCppUTest libCppUTestExt)
nanoarq_add_test(nanoarq_functional_tests nanoarq_functional_tests)

