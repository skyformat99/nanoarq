#include "nanoarq_in_test_project.h"
#include "CppUTest/TestHarness.h"

TEST_GROUP(frame) {};

namespace
{

TEST(frame, required_size_is_header_size_plus_segment_size_plus_cobs_overhead)
{
    int seg_size = 123;
    int cobs_overhead = 2;
    CHECK_EQUAL(cobs_overhead + NANOARQ_FRAME_HEADER_SIZE + seg_size, arq__frame_required_size(seg_size));
}

TEST(frame, init_captures_buf)
{
}

TEST(frame, accumulates_byte_into_decode_buf)
{
}

}

