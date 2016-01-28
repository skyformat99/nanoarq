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

################ plthook

set(PLTHOOK_ROOT_DIR ${CMAKE_SOURCE_DIR}/external/plthook)
set(PLTHOOK_SRC_DIR ${PLTHOOK_ROOT_DIR}/bin)
set(PLTHOOK_INCLUDE_DIR ${PLTHOOK_SRC_DIR})

ExternalProject_Add(plthook_external
                    PREFIX ${PLTHOOK_ROOT_DIR}
                    GIT_REPOSITORY "https://github.com/kubo/plthook.git"
                    GIT_TAG "c731d97f1b81b9c4fd26279fc09112d3f8c7b82a"
                    UPDATE_COMMAND ""
                    PATCH_COMMAND ""
                    CONFIGURE_COMMAND ""
                    BUILD_COMMAND ""
                    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory ${PLTHOOK_ROOT_DIR}/src/plthook_external ${PLTHOOK_SRC_DIR}
                    BUILD_BYPRODUCTS ${PLTHOOK_SRC_DIR}/plthook_osx.c
                                     ${PLTHOOK_SRC_DIR}/plthook_elf.c
                                     ${PLTHOOK_SRC_DIR}/plthook_win32.c
                                     ${PLTHOOK_SRC_DIR}/plthook.h)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(PLTHOOK_SOURCES ${PLTHOOK_SRC_DIR}/plthook_osx.c)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(PLTHOOK_SOURCES ${PLTHOOK_SRC_DIR}/plthook_elf.c)
else()
    error("Unsupported platform ${CMAKE_SYSTEM_NAME}")
endif()

add_library(plthook STATIC ${PLTHOOK_SOURCES})
add_dependencies(plthook plthook_external)

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

include_directories(${CMAKE_SOURCE_DIR} ${CPPUTEST_INCLUDE_DIR} ${PLTHOOK_INCLUDE_DIR})

add_library(nanoarq_c90 SHARED nanoarq.h
                               unit_tests/nanoarq_in_test_project.h
                               unit_tests/nanoarq_in_c90_test_project.c)
target_compile_options(nanoarq_c90 PRIVATE ${NANOARQ_FLAGS} -DNANOARQ_COMPILE_AS_CPP=0 -pedantic -std=c90)

add_library(nanoarq_c99 SHARED nanoarq.h
                               unit_tests/nanoarq_in_test_project.h
                               unit_tests/nanoarq_in_c99_test_project.c)
target_compile_options(nanoarq_c99 PRIVATE ${NANOARQ_FLAGS} -DNANOARQ_COMPILE_AS_CPP=0 -pedantic -std=c99)

add_library(nanoarq_cpp SHARED nanoarq.h
                               unit_tests/nanoarq_in_test_project.h
                               unit_tests/nanoarq_in_cpp_test_project.cpp)
target_compile_options(nanoarq_cpp PRIVATE ${NANOARQ_FLAGS} -DNANOARQ_COMPILE_AS_CPP=1 -pedantic)

set(NANOARQ_UNIT_TEST_FLAGS -Wno-unused-function -Wno-tautological-compare)
set(NANOARQ_UNIT_TEST_SOURCES unit_tests/main.cpp
                              unit_tests/PltHookPlugin.h
                              unit_tests/PltHookPlugin.cpp
                              unit_tests/test_hook.cpp
                              unit_tests/test_nanoarq.cpp
                              unit_tests/test_asserts.cpp
                              unit_tests/test_lin_alloc.cpp
                              unit_tests/test_frame_hdr.cpp
                              unit_tests/test_ntoh_hton_le.cpp
                              unit_tests/test_ntoh_hton_be.cpp
                              unit_tests/test_backend_recv.cpp
                              unit_tests/test_frame.cpp
                              unit_tests/test_cobs.cpp)

add_executable(nanoarq_c90_tests ${NANOARQ_UNIT_TEST_SOURCES})
target_compile_options(nanoarq_c90_tests PRIVATE -DNANOARQ_COMPILE_AS_CPP=0 ${NANOARQ_FLAGS} ${NANOARQ_UNIT_TEST_FLAGS})
target_link_libraries(nanoarq_c90_tests nanoarq_c90 libCppUTest libCppUTestExt plthook dl)
add_unit_tests(nanoarq_c90 nanoarq_c90_tests)

add_executable(nanoarq_c99_tests ${NANOARQ_UNIT_TEST_SOURCES})
add_dependencies(nanoarq_c99_tests nanoarq_c90_tests)
target_compile_options(nanoarq_c99_tests PRIVATE -DNANOARQ_COMPILE_AS_CPP=0 ${NANOARQ_FLAGS} ${NANOARQ_UNIT_TEST_FLAGS})
target_link_libraries(nanoarq_c99_tests nanoarq_c99 libCppUTest libCppUTestExt plthook dl)
add_unit_tests(nanoarq_c99 nanoarq_c99_tests)

add_executable(nanoarq_cpp_tests ${NANOARQ_UNIT_TEST_SOURCES})
add_dependencies(nanoarq_cpp_tests nanoarq_c99_tests)
target_compile_options(nanoarq_cpp_tests PRIVATE -DNANOARQ_COMPILE_AS_CPP=1 ${NANOARQ_FLAGS} ${NANOARQ_UNIT_TEST_FLAGS})
target_link_libraries(nanoarq_cpp_tests nanoarq_cpp libCppUTest libCppUTestExt plthook dl)
add_unit_tests(nanoarq_cpp nanoarq_cpp_tests)

