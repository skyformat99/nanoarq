cmake_minimum_required(VERSION 3.2)
project(nanoarq)

set(CMAKE_CXX_STANDARD 11)

################ CppUTest

set(CPPUTEST_ROOT_DIR ${CMAKE_SOURCE_DIR}/external/cpputest)
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
                    INSTALL_COMMAND ""
                    UPDATE_COMMAND ""
                    PATCH_COMMAND ""
                    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
                    CMAKE_ARGS "${CMAKE_ARGS}")

################ unit tests

