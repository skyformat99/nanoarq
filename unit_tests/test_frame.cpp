#include "nanoarq_in_test_project.h"
#include "PltHookPlugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <cstring>

TEST_GROUP(frame) {};

namespace
{

TEST(frame, size_is_header_size_plus_segment_size_plus_cobs_overhead)
{
    int seg_size = 123;
    CHECK_EQUAL(NANOARQ_FRAME_COBS_OVERHEAD + NANOARQ_FRAME_HEADER_SIZE + seg_size,
                arq__frame_size(seg_size));
}

struct Fixture
{
    Fixture()
    {
        h.version = 12;
        h.seg_len = 0;
        h.win_size = 9;
        h.seq_num = 123;
        h.msg_len = 5;
        h.seg_id = 3;
        h.ack_num = 23;
        h.ack_seg_mask = 0x0AC3;
        h.rst = 1;
        h.fin = 1;
        CHECK((int)sizeof(frame) >= arq__frame_size(h.seg_len));
    }
    arq__frame_hdr_t h;
    char const seg[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    char frame[64];
};

TEST(frame, write_writes_frame_header_at_offset_1)
{
    Fixture f;
    arq__frame_write(&f.h, nullptr, f.frame, sizeof(f.frame));
    arq__frame_hdr_t actual;
    arq__frame_hdr_read(&f.frame[1], &actual);
    MEMCMP_EQUAL(&f.h, &actual, sizeof(actual));
}

TEST(frame, write_copies_segment_into_frame_after_header)
{
    Fixture f;
    f.h.seg_len = sizeof(f.seg);
    arq__frame_write(&f.h, f.seg, f.frame, sizeof(f.frame));
    MEMCMP_EQUAL(f.seg, &f.frame[1 + NANOARQ_FRAME_HEADER_SIZE], sizeof(f.seg));
}

TEST(frame, read_reads_frame_header_at_offset_1)
{
    Fixture f;
    arq__frame_hdr_write(&f.h, &f.frame[1]);
    arq__frame_hdr_t actual;
    void const *seg;
    arq__frame_read(f.frame, &actual, &seg);
    MEMCMP_EQUAL(&f.h, &actual, sizeof(&f.h));
}

TEST(frame, read_points_out_seg_to_segment_in_frame)
{
    Fixture f;
    f.h.seg_len = sizeof(f.seg);
    arq__frame_hdr_write(&f.h, &f.frame[1]);
    std::memcpy(&f.frame[1 + NANOARQ_FRAME_HEADER_SIZE], f.seg, sizeof(f.seg));
    void const *rseg;
    arq__frame_hdr_t rh;
    arq__frame_read(f.frame, &rh, &rseg);
    CHECK_EQUAL(rseg, (void const *)&f.frame[1 + NANOARQ_FRAME_HEADER_SIZE]);
}

}

