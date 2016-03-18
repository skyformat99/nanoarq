cmake_minimum_required(VERSION 3.4)
project(nanoarq C CXX)

############### Global flags

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_RELEASE "-Os")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Os -g")

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g")

set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")

################ CppUTest

include(ExternalProject)

set(CPPUTEST_ROOT_DIR ${CMAKE_SOURCE_DIR}/external/CppUTest)
set(CPPUTEST_LIB_DIR ${CMAKE_BINARY_DIR}/CppUTest/lib)
set(CPPUTEST_INCLUDE_DIR ${CMAKE_BINARY_DIR}/CppUTest/include)
set(CPPUTEST_CMAKE_ARGS -DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
                        -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/CppUTest
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
                    BINARY_DIR ${CMAKE_BINARY_DIR}/CppUTest
                    INSTALL_DIR ${CMAKE_BINARY_DIR}/CppUTest
                    BUILD_BYPRODUCTS ${CPPUTEST_LIB_DIR}/libCppUTest.a ${CPPUTEST_LIB_DIR}/libCppUTestExt.a)

add_library(libCppUTest STATIC IMPORTED)
set_target_properties(libCppUTest PROPERTIES IMPORTED_LOCATION ${CPPUTEST_LIB_DIR}/libCppUTest.a)

add_library(libCppUTestExt STATIC IMPORTED)
set_target_properties(libCppUTestExt PROPERTIES IMPORTED_LOCATION ${CPPUTEST_LIB_DIR}/libCppUTestExt.a)

add_dependencies(libCppUTest CppUTest_external)
add_dependencies(libCppUTestExt CppUTest_external)

################ Test support

function(nanoarq_add_test TEST_TARGET_EXECUTABLE)
    set(TEST_TIMESTAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/${TEST_TARGET_EXECUTABLE}_tests.timestamp)
    add_custom_target(RUN_${TEST_TARGET_EXECUTABLE}_TESTS ALL DEPENDS ${TEST_TIMESTAMP_FILE})
    add_custom_command(OUTPUT ${TEST_TIMESTAMP_FILE}
                       COMMAND ${TEST_TARGET_EXECUTABLE} && touch ${TEST_TIMESTAMP_FILE}
                       DEPENDS ${TEST_TARGET_EXECUTABLE}
                       COMMENT "Running ${TEST_TARGET_EXECUTABLE}")
endfunction()

################ Nanoarq

set(NANOARQ_FLAGS -pedantic -ansi -fstrict-aliasing -Wsign-conversion)
set(NANOARQ_COMMON_FLAGS -Wall -Wextra -Wunreachable-code -Wshadow -Wcast-align -Wcast-qual
                         -Wstrict-aliasing -Wmissing-braces -march=native)
include_directories(${CMAKE_SOURCE_DIR})

################ Targets

add_subdirectory(compilation_tests)
add_subdirectory(unit_tests)
add_subdirectory(functional_tests)

