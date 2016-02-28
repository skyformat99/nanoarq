cmake_minimum_required(VERSION 3.2)
project(unit_tests C CXX)

################ nanoarq libs for testing

set(NANOARQ_COMMON_SOURCES ${CMAKE_SOURCE_DIR}/arq.h nanoarq_in_test_project.h)

add_library(nanoarq_c90 STATIC ${NANOARQ_COMMON_SOURCES} nanoarq_in_c90_test_project.c)
target_compile_options(nanoarq_c90 PRIVATE
                       ${NANOARQ_FLAGS} ${NANOARQ_COMMON_FLAGS} -std=c90 -DARQ_ASSERTS_ENABLED=1)

add_library(nanoarq_c99 STATIC ${NANOARQ_COMMON_SOURCES} nanoarq_in_c99_test_project.c)
target_compile_options(nanoarq_c99 PRIVATE
                       ${NANOARQ_FLAGS} ${NANOARQ_COMMON_FLAGS} -std=c99 -DARQ_ASSERTS_ENABLED=1)

add_library(nanoarq_c99_no_asserts STATIC ${NANOARQ_COMMON_SOURCES} nanoarq_in_c99_test_project.c)
target_compile_options(nanoarq_c99_no_asserts PRIVATE
                       ${NANOARQ_FLAGS} ${NANOARQ_COMMON_FLAGS} -std=c99 -DARQ_ASSERTS_ENABLED=0)

add_library(nanoarq_cpp STATIC ${NANOARQ_COMMON_SOURCES} nanoarq_in_cpp_test_project.cpp)
target_compile_options(nanoarq_cpp PRIVATE ${NANOARQ_FLAGS} ${NANOARQ_COMMON_FLAGS} -DARQ_ASSERTS_ENABLED=1)

################ unit tests

set(NANOARQ_UNIT_TEST_SOURCES main.cpp
                              nanoarq_mocks.h
                              nanoarq_mocks.cpp
                              nanoarq_hook_plugin.h
                              nanoarq_hook_plugin.cpp
                              test_mock_hooks.cpp
                              test_nanoarq.cpp
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
                              test_crc32.cpp)

include_directories(${CPPUTEST_INCLUDE_DIR})

################ nanoarq-c99 unit tests

add_executable(nanoarq_unit_tests ${NANOARQ_UNIT_TEST_SOURCES})
target_compile_options(nanoarq_unit_tests PRIVATE ${NANOARQ_COMMON_FLAGS} -DARQ_ASSERTS_ENABLED=1)
target_link_libraries(nanoarq_unit_tests nanoarq_c99 libCppUTest libCppUTestExt)
nanoarq_add_test(nanoarq_c99 nanoarq_unit_tests)

################ nanoarq-c99 unit tests, asserts disabled

add_executable(nanoarq_no_asserts_unit_tests ${NANOARQ_UNIT_TEST_SOURCES})
target_compile_options(nanoarq_no_asserts_unit_tests PRIVATE ${NANOARQ_COMMON_FLAGS} -DARQ_ASSERTS_ENABLED=0)
target_link_libraries(nanoarq_no_asserts_unit_tests nanoarq_c99_no_asserts libCppUTest libCppUTestExt)
nanoarq_add_test(nanoarq_c99_no_asserts nanoarq_no_asserts_unit_tests)

