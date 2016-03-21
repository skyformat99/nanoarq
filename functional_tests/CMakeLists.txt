cmake_minimum_required(VERSION 3.4)
project(functional_tests C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED on)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

include_directories(${CPPUTEST_INCLUDE_DIR})

set(NANOARQ_FUNCTIONAL_TEST_SOURCES main.cpp
                                    functional_tests.h
                                    nanoarq_functional_test.h
                                    nanoarq_functional_test.c
                                    frame_serialization.cpp
                                    send_full_window.cpp
                                    send_10mb_through_window.cpp
                                    recv_full_window.cpp
                                    recv_10mb_through_window.cpp)

string(REPLACE ";" " " NANOARQ_FLAGS_STR "${NANOARQ_FLAGS}")
set_source_files_properties(nanoarq_in_test_project.c PROPERTIES COMPILE_FLAGS "${NANOARQ_FLAGS_STR}")

add_executable(nanoarq_functional_tests ${NANOARQ_FUNCTIONAL_TEST_SOURCES})
target_compile_options(nanoarq_functional_tests PRIVATE ${ARQ_COMMON_FLAGS})
add_dependencies(nanoarq_functional_tests CppUTest_external all_unit_tests)
target_link_libraries(nanoarq_functional_tests libCppUTest libCppUTestExt)
nanoarq_add_test(nanoarq_functional_tests)

