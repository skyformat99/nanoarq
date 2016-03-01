cmake_minimum_required(VERSION 3.2)
project(unit_tests C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED on)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

include_directories(${CPPUTEST_INCLUDE_DIR})

set(NANOARQ_UNIT_TEST_SOURCES ${CMAKE_SOURCE_DIR}/arq.h
                              nanoarq_unit_test.h
                              nanoarq_unit_test.c
                              nanoarq_mocks.h
                              nanoarq_mocks.cpp
                              nanoarq_hook_plugin.h
                              nanoarq_hook_plugin.cpp
                              test_mock_hooks.cpp
                              test_send.cpp
                              test_flush.cpp
                              test_asserts.cpp
                              test_lin_alloc.cpp
                              test_frame_hdr.cpp
                              test_ntoh_hton_le.cpp
                              test_backend_send.cpp
                              test_backend_recv.cpp
                              test_backend_poll.cpp
                              test_send_window.cpp
                              test_send_window_ptr.cpp
                              test_send_frame.cpp
                              test_send_poll.cpp
                              test_frame.cpp
                              test_cobs.cpp
                              test_min.cpp
                              test_saturated_subtraction.cpp
                              test_crc32.cpp
                              main.cpp)

string(REPLACE ";" " " NANOARQ_FLAGS_STR "${NANOARQ_FLAGS}")
set_source_files_properties(nanoarq_unit_test.c PROPERTIES COMPILE_FLAGS "${NANOARQ_FLAGS_STR}")

add_executable(nanoarq_unit_tests ${NANOARQ_UNIT_TEST_SOURCES})
target_compile_options(nanoarq_unit_tests PRIVATE ${NANOARQ_COMMON_FLAGS} -DARQ_ASSERTS_ENABLED=1)
target_link_libraries(nanoarq_unit_tests libCppUTest libCppUTestExt)
nanoarq_add_test(nanoarq_c99 nanoarq_unit_tests)

add_executable(nanoarq_no_asserts_unit_tests ${NANOARQ_UNIT_TEST_SOURCES})
target_compile_options(nanoarq_no_asserts_unit_tests PRIVATE ${NANOARQ_COMMON_FLAGS} -DARQ_ASSERTS_ENABLED=0)
target_link_libraries(nanoarq_no_asserts_unit_tests libCppUTest libCppUTestExt)
nanoarq_add_test(nanoarq_c99_no_asserts nanoarq_no_asserts_unit_tests)

