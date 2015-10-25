cmake_minimum_required(VERSION 3.2)
project(nanoarq)

set(CMAKE_CXX_STANDARD 11)

################ CppUTest

set(CPPUTEST_ROOT_DIR ${CMAKE_BINARY_DIR}/.deps/cpputest)
set(CPPUTEST_LIB_DIR ${CPPUTEST_ROOT_DIR}/lib)
set(CPPUTEST_INCLUDE_DIR ${CPPUTEST_ROOT_DIR}/include)

include(ExternalProject)

set(CPPUTEST_CMAKE_ARGS "-DCPPUTEST_FLAGS=OFF"
                        "-DC++11=ON"
                        "-DTESTS=OFF"
                        "-DCMAKE_BUILD_TYPE=Debug"
                        "-DCMAKE_INSTALL_PREFIX=${CPPUTEST_ROOT_DIR}")

ExternalProject_Add(CppUTest
                    PREFIX ${CPPUTEST_ROOT_DIR}
                    GIT_REPOSITORY "https://github.com/cpputest/cpputest.git"
                    GIT_TAG "${GIT_TAG}"
                    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
                    CMAKE_ARGS "${CMAKE_ARGS}")

                #add_library(libhidapi STATIC IMPORTED)
                #set_target_properties(libhidapi PROPERTIES IMPORTED_LOCATION ${HIDAPI_LIB_DIR}/libhidapi.a)
                #add_dependencies(libhidapi hidapi_external)

################ test app

#include_directories(${HIDAPI_INCLUDE_DIR})
#add_executable(hidapi-test src/main.cpp)
#target_link_libraries(hidapi-test ${IOKIT} ${COREFOUNDATION} libhidapi)

