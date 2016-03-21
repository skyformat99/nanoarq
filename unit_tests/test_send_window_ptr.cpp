#include "arq_in_unit_tests.h"
#include <CppUTest/TestHarness.h>
#include <array>
#include <cstring>

TEST_GROUP(send_wnd_ptr) {};

namespace {

TEST(send_wnd_ptr, init_sets_valid_to_zero)
{
    arq__send_wnd_ptr_t p;
    p.valid = 1;
    arq__send_wnd_ptr_init(&p);
    CHECK_EQUAL(0, p.valid);
}

TEST(send_wnd_ptr, init_sets_seq_and_seg_to_zero)
{
    arq__send_wnd_ptr_t p;
    p.valid = p.seq = p.seg = 1;
    arq__send_wnd_ptr_init(&p);
    CHECK_EQUAL(0, p.seq);
    CHECK_EQUAL(0, p.seg);
}

struct Fixture
{
    Fixture()
    {
        sw.w.msg = m.data();
        sw.rtx = rtx.data();
        arq__send_wnd_ptr_init(&p);
        arq__wnd_init(&sw.w, m.size(), 128, 16);
        arq__send_wnd_rst(&sw);
    }
    arq__send_wnd_ptr_t p;
    arq__send_wnd_t sw;
    std::array< arq__msg_t, 16 > m;
    std::array< arq_time_t, 16 > rtx;
};

TEST(send_wnd_ptr, next_remains_invalid_if_invalid_and_nothing_to_send)
{
    Fixture f;
    arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(0, f.p.valid);
}

TEST(send_wnd_ptr, next_changes_to_valid_if_invalid_and_message_in_window)
{
    Fixture f;
    f.sw.w.size = 1;
    f.sw.w.msg[0].len = f.sw.w.seg_len;
    arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(1, f.p.valid);
}

TEST(send_wnd_ptr, next_ptr_msg_is_zero_if_first_msg_is_at_index_zero)
{
    Fixture f;
    f.sw.w.size = 1;
    f.sw.w.msg[0].len = 4;
    f.p.seq = 1;
    arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(0, f.p.seq);
}

TEST(send_wnd_ptr, next_ptr_seg_is_zero_if_ptr_was_invalid)
{
    Fixture f;
    f.sw.w.size = 1;
    f.sw.w.msg[0].len = 1;
    f.p.seg = 1;
    arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(0, f.p.seg);
}

TEST(send_wnd_ptr, next_ptr_remains_invalid_if_rtx_of_only_message_is_nonzero)
{
    Fixture f;
    f.sw.w.msg[0].len = 1;
    f.sw.rtx[0] = 100;
    arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(0, f.p.valid);
}

TEST(send_wnd_ptr, next_ptr_remains_invalid_if_all_messages_have_nonzero_rtx)
{
    Fixture f;
    f.sw.w.size = 4;
    for (int i = 0; i < f.sw.w.size; ++i) {
        f.sw.w.msg[i].len = 1;
        f.sw.rtx[i] = 1;
    }
    arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(0, f.p.valid);
}

TEST(send_wnd_ptr, next_sets_invalid_ptr_to_first_zero_rtx_msg)
{
    Fixture f;
    f.sw.w.size = 2;
    f.sw.rtx[0] = 1;
    f.sw.w.msg[0].len = 16;
    f.sw.rtx[1] = 0;
    f.sw.w.msg[1].len = 16;
    arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(1, f.p.seq);
}

TEST(send_wnd_ptr, next_increments_segment_if_not_at_final_seg)
{
    Fixture f;
    f.sw.w.msg[0].len = f.sw.w.seg_len + 1;
    f.sw.w.msg[0].full_ack_vec = 0b11;
    f.p.valid = 1;
    arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(1, f.p.seg);
}

TEST(send_wnd_ptr, next_can_iterate_across_entire_message)
{
    Fixture f;
    f.sw.w.size = 1;
    f.sw.w.msg[0].len = f.sw.w.msg_len;
    f.sw.w.msg[0].full_ack_vec = (1 << (f.sw.w.msg_len / f.sw.w.seg_len)) - 1;
    f.p.valid = 0;
    for (int i = 0; i < (f.sw.w.msg_len / f.sw.w.seg_len); ++i) {
        arq__send_wnd_ptr_next(&f.p, &f.sw);
        CHECK_EQUAL(0, f.p.seq);
        CHECK_EQUAL(i, f.p.seg);
        CHECK_EQUAL(1, f.p.valid);
    }
}

TEST(send_wnd_ptr, next_skips_msg_if_all_segs_are_already_acked)
{
    Fixture f;
    f.sw.w.size = 2;
    f.sw.w.msg[0].len = 1; // being retransmitted
    f.sw.rtx[0] = 1;
    f.sw.w.msg[0].full_ack_vec = 1;
    f.sw.w.msg[0].cur_ack_vec = 0;
    f.sw.w.msg[1].len = f.sw.w.msg_len; // already fully ACK'd
    f.sw.w.msg[1].full_ack_vec = 0xFF;
    f.sw.w.msg[1].cur_ack_vec = 0xFF;
    f.sw.rtx[1] = 0;
    f.p.valid = 1;
    arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(0, f.p.valid);
}

TEST(send_wnd_ptr, next_invalid_ptr_skips_acked_segs_in_new_msg)
{
    Fixture f;
    f.sw.w.size = 1;
    f.sw.w.msg[0].len = f.sw.w.seg_len * 3;
    f.sw.rtx[0] = 0;
    f.sw.w.msg[0].full_ack_vec = 0b111;
    f.sw.w.msg[0].cur_ack_vec = 0b101;
    f.p.valid = 0;
    arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(1, f.p.valid);
    CHECK_EQUAL(1, f.p.seg);
}

TEST(send_wnd_ptr, next_valid_ptr_skips_acked_segs_in_current_msg)
{
    Fixture f;
    f.sw.w.msg[0].len = f.sw.w.seg_len * 4;
    f.sw.rtx[0] = 0;
    f.sw.w.msg[0].full_ack_vec = 0b1111;
    f.sw.w.msg[0].cur_ack_vec = 0b1010;
    f.p.valid = 1;
    f.p.seq = 0;
    f.p.seg = 0;
    arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(1, f.p.valid);
    CHECK_EQUAL(2, f.p.seg);
}

TEST(send_wnd_ptr, next_increments_msg_index_if_ptr_was_pointing_at_last_segment_of_nonfinal_message)
{
    Fixture f;
    f.sw.w.size = 2;
    f.sw.w.msg[0].len = (f.sw.w.seg_len * 2) + 1;
    f.sw.w.msg[0].full_ack_vec = 0b111;
    f.sw.rtx[0] = 1;
    f.sw.w.msg[1].len = 1;
    f.sw.w.msg[1].full_ack_vec = 1;
    f.p.seq = 0;
    f.p.seg = 2;
    f.p.valid = 1;
    arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(1, f.p.seq);
    CHECK_EQUAL(0, f.p.seg);
    CHECK_EQUAL(1, f.p.valid);
}

TEST(send_wnd_ptr, next_wraps_seq_to_zero_from_max)
{
    Fixture f;
    f.sw.w.size = 2;
    f.sw.w.seq = ARQ__FRAME_MAX_SEQ_NUM;
    f.sw.w.msg[f.sw.w.seq % f.sw.w.cap].len = (f.sw.w.seg_len * 2) + 1;
    f.sw.w.msg[f.sw.w.seq % f.sw.w.cap].full_ack_vec = 0b111;
    f.sw.rtx[f.sw.w.seq % f.sw.w.cap] = 1;
    f.p.seq = f.sw.w.seq;
    f.p.seg = 2;
    f.p.valid = 1;
    f.sw.w.msg[(f.sw.w.seq + 1) % f.sw.w.cap].len = 1;
    f.sw.w.msg[(f.sw.w.seq + 1) % f.sw.w.cap].full_ack_vec = 1;
    arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(0, f.p.seq);
    CHECK_EQUAL(0, f.p.seg);
    CHECK_EQUAL(1, f.p.valid);
}

TEST(send_wnd_ptr, next_wraps_seq_past_zero_to_first_sendable_message_from_max)
{
    Fixture f;
    f.sw.w.size = 3;
    f.sw.w.seq = ARQ__FRAME_MAX_SEQ_NUM;
    f.sw.w.msg[f.sw.w.seq % f.sw.w.cap].len = (f.sw.w.seg_len * 2) + 1;
    f.sw.w.msg[f.sw.w.seq % f.sw.w.cap].full_ack_vec = 0b111;
    f.sw.rtx[f.sw.w.seq % f.sw.w.cap] = 1;
    f.p.seq = f.sw.w.seq;
    f.p.seg = 2;
    f.p.valid = 1;
    f.sw.w.msg[(f.sw.w.seq + 1) % f.sw.w.cap].len = 1;
    f.sw.w.msg[(f.sw.w.seq + 1) % f.sw.w.cap].full_ack_vec = 1;
    f.sw.rtx[(f.sw.w.seq + 1) % f.sw.w.cap] = 1;
    f.sw.w.msg[(f.sw.w.seq + 2) % f.sw.w.cap].len = 1;
    f.sw.w.msg[(f.sw.w.seq + 2) % f.sw.w.cap].full_ack_vec = 1;
    f.sw.rtx[(f.sw.w.seq + 2) % f.sw.w.cap] = 0;
    arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(1, f.p.seq);
    CHECK_EQUAL(0, f.p.seg);
    CHECK_EQUAL(1, f.p.valid);
}

TEST(send_wnd_ptr, next_changes_to_invalid_if_was_pointing_at_last_segment_of_last_message)
{
    Fixture f;
    f.sw.w.size = 2;
    f.sw.w.msg[0].len = 1;
    f.sw.w.msg[0].full_ack_vec = 0b1;
    f.sw.rtx[0] = 1;
    f.sw.w.msg[1].len = f.sw.w.seg_len + 1;
    f.sw.w.msg[1].full_ack_vec = 0b11;
    f.sw.rtx[1] = 1;
    f.p.seq = f.p.seg = f.p.valid = 1;
    arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(0, f.p.valid);
}

TEST(send_wnd_ptr, next_changes_to_invalid_if_was_pointing_at_last_seg_of_last_msg_wnd_size_one)
{
    Fixture f;
    f.sw.w.size = 1;
    f.sw.w.msg[0].len = 1;
    f.sw.w.msg[0].full_ack_vec = 1;
    f.sw.rtx[0] = 1;
    f.sw.w.msg[1].len = 1; // out of bounds, f.sw.w.size == 1
    f.sw.w.msg[1].full_ack_vec = 1;
    f.sw.rtx[1] = 0;
    f.p.valid = 1;
    f.p.seq = 0;
    f.p.seg = 0;
    arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(0, f.p.valid);
}

TEST(send_wnd_ptr, next_skips_current_message_when_advancing)
{
    Fixture f;
    f.sw.w.size = 2;
    f.sw.w.msg[0].len = 1;
    f.sw.w.msg[0].full_ack_vec = 1;
    f.sw.w.msg[1].len = 1;
    f.sw.w.msg[1].full_ack_vec = 1;
    f.p.valid = 1;
    arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(1, f.p.seq);
}

TEST(send_wnd_ptr, next_returns_zero_if_the_pointer_stays_inside_of_current_msg)
{
    Fixture f;
    f.sw.w.msg[0].len = f.sw.w.seg_len * 3;
    f.p.valid = 1;
    arq__send_wnd_ptr_next_result_t const rv = arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(ARQ__SEND_WND_PTR_NEXT_INSIDE_MSG, rv);
}

TEST(send_wnd_ptr, next_returns_zero_when_pointer_goes_to_last_seg_in_longer_msg)
{
    Fixture f;
    f.sw.w.msg[0].len = f.sw.w.seg_len * 3;
    f.p.seg = f.p.valid = 1;
    arq__send_wnd_ptr_next_result_t const rv = arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(ARQ__SEND_WND_PTR_NEXT_INSIDE_MSG, rv);
}

TEST(send_wnd_ptr, next_returns_zero_when_moving_to_first_msg_from_invalid)
{
    Fixture f;
    f.sw.w.msg[0].len = 1;
    f.p.valid = 0;
    arq__send_wnd_ptr_next_result_t const rv = arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(ARQ__SEND_WND_PTR_NEXT_INSIDE_MSG, rv);
}

TEST(send_wnd_ptr, next_returns_one_when_ptr_moves_past_msg_with_one_seg)
{
    Fixture f;
    f.sw.w.size = 2;
    f.sw.w.msg[0].len = f.sw.w.msg[0].full_ack_vec = 1;
    f.sw.w.msg[1].len = 1;
    f.p.valid = 1;
    arq__send_wnd_ptr_next_result_t const rv = arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(ARQ__SEND_WND_PTR_NEXT_COMPLETED_MSG, rv);
}

TEST(send_wnd_ptr, next_returns_one_when_ptr_moves_past_last_seg_in_msg)
{
    Fixture f;
    f.sw.w.size = 2;
    f.sw.w.msg[0].len = f.sw.w.seg_len * 3;
    f.sw.rtx[0] = 1;
    f.sw.w.msg[0].full_ack_vec = 0b111;
    f.sw.w.msg[1].len = 1;
    f.p.seg = 2;
    f.p.valid = 1;
    arq__send_wnd_ptr_next_result_t const rv = arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(ARQ__SEND_WND_PTR_NEXT_COMPLETED_MSG, rv);
}

TEST(send_wnd_ptr, next_returns_one_when_finishing_only_msg)
{
    Fixture f;
    f.sw.w.msg[0].len = f.sw.w.msg[0].full_ack_vec = 1;
    f.p.valid = 1;
    arq__send_wnd_ptr_next_result_t const rv = arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(ARQ__SEND_WND_PTR_NEXT_COMPLETED_MSG, rv);
}

TEST(send_wnd_ptr, next_returns_one_when_finishing_final_msg)
{
    Fixture f;
    f.sw.w.size = 2;
    f.sw.w.msg[0].len = f.sw.rtx[0] = f.sw.w.msg[0].full_ack_vec = 1;
    f.sw.w.msg[1].len = f.sw.w.msg[1].full_ack_vec = 1;
    f.p.valid = 1;
    f.p.seq = 1;
    arq__send_wnd_ptr_next_result_t const rv = arq__send_wnd_ptr_next(&f.p, &f.sw);
    CHECK_EQUAL(ARQ__SEND_WND_PTR_NEXT_COMPLETED_MSG, rv);
}

}

