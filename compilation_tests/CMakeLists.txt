cmake_minimum_required(VERSION 3.4)
project(compilation_tests C CXX)

function(add_nanoarq_lib TARGET_NAME TARGET_FLAGS TARGET_SOURCE)
    string(REPLACE " " ";" REPLACED_FLAGS ${TARGET_FLAGS})

    add_library(${TARGET_NAME} STATIC ${CMAKE_SOURCE_DIR}/arq.h ${TARGET_SOURCE})
    target_compile_options(${TARGET_NAME} PRIVATE
                           ${NANOARQ_FLAGS}
                           ${NANOARQ_COMMON_FLAGS}
                           ${REPLACED_FLAGS}
                           -DARQ_ASSERTS_ENABLED=1)

    add_library(${TARGET_NAME}_no_asserts STATIC ${CMAKE_SOURCE_DIR}/arq.h ${TARGET_SOURCE})
    target_compile_options(${TARGET_NAME}_no_asserts PRIVATE
                           ${NANOARQ_FLAGS}
                           ${NANOARQ_COMMON_FLAGS}
                           ${REPLACED_FLAGS}
                           -DARQ_ASSERTS_ENABLED=0)
endfunction()

add_nanoarq_lib(nanoarq_c90 -std=c90 nanoarq_compilation_test.c)
add_nanoarq_lib(nanoarq_c99 -std=c99 nanoarq_compilation_test.c)
add_nanoarq_lib(nanoarq_c11 -std=c11 nanoarq_compilation_test.c)
add_nanoarq_lib(nanoarq_cpp03 -std=c++03 nanoarq_compilation_test.cpp)
add_nanoarq_lib(nanoarq_cpp11 -std=c++11 nanoarq_compilation_test.cpp)
