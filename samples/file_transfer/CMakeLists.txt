cmake_minimum_required(VERSION 3.4)
project(file_transfer CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)

############# Arq runtime

set(ARQ_SOURCES ${CMAKE_SOURCE_DIR}/arq.h file_transfer_arq.h file_transfer_arq.cpp)
string(REPLACE ";" " " ARQ_RUNTIME_FLAGS_STR "${ARQ_RUNTIME_FLAGS}")
set_source_files_properties(file_transfer_arq.cpp PROPERTIES COMPILE_FLAGS "${ARQ_RUNTIME_FLAGS_STR}")

############# file_transfer

set(FILE_TRANSFER_SOURCES main.cpp)
add_executable(file_transfer ${FILE_TRANSFER_SOURCES} ${ARQ_SOURCES})
set_target_properties(file_transfer PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

