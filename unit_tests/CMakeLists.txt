cmake_minimum_required(VERSION 3.4)
project(unit_tests C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED on)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

include_directories(${CPPUTEST_INCLUDE_DIR})

add_library(nanoarq_test_support STATIC replace_arq_runtime_function.h
                                        replace_arq_runtime_function.cpp
                                        arq_mock_list.h
                                        arq_enable_mocks.h
                                        arq_runtime_mock_plugin.h
                                        arq_runtime_mock_plugin.cpp
                                        strict_mock_plugin.h
                                        strict_mock_plugin.cpp
                                        main.cpp)

target_compile_options(nanoarq_test_support PRIVATE ${NANOARQ_COMMON_FLAGS})
add_dependencies(nanoarq_test_support CppUTest_external)

add_library(nanoarq_unit_test_objs OBJECT test_mock_hooks.cpp
                                          test_succeeded.cpp
                                          test_send.cpp
                                          test_recv.cpp
                                          test_flush.cpp
                                          test_asserts.cpp
                                          test_lin_alloc.cpp
                                          test_frame_hdr.cpp
                                          test_ntoh_hton_le.cpp
                                          test_backend_send.cpp
                                          test_backend_recv.cpp
                                          test_backend_poll.cpp
                                          test_window.cpp
                                          test_send_window.cpp
                                          test_send_window_ptr.cpp
                                          test_send_frame.cpp
                                          test_send_poll.cpp
                                          test_recv_window.cpp
                                          test_recv_window_ptr.cpp
                                          test_recv_frame.cpp
                                          test_recv_poll.cpp
                                          test_frame.cpp
                                          test_cobs.cpp
                                          test_minmax.cpp
                                          test_saturated_subtraction.cpp
                                          test_crc32.cpp)

target_compile_options(nanoarq_unit_test_objs PRIVATE ${NANOARQ_COMMON_FLAGS})
add_dependencies(nanoarq_unit_test_objs CppUTest_external)

############# Nanoarq runtime for unit tests

set(NANOARQ_UNIT_TEST_SOURCES ${CMAKE_SOURCE_DIR}/arq.h nanoarq_unit_test.h nanoarq_unit_test.c)
string(REPLACE ";" " " NANOARQ_FLAGS_STR "${NANOARQ_FLAGS}")
set_source_files_properties(nanoarq_unit_test.c PROPERTIES COMPILE_FLAGS "${NANOARQ_FLAGS_STR}")

############# Unit tests

add_executable(nanoarq_unit_tests ${NANOARQ_UNIT_TEST_SOURCES} $<TARGET_OBJECTS:nanoarq_unit_test_objs>)
add_dependencies(nanoarq_unit_tests CppUTest_external)
target_compile_options(nanoarq_unit_tests PRIVATE ${NANOARQ_COMMON_FLAGS} -DARQ_ASSERTS_ENABLED=1)
target_link_libraries(nanoarq_unit_tests nanoarq_test_support libCppUTest libCppUTestExt)
if(CMAKE_GENERATOR STREQUAL Xcode)
    target_link_libraries(nanoarq_unit_tests c++)
endif()
nanoarq_add_test(nanoarq_unit_tests)

############# Unit tests without asserts enabled

add_executable(nanoarq_no_asserts_unit_tests ${NANOARQ_UNIT_TEST_SOURCES}
                                             $<TARGET_OBJECTS:nanoarq_unit_test_objs>)
add_dependencies(nanoarq_no_asserts_unit_tests CppUTest_external)
target_compile_options(nanoarq_no_asserts_unit_tests PRIVATE ${NANOARQ_COMMON_FLAGS} -DARQ_ASSERTS_ENABLED=0)
target_link_libraries(nanoarq_no_asserts_unit_tests nanoarq_test_support libCppUTest libCppUTestExt)
if(CMAKE_GENERATOR STREQUAL Xcode)
    target_link_libraries(nanoarq_no_asserts_unit_tests c++)
endif()
nanoarq_add_test(nanoarq_no_asserts_unit_tests)

############# Dummy target that depends on all unit tests

add_custom_target(all_unit_tests DEPENDS nanoarq_unit_tests nanoarq_no_asserts_unit_tests)

