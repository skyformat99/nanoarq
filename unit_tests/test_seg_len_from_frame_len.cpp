#include "arq_in_unit_tests.h"
#include <CppUTest/TestHarness.h>

TEST_GROUP(seg_len_from_frame_len) {};

namespace {

TEST(seg_len_from_frame_len, null_pointer)
{
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_seg_len_from_frame_len(100, nullptr));
}

TEST(seg_len_from_frame_len, tiny_frame)
{
    unsigned x;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_seg_len_from_frame_len(1, &x));
}

TEST(seg_len_from_frame_len, success_with_valid_params)
{
    unsigned x;
    CHECK_EQUAL(ARQ_OK_COMPLETED, arq_seg_len_from_frame_len(100, &x));
}

TEST(seg_len_from_frame_len, seg_len_is_frame_len_minus_overhead)
{
    unsigned x;
    arq_seg_len_from_frame_len(100, &x);
    CHECK_EQUAL(100 - ARQ__FRAME_COBS_OVERHEAD - ARQ__FRAME_HEADER_SIZE - 4, x);
}

}

