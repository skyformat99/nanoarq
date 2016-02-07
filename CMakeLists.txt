cmake_minimum_required(VERSION 3.2)
project(nanoarq C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

include(ExternalProject)

################ CppUTest

set(CPPUTEST_ROOT_DIR ${CMAKE_SOURCE_DIR}/external/CppUTest)
set(CPPUTEST_LIB_DIR ${CPPUTEST_ROOT_DIR}/bin/lib)
set(CPPUTEST_INCLUDE_DIR ${CPPUTEST_ROOT_DIR}/bin/include)

ExternalProject_Add(CppUTest_external
                    PREFIX ${CPPUTEST_ROOT_DIR}
                    GIT_REPOSITORY "https://github.com/cpputest/cpputest.git"
                    GIT_TAG "efd7a2b75f79d4ee5bd92d1e48a78f8a5b869cd1"
                    UPDATE_COMMAND ""
                    PATCH_COMMAND ""
                    BINARY_DIR ${CPPUTEST_ROOT_DIR}/src/CppUTest
                    SOURCE_DIR ${CPPUTEST_ROOT_DIR}/src/CppUTest
                    INSTALL_DIR ${CPPUTEST_ROOT_DIR}/bin
                    CONFIGURE_COMMAND ./configure --prefix=<INSTALL_DIR>
                    BUILD_COMMAND make
                    BUILD_BYPRODUCTS ${CPPUTEST_LIB_DIR}/libCppUTest.a
                                     ${CPPUTEST_LIB_DIR}/libCppUTestExt.a)

ExternalProject_Add_Step(CppUTest_external
                         autogen
                         COMMAND ./autogen.sh
                         DEPENDEES download
                         DEPENDERS configure
                         WORKING_DIRECTORY ${CPPUTEST_ROOT_DIR}/src/CppUTest)

add_library(libCppUTest STATIC IMPORTED)
set_target_properties(libCppUTest PROPERTIES IMPORTED_LOCATION ${CPPUTEST_LIB_DIR}/libCppUTest.a)

add_library(libCppUTestExt STATIC IMPORTED)
set_target_properties(libCppUTestExt PROPERTIES IMPORTED_LOCATION ${CPPUTEST_LIB_DIR}/libCppUTestExt.a)

add_dependencies(libCppUTest CppUTest_external)
add_dependencies(libCppUTestExt CppUTest_external)

################ tests

function(add_unit_tests TARGET_NAME UNIT_TEST_EXECUTABLE)
    set(UNIT_TEST_TIMESTAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_unit_tests.timestamp)
    add_custom_target(RUN_${TARGET_NAME}_UNIT_TESTS ALL DEPENDS ${UNIT_TEST_TIMESTAMP_FILE})
    add_custom_command(OUTPUT ${UNIT_TEST_TIMESTAMP_FILE}
                       COMMAND ${UNIT_TEST_EXECUTABLE} && touch ${UNIT_TEST_TIMESTAMP_FILE}
                       DEPENDS ${UNIT_TEST_EXECUTABLE}
                       COMMENT "Running ${UNIT_TEST_EXECUTABLE}")
endfunction()

set(NANOARQ_FLAGS -Wall
                  -Wextra
                  -Wshadow
                  -Wsign-conversion
                  -Wcast-align
                  -Wcast-qual
                  -Wstrict-aliasing=2
                  --save-temps
                  -g
                  -Os)

include_directories(${CMAKE_SOURCE_DIR} ${CPPUTEST_INCLUDE_DIR})

add_library(nanoarq_c90 STATIC nanoarq.h
                               unit_tests/nanoarq_in_test_project.h
                               unit_tests/nanoarq_in_c90_test_project.c)
target_compile_options(nanoarq_c90 PRIVATE ${NANOARQ_FLAGS} -pedantic -std=c90)

add_library(nanoarq_c99 STATIC nanoarq.h
                               unit_tests/nanoarq_in_test_project.h
                               unit_tests/nanoarq_in_c99_test_project.c)
target_compile_options(nanoarq_c99 PRIVATE ${NANOARQ_FLAGS} -pedantic -std=c99)

add_library(nanoarq_cpp STATIC nanoarq.h
                               unit_tests/nanoarq_in_test_project.h
                               unit_tests/nanoarq_in_cpp_test_project.cpp)
target_compile_options(nanoarq_cpp PRIVATE ${NANOARQ_FLAGS} -pedantic)

set(NANOARQ_UNIT_TEST_FLAGS -Wno-unused-function -Wno-tautological-compare)
set(NANOARQ_UNIT_TEST_SOURCES unit_tests/main.cpp
                              unit_tests/nanoarq_mocks.h
                              unit_tests/nanoarq_mocks.cpp
                              unit_tests/nanoarq_hook_plugin.h
                              unit_tests/nanoarq_hook_plugin.cpp
                              unit_tests/test_mock_hooks.cpp
                              unit_tests/test_nanoarq.cpp
                              unit_tests/test_asserts.cpp
                              unit_tests/test_lin_alloc.cpp
                              unit_tests/test_frame_hdr.cpp
                              unit_tests/test_ntoh_hton_le.cpp
                              unit_tests/test_backend_recv.cpp
                              unit_tests/test_frame.cpp
                              unit_tests/test_cobs.cpp
                              unit_tests/test_crc32.cpp)

add_executable(nanoarq_unit_tests ${NANOARQ_UNIT_TEST_SOURCES})
target_compile_options(nanoarq_unit_tests PRIVATE ${NANOARQ_FLAGS} ${NANOARQ_UNIT_TEST_FLAGS})
target_link_libraries(nanoarq_unit_tests nanoarq_c99 libCppUTest libCppUTestExt dl)
add_unit_tests(nanoarq_c99 nanoarq_unit_tests)
