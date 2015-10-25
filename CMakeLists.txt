cmake_minimum_required(VERSION 3.2)
project(nanoarq)

set(CMAKE_CXX_STANDARD 11)

################ CppUTest

set(CPPUTEST_ROOT_DIR ${CMAKE_SOURCE_DIR}/external/cpputest)
set(CPPUTEST_LIB_DIR ${CPPUTEST_ROOT_DIR}/bin/lib)
set(CPPUTEST_INCLUDE_DIR ${CPPUTEST_ROOT_DIR}/bin/include)

include(ExternalProject)

ExternalProject_Add(CppUTest_external
                    PREFIX ${CPPUTEST_ROOT_DIR}
                    GIT_REPOSITORY "https://github.com/cpputest/cpputest.git"
                    GIT_TAG "f09a20eab1510357514f0f655253537f2dcbc229"
                    UPDATE_COMMAND ""
                    PATCH_COMMAND ""
                    BINARY_DIR ${CPPUTEST_ROOT_DIR}/src/CppUTest
                    SOURCE_DIR ${CPPUTEST_ROOT_DIR}/src/CppUTest
                    INSTALL_DIR ${CPPUTEST_ROOT_DIR}/bin
                    CONFIGURE_COMMAND ./configure --prefix=<INSTALL_DIR>
                    BUILD_COMMAND make
                    BUILD_BYPRODUCTS ${CPPUTEST_LIB_DIR}/libCppUTest.a)

ExternalProject_Add_Step(CppUTest_external
                         autogen
                         COMMAND ./autogen.sh
                         DEPENDEES download
                         DEPENDERS configure
                         WORKING_DIRECTORY ${CPPUTEST_ROOT_DIR}/src/CppUTest)

add_library(libCppUTest STATIC IMPORTED)
add_library(libCppUTestExt STATIC IMPORTED)
set_target_properties(libCppUTest PROPERTIES IMPORTED_LOCATION ${CPPUTEST_LIB_DIR}/libCppUTest.a)
set_target_properties(libCppUTestExt PROPERTIES IMPORTED_LOCATION ${CPPUTEST_LIB_DIR}/libCppUTestExt.a)
add_dependencies(libCppUTest CppUTest_external)
add_dependencies(libCppUTestExt CppUTest_external)

################ unit tests

