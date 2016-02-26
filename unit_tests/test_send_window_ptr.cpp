#include "nanoarq_in_test_project.h"
#include <CppUTest/TestHarness.h>
#include <array>
#include <cstring>

TEST_GROUP(send_wnd_ptr) {};

namespace
{

TEST(send_wnd_ptr, init_sets_valid_to_zero)
{
    arq__send_wnd_ptr_t p;
    p.valid = 1;
    arq__send_wnd_ptr_init(&p);
    CHECK_EQUAL(0, p.valid);
}

TEST(send_wnd_ptr, init_sets_msg_and_seg_to_zero)
{
    arq__send_wnd_ptr_t p;
    p.valid = p.msg = p.seg = 1;
    arq__send_wnd_ptr_init(&p);
    CHECK_EQUAL(0, p.msg);
    CHECK_EQUAL(0, p.seg);
}

struct Fixture
{
    Fixture()
    {
        w.msg = m.data();
        arq__send_wnd_ptr_init(&p);
        arq__send_wnd_init(&w, m.size(), 128, 16);
    }
    arq__send_wnd_ptr_t p;
    arq__send_wnd_t w;
    std::array< arq__msg_t, 16 > m;
};

TEST(send_wnd_ptr, next_remains_invalid_if_invalid_and_nothing_to_send)
{
    Fixture f;
    arq__send_wnd_ptr_next(&f.p, &f.w);
    CHECK_EQUAL(0, f.p.valid);
}

TEST(send_wnd_ptr, next_changes_to_valid_if_invalid_and_message_in_window)
{
    Fixture f;
    f.w.msg[0].len = f.w.seg_len;
    arq__send_wnd_ptr_next(&f.p, &f.w);
    CHECK_EQUAL(1, f.p.valid);
}

TEST(send_wnd_ptr, next_ptr_msg_is_zero_if_first_msg_is_at_index_zero)
{
    Fixture f;
    f.w.msg[0].len = 4;
    f.w.base_idx = 0;
    f.p.msg = 1;
    arq__send_wnd_ptr_next(&f.p, &f.w);
    CHECK_EQUAL(0, f.p.msg);
}

TEST(send_wnd_ptr, next_ptr_seg_is_zero_if_ptr_was_invalid)
{
    Fixture f;
    f.w.msg[0].len = 1;
    f.p.seg = 1;
    arq__send_wnd_ptr_next(&f.p, &f.w);
    CHECK_EQUAL(0, f.p.seg);
}

TEST(send_wnd_ptr, next_ptr_remains_invalid_if_rtx_of_only_message_is_nonzero)
{
    Fixture f;
    f.w.msg[0].len = 1;
    f.w.msg[0].rtx = 100;
    arq__send_wnd_ptr_next(&f.p, &f.w);
    CHECK_EQUAL(0, f.p.valid);
}

TEST(send_wnd_ptr, next_ptr_remains_invalid_if_all_messages_have_nonzero_rtx)
{
    Fixture f;
    f.w.size = 4;
    for (int i = 0; i < f.w.size; ++i) {
        f.w.msg[i].len = 1;
        f.w.msg[i].rtx = 1;
    }
    arq__send_wnd_ptr_next(&f.p, &f.w);
    CHECK_EQUAL(0, f.p.valid);
}

TEST(send_wnd_ptr, next_sets_invalid_ptr_to_first_zero_rtx_msg)
{
    Fixture f;
    f.w.size = 2;
    f.w.msg[0].rtx = 1;
    f.w.msg[0].len = 16;
    f.w.msg[1].rtx = 0;
    f.w.msg[1].len = 16;
    arq__send_wnd_ptr_next(&f.p, &f.w);
    CHECK_EQUAL(1, f.p.msg);
}

TEST(send_wnd_ptr, next_increments_segment_if_not_at_final_seg)
{
    Fixture f;
    f.w.msg[0].len = f.w.seg_len + 1;
    f.w.msg[0].full_ack_vec = 0b11;
    f.p.valid = 1;
    arq__send_wnd_ptr_next(&f.p, &f.w);
    CHECK_EQUAL(1, f.p.seg);
}

TEST(send_wnd_ptr, next_can_iterate_across_entire_message)
{
    Fixture f;
    f.w.msg[0].len = f.w.msg_len;
    f.w.msg[0].full_ack_vec = (1 << (f.w.msg_len / f.w.seg_len)) - 1;
    f.p.valid = 0;
    for (int i = 0; i < (f.w.msg_len / f.w.seg_len); ++i) {
        arq__send_wnd_ptr_next(&f.p, &f.w);
        CHECK_EQUAL(0, f.p.msg);
        CHECK_EQUAL(i, f.p.seg);
        CHECK_EQUAL(1, f.p.valid);
    }
}

TEST(send_wnd_ptr, next_skips_msg_if_all_segs_are_already_acked)
{
    Fixture f;
    f.w.size = 2;
    f.w.msg[0].len = 1; // being retransmitted
    f.w.msg[0].rtx = 1;
    f.w.msg[0].full_ack_vec = 1;
    f.w.msg[0].cur_ack_vec = 0;
    f.w.msg[1].len = f.w.msg_len; // already fully ACK'd
    f.w.msg[1].full_ack_vec = 0xFF;
    f.w.msg[1].cur_ack_vec = 0xFF;
    f.w.msg[1].rtx = 0;
    f.p.valid = 1;
    arq__send_wnd_ptr_next(&f.p, &f.w);
    CHECK_EQUAL(0, f.p.valid);
}

TEST(send_wnd_ptr, next_invalid_ptr_skips_acked_segs_in_new_msg)
{
    Fixture f;
    f.w.msg[0].len = f.w.seg_len * 3;
    f.w.msg[0].rtx = 0;
    f.w.msg[0].full_ack_vec = 0b111;
    f.w.msg[0].cur_ack_vec = 0b101;
    f.p.valid = 0;
    arq__send_wnd_ptr_next(&f.p, &f.w);
    CHECK_EQUAL(1, f.p.valid);
    CHECK_EQUAL(1, f.p.seg);
}

TEST(send_wnd_ptr, next_valid_ptr_skips_acked_segs_in_current_msg)
{
    Fixture f;
    f.w.msg[0].len = f.w.seg_len * 4;
    f.w.msg[0].rtx = 0;
    f.w.msg[0].full_ack_vec = 0b1111;
    f.w.msg[0].cur_ack_vec = 0b1010;
    f.p.valid = 1;
    f.p.msg = 0;
    f.p.seg = 0;
    arq__send_wnd_ptr_next(&f.p, &f.w);
    CHECK_EQUAL(1, f.p.valid);
    CHECK_EQUAL(2, f.p.seg);
}

TEST(send_wnd_ptr, next_increments_msg_index_if_ptr_was_pointing_at_last_segment_of_nonfinal_message)
{
    Fixture f;
    f.w.size = 2;
    f.w.msg[0].len = (f.w.seg_len * 2) + 1;
    f.w.msg[0].full_ack_vec = 0b111;
    f.w.msg[0].rtx = 1;
    f.w.msg[1].len = 1;
    f.w.msg[1].full_ack_vec = 1;
    f.p.msg = 0;
    f.p.seg = 2;
    f.p.valid = 1;
    arq__send_wnd_ptr_next(&f.p, &f.w);
    CHECK_EQUAL(1, f.p.msg);
    CHECK_EQUAL(0, f.p.seg);
    CHECK_EQUAL(1, f.p.valid);
}

TEST(send_wnd_ptr, next_wraps_msg_index_to_first_zero_rtx_msg_in_window_if_pointing_at_final_seg)
{
    Fixture f;
    f.w.size = 2;
    f.w.base_idx = f.w.cap - 1;
    f.w.msg[f.w.base_idx].len = (f.w.seg_len * 2) + 1;
    f.w.msg[f.w.base_idx].full_ack_vec = 0b111;
    f.w.msg[f.w.base_idx].rtx = 1;
    f.p.msg = f.w.base_idx;
    f.p.seg = 2;
    f.p.valid = 1;
    f.w.msg[0].len = 1;
    f.w.msg[0].full_ack_vec = 1;
    arq__send_wnd_ptr_next(&f.p, &f.w);
    CHECK_EQUAL(0, f.p.msg);
    CHECK_EQUAL(0, f.p.seg);
    CHECK_EQUAL(1, f.p.valid);
}

TEST(send_wnd_ptr, next_changes_to_invalid_if_was_pointing_at_last_segment_of_last_message)
{
    Fixture f;
    f.w.size = 2;
    f.w.msg[0].len = 1;
    f.w.msg[0].full_ack_vec = 0b1;
    f.w.msg[0].rtx = 1;
    f.w.msg[1].len = f.w.seg_len + 1;
    f.w.msg[1].full_ack_vec = 0b11;
    f.w.msg[1].rtx = 1;
    f.p.msg = f.p.seg = f.p.valid = 1;
    arq__send_wnd_ptr_next(&f.p, &f.w);
    CHECK_EQUAL(0, f.p.valid);
}

TEST(send_wnd_ptr, next_changes_to_invalid_if_was_pointing_at_last_seg_of_last_msg_wnd_size_one)
{
    Fixture f;
    f.w.size = 1;
    f.w.msg[0].len = 1;
    f.w.msg[0].full_ack_vec = 1;
    f.w.msg[0].rtx = 1;
    f.w.msg[1].len = 1; // out of bounds, f.w.size == 1
    f.w.msg[1].full_ack_vec = 1;
    f.w.msg[1].rtx = 0;
    f.p.valid = 1;
    f.p.msg = 0;
    f.p.seg = 0;
    arq__send_wnd_ptr_next(&f.p, &f.w);
    CHECK_EQUAL(0, f.p.valid);
}

}

