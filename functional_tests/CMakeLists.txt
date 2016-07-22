cmake_minimum_required(VERSION 3.4)
project(functional_tests C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED on)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

include_directories(${CPPUTEST_INCLUDE_DIR})

set(ARQ_FUNCTIONAL_TEST_SOURCES ${CMAKE_SOURCE_DIR}/arq.h
								main.cpp
                                functional_tests.h
                                arq_in_functional_tests.h
                                arq_in_functional_tests.c
                                arq_context.h
                                arq_context.cpp
                                frame_serialization.cpp
                                send_full_window.cpp
                                send_10mb_through_window.cpp
                                recv_full_window_one_segment_at_a_time.cpp
                                recv_full_window_in_one_call.cpp
                                recv_full_window_one_byte_at_a_time.cpp
                                recv_10mb_through_window.cpp
                                recv_full_frame_partial_frame_full_frame.cpp
                                transfer_full_window_one_way.cpp
                                transfer_10mb_one_way_manual_acks.cpp
                                ack_one_message.cpp
                                ack_full_window.cpp
                                poll_until_tinygram_sends.cpp
                                flush_tinygram.cpp
                                tiny_sends_accumulate_into_message.cpp
                                retransmission_timers.cpp
                                lost_ack_payload_retransmitted_already_received.cpp
                                losing_final_segment_in_message_triggers_nak.cpp
                                losing_non_final_segment_triggers_nak.cpp
                                connect_times_out_after_n_attempts.cpp
                                connect_three_way_handshake.cpp
                                connect_simultaneous.cpp)

string(REPLACE ";" " " ARQ_RUNTIME_FLAGS_STR "${ARQ_RUNTIME_FLAGS}")
set_source_files_properties(arq_in_test_project.c PROPERTIES COMPILE_FLAGS "${ARQ_RUNTIME_FLAGS_STR}")

add_executable(arq_functional_tests ${ARQ_FUNCTIONAL_TEST_SOURCES})
target_compile_options(arq_functional_tests PRIVATE ${ARQ_COMMON_FLAGS})
add_dependencies(arq_functional_tests CppUTest_external all_unit_tests)
target_link_libraries(arq_functional_tests libCppUTest libCppUTestExt)
if (MSVC)
	target_link_libraries(arq_functional_tests winmm)
endif()
arq_add_test(arq_functional_tests)
