cmake_minimum_required(VERSION 3.2)
project(nanoarq C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

################ CppUTest

include(ExternalProject)

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

################ Test support

function(nanoarq_add_test TARGET_NAME TEST_EXECUTABLE)
    set(TEST_TIMESTAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_tests.timestamp)
    add_custom_target(RUN_${TARGET_NAME}_TESTS ALL DEPENDS ${TEST_TIMESTAMP_FILE})
    add_custom_command(OUTPUT ${TEST_TIMESTAMP_FILE}
                       COMMAND ${TEST_EXECUTABLE} && touch ${TEST_TIMESTAMP_FILE}
                       DEPENDS ${TEST_EXECUTABLE}
                       COMMENT "Running ${TEST_EXECUTABLE}")
endfunction()

################ Nanoarq

set(NANOARQ_FLAGS -Os -pedantic -ansi -fstrict-aliasing -Wsign-conversion)
set(NANOARQ_COMMON_FLAGS -Wall -Wextra -Wshadow -Wcast-align -Wcast-qual -Wstrict-aliasing=2 -march=native -g)
include_directories(${CMAKE_SOURCE_DIR}) # arq.h

################ Targets

add_subdirectory(unit_tests)
add_subdirectory(functional_tests)

