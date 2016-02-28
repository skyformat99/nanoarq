cmake_minimum_required(VERSION 3.2)
project(nanoarq C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

include(ExternalProject)

################ CppUTest

set(CPPUTEST_ROOT_DIR ${CMAKE_SOURCE_DIR}/external/CppUTest)
set(CPPUTEST_LIB_DIR ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/lib)
set(CPPUTEST_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/include)
set(CPPUTEST_CMAKE_ARGS -DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
                        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${PROJECT_NAME}
                        -DTESTS=OFF
                        -DC++11=ON)

ExternalProject_Add(CppUTest_external
                    PREFIX ${CPPUTEST_ROOT_DIR}
                    GIT_REPOSITORY "https://github.com/cpputest/cpputest.git"
                    GIT_TAG "efd7a2b75f79d4ee5bd92d1e48a78f8a5b869cd1"
                    UPDATE_COMMAND ""
                    PATCH_COMMAND ""
                    TEST_COMMAND ""
                    CMAKE_ARGS ${CPPUTEST_CMAKE_ARGS}
                    BINARY_DIR ${CMAKE_BINARY_DIR}/${PROJECT_NAME}
                    BUILD_BYPRODUCTS ${CPPUTEST_LIB_DIR}/libCppUTest.a ${CPPUTEST_LIB_DIR}/libCppUTestExt.a)

add_library(libCppUTest STATIC IMPORTED)
set_target_properties(libCppUTest PROPERTIES IMPORTED_LOCATION ${CPPUTEST_LIB_DIR}/libCppUTest.a)

add_library(libCppUTestExt STATIC IMPORTED)
set_target_properties(libCppUTestExt PROPERTIES IMPORTED_LOCATION ${CPPUTEST_LIB_DIR}/libCppUTestExt.a)

add_dependencies(libCppUTest CppUTest_external)
add_dependencies(libCppUTestExt CppUTest_external)

################ nanoarq

set(NANOARQ_FLAGS -Os -pedantic -ansi -fstrict-aliasing -Wsign-conversion)
set(NANOARQ_COMMON_FLAGS -Wall -Wextra -Wshadow -Wcast-align -Wcast-qual -Wstrict-aliasing=2 -march=native -g)
set(NANOARQ_COMMON_SOURCES arq.h unit_tests/nanoarq_in_test_project.h)

include_directories(${CMAKE_SOURCE_DIR})

add_library(nanoarq_c90 STATIC ${NANOARQ_COMMON_SOURCES} unit_tests/nanoarq_in_c90_test_project.c)
target_compile_options(nanoarq_c90 PRIVATE
                       ${NANOARQ_FLAGS} ${NANOARQ_COMMON_FLAGS} -std=c90 -DARQ_ASSERTS_ENABLED=1)

add_library(nanoarq_c99 STATIC ${NANOARQ_COMMON_SOURCES} unit_tests/nanoarq_in_c99_test_project.c)
target_compile_options(nanoarq_c99 PRIVATE
                       ${NANOARQ_FLAGS} ${NANOARQ_COMMON_FLAGS} -std=c99 -DARQ_ASSERTS_ENABLED=1)

add_library(nanoarq_c99_no_asserts STATIC ${NANOARQ_COMMON_SOURCES} unit_tests/nanoarq_in_c99_test_project.c)
target_compile_options(nanoarq_c99_no_asserts PRIVATE
                       ${NANOARQ_FLAGS} ${NANOARQ_COMMON_FLAGS} -std=c99 -DARQ_ASSERTS_ENABLED=0)

add_library(nanoarq_cpp STATIC ${NANOARQ_COMMON_SOURCES} unit_tests/nanoarq_in_cpp_test_project.cpp)
target_compile_options(nanoarq_cpp PRIVATE ${NANOARQ_FLAGS} ${NANOARQ_COMMON_FLAGS} -DARQ_ASSERTS_ENABLED=1)

################ tests

function(add_unit_tests TARGET_NAME UNIT_TEST_EXECUTABLE)
    set(UNIT_TEST_TIMESTAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_unit_tests.timestamp)
    add_custom_target(RUN_${TARGET_NAME}_UNIT_TESTS ALL DEPENDS ${UNIT_TEST_TIMESTAMP_FILE})
    add_custom_command(OUTPUT ${UNIT_TEST_TIMESTAMP_FILE}
                       COMMAND ${UNIT_TEST_EXECUTABLE} && touch ${UNIT_TEST_TIMESTAMP_FILE}
                       DEPENDS ${UNIT_TEST_EXECUTABLE}
                       COMMENT "Running ${UNIT_TEST_EXECUTABLE}")
endfunction()

set(NANOARQ_UNIT_TEST_FLAGS -Wno-tautological-compare)
set(NANOARQ_UNIT_TEST_SOURCES unit_tests/main.cpp
                              unit_tests/nanoarq_mocks.h
                              unit_tests/nanoarq_mocks.cpp
                              unit_tests/nanoarq_hook_plugin.h
                              unit_tests/nanoarq_hook_plugin.cpp
                              unit_tests/test_mock_hooks.cpp
                              unit_tests/test_nanoarq.cpp
                              unit_tests/test_send.cpp
                              unit_tests/test_flush.cpp
                              unit_tests/test_asserts.cpp
                              unit_tests/test_lin_alloc.cpp
                              unit_tests/test_frame_hdr.cpp
                              unit_tests/test_ntoh_hton_le.cpp
                              unit_tests/test_backend_send.cpp
                              unit_tests/test_backend_recv.cpp
                              unit_tests/test_backend_poll.cpp
                              unit_tests/test_send_window.cpp
                              unit_tests/test_send_window_ptr.cpp
                              unit_tests/test_send_frame.cpp
                              unit_tests/test_send_poll.cpp
                              unit_tests/test_frame.cpp
                              unit_tests/test_cobs.cpp
                              unit_tests/test_min.cpp
                              unit_tests/test_saturated_subtraction.cpp
                              unit_tests/test_crc32.cpp)

include_directories(${CPPUTEST_INCLUDE_DIR})

add_executable(nanoarq_unit_tests ${NANOARQ_UNIT_TEST_SOURCES})
target_compile_options(nanoarq_unit_tests PRIVATE
                       ${NANOARQ_COMMON_FLAGS} ${NANOARQ_UNIT_TEST_FLAGS} -DARQ_ASSERTS_ENABLED=1)
target_link_libraries(nanoarq_unit_tests nanoarq_c99 libCppUTest libCppUTestExt)
add_unit_tests(nanoarq_c99 nanoarq_unit_tests)

add_executable(nanoarq_no_asserts_unit_tests ${NANOARQ_UNIT_TEST_SOURCES})
add_dependencies(nanoarq_no_asserts_unit_tests nanoarq_unit_tests)
target_compile_options(nanoarq_no_asserts_unit_tests PRIVATE
                       ${NANOARQ_COMMON_FLAGS} ${NANOARQ_UNIT_TEST_FLAGS} -DARQ_ASSERTS_ENABLED=0)
target_link_libraries(nanoarq_no_asserts_unit_tests nanoarq_c99_no_asserts libCppUTest libCppUTestExt)
add_unit_tests(nanoarq_c99_no_asserts nanoarq_no_asserts_unit_tests)

