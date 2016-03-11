#include "nanoarq_unit_test.h"
#include "nanoarq_hook_plugin.h"
#include <CppUTestExt/MockSupport.h>
#include <CppUTest/TestHarness.h>
#include <array>
#include <vector>

TEST_GROUP(recv_wnd) {};

namespace {

struct Fixture
{
    Fixture()
    {
        w.msg = msg.data();
        arq__wnd_init(&w, msg.size(), 32, 64);
        buf.resize(w.msg_len * w.cap);
        w.buf = buf.data();
    }

    arq__wnd_t w;
    std::array< arq__msg_t, 64 > msg;
    std::vector< unsigned char > buf;
    std::vector< unsigned char > seg;
};

TEST(recv_wnd, frame_writes_len_to_msg)
{
    Fixture f;
    f.seg.resize(34);
    arq__recv_wnd_frame(&f.w, 0, 0, 1, f.seg.data(), f.seg.size());
    CHECK_EQUAL(f.seg.size(), f.msg[0].len);
}

TEST(recv_wnd, frame_accumulates_seg_len_to_partially_full_msg)
{
    Fixture f;
    f.seg.resize(29);
    f.msg[0].len = 17;
    arq__recv_wnd_frame(&f.w, 0, 1, 2, f.seg.data(), f.seg.size());
    CHECK_EQUAL(17 + 29, f.msg[0].len);
}

TEST(recv_wnd, frame_indexes_msg_by_seq)
{
    Fixture f;
    f.seg.resize(1);
    f.w.seq = 12344;
    arq__recv_wnd_frame(&f.w, 12345, 0, 1, f.seg.data(), f.seg.size());
    CHECK_EQUAL(1, f.msg[12345 % f.w.cap].len);
}

void MockWndSeg(arq__wnd_t *w, int seq, int seg, void const **out_seg, int *out_seg_len)
{
    mock().actualCall("arq__wnd_seg")
          .withParameter("w", w)
          .withParameter("seq", seq)
          .withParameter("seg", seg)
          .withOutputParameter("out_seg", (void *)out_seg)
          .withOutputParameter("out_seg_len", out_seg_len);
}

TEST(recv_wnd, frame_copies_data_into_seat_provided_by_seg_call)
{
    Fixture f;
    f.w.seq = 120;
    int const seq = 123;
    int const seg = 456;
    for (auto i = 0; i < 7; ++i) {
        f.seg.emplace_back((unsigned char)i);
    }
    void *seat = &f.buf[9];
    ARQ_MOCK_HOOK(arq__wnd_seg, MockWndSeg);
    mock().expectOneCall("arq__wnd_seg")
          .withParameter("w", &f.w)
          .withParameter("seq", seq)
          .withParameter("seg", seg)
          .withOutputParameterReturning("out_seg", &seat, sizeof(void *))
          .ignoreOtherParameters();
    arq__recv_wnd_frame(&f.w, seq, seg, 1, f.seg.data(), f.seg.size());
    MEMCMP_EQUAL(f.seg.data(), seat, f.seg.size());
}

TEST(recv_wnd, frame_writes_full_ack_vec_to_msg)
{
    Fixture f;
    f.seg.resize(1);
    f.msg[0].full_ack_vec = 0;
    arq__recv_wnd_frame(&f.w, 0, 0, 7, f.seg.data(), f.seg.size());
    CHECK_EQUAL(0b1111111, f.msg[0].full_ack_vec);
}

TEST(recv_wnd, frame_ors_in_current_seg_to_cur_ack_vector)
{
    Fixture f;
    f.seg.resize(1);
    f.msg[0].full_ack_vec = 0b1111;
    f.msg[0].cur_ack_vec = 0b0101;
    arq__recv_wnd_frame(&f.w, 0, 3, 4, f.seg.data(), f.seg.size());
    CHECK_EQUAL(0b1101, f.msg[0].cur_ack_vec);
}

TEST(recv_wnd, frame_does_not_update_window_size_if_segment_in_existing_message)
{
    Fixture f;
    f.w.size = 1;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.w, f.w.seq, 1, 2, f.seg.data(), f.seg.size());
    CHECK_EQUAL(1, f.w.size);
}

TEST(recv_wnd, frame_increments_window_size_for_first_segment_in_next_message)
{
    Fixture f;
    f.w.size = 1;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.w, f.w.seq + 1, 0, 1, f.seg.data(), f.seg.size());
    CHECK_EQUAL(2, f.w.size);
}

TEST(recv_wnd, frame_increments_window_size_for_non_first_segment_in_next_message)
{
    Fixture f;
    f.w.size = 1;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.w, f.w.seq + 1, 1, 2, f.seg.data(), f.seg.size());
    CHECK_EQUAL(2, f.w.size);
}

TEST(recv_wnd, frame_updates_window_size_for_first_segment_in_non_adjacent_message)
{
    Fixture f;
    f.w.size = 0;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.w, f.w.seq + 4, 0, 1, f.seg.data(), f.seg.size());
    CHECK_EQUAL(5, f.w.size);
}

TEST(recv_wnd, frame_updates_window_size_for_non_first_segment_in_non_adjacent_message)
{
    Fixture f;
    f.w.size = 0;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.w, f.w.seq + 4, 2, 4, f.seg.data(), f.seg.size());
    CHECK_EQUAL(5, f.w.size);
}

TEST(recv_wnd, frame_updates_window_size_correctly_when_new_segment_wraps_sequence_number_space)
{
    Fixture f;
    f.w.size = 1;
    f.w.seq = ARQ__FRAME_MAX_SEQ_NUM;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.w, 0, 1, 1, f.seg.data(), f.seg.size());
    CHECK_EQUAL(2, f.w.size);
}

TEST(recv_wnd, frame_ignores_sequence_numbers_outside_of_window_capacity)
{
    Fixture f;
    f.w.cap = 10;
    f.w.seq = 100;
    f.w.size = 2;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.w, 2000, 0, 1, f.seg.data(), f.seg.size());
    CHECK_EQUAL(2, f.w.size);
}

}

