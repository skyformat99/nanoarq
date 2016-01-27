#include "nanoarq_in_test_project.h"
#include "CppUTest/TestHarness.h"

TEST_GROUP(frame) {};

namespace
{

TEST(frame, size_is_header_size_plus_segment_size_plus_cobs_overhead)
{
    int seg_size = 123;
    CHECK_EQUAL(NANOARQ_FRAME_COBS_OVERHEAD + NANOARQ_FRAME_HEADER_SIZE + seg_size,
                arq__frame_size(seg_size));
}

TEST(frame, write_writes_frame_header_at_offset_1)
{
    arq__frame_hdr_t orig;
    orig.version = 12;
    orig.seg_len = 0;
    orig.win_size = 9;
    orig.seq_num = 123;
    orig.msg_len = 5;
    orig.seg_id = 3;
    orig.ack_num = 23;
    orig.ack_seg_mask = 0x0AC3;
    orig.rst = 1;
    orig.fin = 1;
    char frame[64];
    CHECK((int)sizeof(frame) >= arq__frame_size(orig.seg_len));
    arq__frame_write(&orig, nullptr, frame, sizeof(frame));
    arq__frame_hdr_t actual;
    arq__frame_hdr_read(&frame[1], &actual);
    MEMCMP_EQUAL(&orig, &actual, sizeof(actual));
}

TEST(frame, write_copies_segment_into_frame_after_header)
{
    arq__frame_hdr_t h;
    h.seg_len = 16;
    char const seg[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    char frame[64];
    CHECK((int)sizeof(frame) >= arq__frame_size(h.seg_len));
    arq__frame_write(&h, seg, frame, sizeof(frame));
    MEMCMP_EQUAL(seg, &frame[1 + NANOARQ_FRAME_HEADER_SIZE], sizeof(seg));
}

}

