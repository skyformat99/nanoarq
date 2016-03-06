#include "nanoarq_unit_test.h"
#include "nanoarq_hook_plugin.h"
#include <CppUTestExt/MockSupport.h>
#include <CppUTest/TestHarness.h>
#include <cstring>
#include <vector>
#include <array>
#include <algorithm>

TEST_GROUP(send_wnd) {};

namespace {

struct UninitializedSendWindowFixture
{
    UninitializedSendWindowFixture()
    {
        wnd.w.buf = nullptr;
        wnd.w.msg = msg.data();
        wnd.rtx = rtx.data();
    }
    arq__send_wnd_t wnd;
    std::array< arq__msg_t, 64 > msg;
    std::array< arq_time_t, 64 > rtx;
};

TEST(send_wnd, init_writes_params_to_struct)
{
    UninitializedSendWindowFixture f;
    arq__send_wnd_init(&f.wnd, f.msg.size(), 16, 8);
    CHECK_EQUAL(f.msg.size(), f.wnd.w.cap);
    CHECK_EQUAL(16, f.wnd.w.msg_len);
    CHECK_EQUAL(8, f.wnd.w.seg_len);
}

TEST(send_wnd, init_full_ack_vec_is_one_if_seg_len_equals_msg_len)
{
    UninitializedSendWindowFixture f;
    arq__send_wnd_init(&f.wnd, f.msg.size(), 16, 16);
    CHECK_EQUAL(0b1, f.wnd.w.full_ack_vec);
}

TEST(send_wnd, init_calculates_full_ack_vector_from_msg_len_and_seg_len)
{
    UninitializedSendWindowFixture f;
    arq__send_wnd_init(&f.wnd, f.msg.size(), 32, 8);
    CHECK_EQUAL(0b1111, f.wnd.w.full_ack_vec);
}

void MockSendWndRst(arq__send_wnd_t *w)
{
    mock().actualCall("arq__send_wnd_rst").withParameter("w", w);
}

TEST(send_wnd, init_calls_rst)
{
    UninitializedSendWindowFixture f;
    ARQ_MOCK_HOOK(arq__send_wnd_rst, MockSendWndRst);
    mock().expectOneCall("arq__send_wnd_rst").withParameter("w", &f.wnd);
    arq__send_wnd_init(&f.wnd, f.msg.size(), 16, 8);
}

struct Fixture : UninitializedSendWindowFixture
{
    Fixture()
    {
        arq__send_wnd_init(&wnd, msg.size(), 128, 16);
        buf.resize(wnd.w.msg_len * wnd.w.cap);
        wnd.w.buf = buf.data();
    }
    std::vector< unsigned char > buf;
    std::vector< unsigned char > snd;
};

TEST(send_wnd, rst_resets_window)
{
    Fixture f;
    f.wnd.w.size = 13;
    f.wnd.w.seq = 4321;
    arq__send_wnd_rst(&f.wnd);
    CHECK_EQUAL(0, f.wnd.w.seq);
    CHECK_EQUAL(0, f.wnd.w.size);
}

TEST(send_wnd, rst_resets_messages_to_default)
{
    Fixture f;
    for (auto &m : f.msg) {
        m.len = 1234;
        m.cur_ack_vec = 100;
        m.full_ack_vec = 123;
    }
    f.wnd.w.full_ack_vec = 0xABCD;
    arq__send_wnd_rst(&f.wnd);
    for (auto const &m : f.msg) {
        CHECK_EQUAL(0, m.len);
        CHECK_EQUAL(0, m.cur_ack_vec);
        CHECK_EQUAL(f.wnd.w.full_ack_vec, m.full_ack_vec);
    }
}

TEST(send_wnd, rst_resets_rtx_to_default)
{
    Fixture f;
    for (auto &t : f.rtx) {
        t = 1234;
    }
    arq__send_wnd_rst(&f.wnd);
    for (auto const &t : f.rtx) {
        CHECK_EQUAL(0, t);
    }
}

TEST(send_wnd, send_size_increases_if_payload_uses_new_message)
{
    Fixture f;
    f.snd.resize(1);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(1, f.wnd.w.size);
}

TEST(send_wnd, send_with_empty_window_returns_full_size_written)
{
    Fixture f;
    f.snd.resize(5);
    int const written = arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(5, written);
}

TEST(send_wnd, send_small_with_empty_message_sets_message_size)
{
    Fixture f;
    f.snd.resize(5);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(5, f.msg[0].len);
}

TEST(send_wnd, send_with_empty_window_copies_to_start_of_buf)
{
    Fixture f;
    f.snd.resize(5);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    MEMCMP_EQUAL(f.snd.data(), f.buf.data(), f.snd.size());
}

TEST(send_wnd, send_with_partially_full_window_appends)
{
    Fixture f;
    f.snd.resize(10);
    for (size_t i = 0; i < f.snd.size(); ++i) {
        f.snd[i] = (unsigned char)i;
    }
    std::fill(std::begin(f.buf), std::end(f.buf), 0xFF);
    f.wnd.w.msg[0].len = 3;
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    MEMCMP_EQUAL(f.snd.data(), &f.buf[3], f.snd.size());
}

TEST(send_wnd, send_with_partially_full_window_updates_message_length)
{
    Fixture f;
    f.wnd.w.size = 1;
    f.wnd.w.msg[0].len = 12;
    f.snd.resize(15);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(12 + 15, f.msg[0].len);
}

TEST(send_wnd, send_more_than_one_message_updates_lengths_in_first_two_messages)
{
    Fixture f;
    f.snd.resize(f.wnd.w.msg_len + 7);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(f.wnd.w.msg_len, f.msg[0].len);
    CHECK_EQUAL((int)f.snd.size() % f.wnd.w.msg_len, f.msg[1].len);
    CHECK_EQUAL(0, f.msg[2].len);
}

TEST(send_wnd, send_one_full_message_sets_rtx_to_zero)
{
    Fixture f;
    f.snd.resize(f.wnd.w.msg_len);
    f.wnd.w.size = 1;
    f.wnd.rtx[0] = 1;
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(0, f.wnd.rtx[0]);
}

TEST(send_wnd, send_two_messages_sets_rtx_to_zero_for_full_message)
{
    Fixture f;
    f.snd.resize(f.wnd.w.msg_len + 1);
    f.wnd.w.size = 1;
    f.wnd.rtx[0] = 1;
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(0, f.wnd.rtx[0]);
}

TEST(send_wnd, send_more_than_two_messages_updates_lengths_in_first_three_messages)
{
    Fixture f;
    f.snd.resize(f.wnd.w.msg_len * 2 + 7);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(f.wnd.w.msg_len, f.msg[0].len);
    CHECK_EQUAL(f.wnd.w.msg_len, f.msg[1].len);
    CHECK_EQUAL((int)f.snd.size() % f.wnd.w.msg_len, f.msg[2].len);
    CHECK_EQUAL(0, f.msg[3].len);
}

TEST(send_wnd, send_three_messages_sets_rtx_to_zero_for_first_two_messages)
{
    Fixture f;
    f.snd.resize(f.wnd.w.msg_len * 2 + 1);
    f.wnd.rtx[0] = f.wnd.rtx[1] = 1;
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(0, f.wnd.rtx[0]);
    CHECK_EQUAL(0, f.wnd.rtx[1]);
}

TEST(send_wnd, send_more_than_one_messages_increments_size_by_number_of_messages)
{
    Fixture f;
    f.snd.resize(f.wnd.w.msg_len * 4);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(4, f.wnd.w.size);
}

TEST(send_wnd, send_perfectly_completing_a_message_doesnt_increment_size)
{
    Fixture f;
    f.wnd.w.size = 2;
    f.wnd.w.msg[0].len = f.wnd.w.msg_len;
    f.wnd.w.msg[1].len = f.wnd.w.msg_len - 12;
    f.snd.resize(12);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(2, f.wnd.w.size);
}

TEST(send_wnd, send_when_cur_index_greater_than_zero_updates_correct_message_len)
{
    Fixture f;
    f.wnd.w.size = 2;
    f.wnd.w.seq = 1;
    f.msg[2].len = 3;
    f.snd.resize(15);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(3 + 15, f.msg[2].len);
}

TEST(send_wnd, send_when_cur_index_greater_than_zero_more_than_one_message_updates_sizes)
{
    Fixture f;
    f.wnd.w.seq = 2;
    f.snd.resize(f.wnd.w.msg_len + 1);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(f.wnd.w.msg_len, f.msg[2].len);
    CHECK_EQUAL(1, f.msg[3].len);
    CHECK_EQUAL(0, f.msg[4].len);
}

TEST(send_wnd, send_updates_messages_at_beginning_of_msg_array_when_copy_wraps_around)
{
    Fixture f;
    f.wnd.w.seq = f.wnd.w.cap - 1;
    f.snd.resize(f.wnd.w.msg_len * 3);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(f.wnd.w.msg_len, f.msg[f.wnd.w.cap - 1].len);
    CHECK_EQUAL(f.wnd.w.msg_len, f.msg[0].len);
    CHECK_EQUAL(f.wnd.w.msg_len, f.msg[1].len);
    CHECK_EQUAL(0, f.msg[2].len);
}

TEST(send_wnd, send_more_data_than_window_space_returns_bytes_sent)
{
    Fixture f;
    f.snd.resize(((f.wnd.w.msg_len * f.wnd.w.cap) + 1));
    int const written = arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(f.wnd.w.msg_len * f.wnd.w.cap, written);
}

TEST(send_wnd, send_to_full_window_returns_zero)
{
    Fixture f;
    f.wnd.w.size = f.wnd.w.cap;
    std::for_each(std::begin(f.msg), std::end(f.msg), [&](arq__msg_t& m) { m.len = f.wnd.w.msg_len; });
    f.snd.resize(16);
    int const written = arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(0, written);
}

TEST(send_wnd, send_more_data_than_window_space_with_partial_msg_returns_rest_of_window)
{
    Fixture f;
    f.wnd.w.cap = 3;
    f.wnd.w.size = 3;
    f.wnd.w.msg[0].len = f.wnd.w.msg_len;
    f.wnd.w.msg[1].len = f.wnd.w.msg_len;
    f.wnd.w.msg[2].len = 27;
    int const cap_in_bytes = f.wnd.w.cap * f.wnd.w.msg_len;
    int const size_in_bytes = f.wnd.w.msg[0].len + f.wnd.w.msg[1].len + f.wnd.w.msg[2].len;
    f.snd.resize(cap_in_bytes);
    int const written = arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(cap_in_bytes - size_in_bytes, written);
}

TEST(send_wnd, send_copies_data_to_current_message_space_in_buf)
{
    Fixture f;
    f.wnd.w.seq = 1;
    f.snd.resize(19);
    for (size_t i = 0; i < f.snd.size(); ++i) {
        f.snd[i] = (unsigned char)i;
    }
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    MEMCMP_EQUAL(f.snd.data(), &f.buf[f.wnd.w.msg_len], f.snd.size());
}

TEST(send_wnd, send_wraps_copy_around_if_inside_window_at_end_of_buf)
{
    Fixture f;
    int const orig_msg_idx = f.wnd.w.cap - 1;
    f.wnd.w.seq = orig_msg_idx;
    f.snd.resize(f.wnd.w.msg_len * 2);
    for (size_t i = 0; i < f.snd.size(); ++i) {
        f.snd[i] = (unsigned char)i;
    }
    std::fill(std::begin(f.buf), std::end(f.buf), 0xFE);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    MEMCMP_EQUAL(f.snd.data(), &f.buf[(orig_msg_idx * f.wnd.w.msg_len)], f.wnd.w.msg_len);
    MEMCMP_EQUAL(&f.snd[f.wnd.w.msg_len], f.buf.data(), f.wnd.w.msg_len);
}

TEST(send_wnd, send_wraps_copy_around_and_respects_partially_filled_starting_message)
{
    Fixture f;
    f.snd.resize(f.wnd.w.msg_len);
    for (size_t i = 0; i < f.snd.size(); ++i) {
        f.snd[i] = (unsigned char)i;
    }
    std::fill(std::begin(f.buf), std::end(f.buf), 0xFE);
    int const orig_msg_idx = f.wnd.w.cap - 1;
    f.wnd.w.seq = orig_msg_idx;
    f.msg[orig_msg_idx].len = 3;
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    MEMCMP_EQUAL(f.snd.data(), &f.buf[(orig_msg_idx * f.wnd.w.msg_len) + 3], f.wnd.w.msg_len - 3);
    MEMCMP_EQUAL(&f.snd[f.wnd.w.msg_len - 3], f.buf.data(), 3);
}

TEST(send_wnd, send_payload_is_message_size_wrap_window)
{
    // this edge case teased out a bug in arq__send_wnd_send
    Fixture f;
    f.wnd.w.cap = 4;
    f.wnd.w.size = 3;
    f.wnd.w.seq = 1;
    f.wnd.w.msg_len = 32;
    f.wnd.w.full_ack_vec = 0b11111;
    f.wnd.w.msg[1].len = 32;
    f.wnd.w.msg[2].len = 32;
    f.wnd.w.msg[3].len = 32;
    f.snd.resize(32);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(32, f.wnd.w.msg[0].len);
}

TEST(send_wnd, ack_seq_outside_of_send_wnd_greater_does_nothing)
{
    Fixture f;
    f.wnd.w.size = 1;
    f.wnd.w.cap = 2;
    f.wnd.w.seq = 0;
    arq__send_wnd_ack(&f.wnd, 1, 1);
    CHECK_EQUAL(0, f.msg[1].cur_ack_vec);
    CHECK_EQUAL(0, f.wnd.w.seq);
}

TEST(send_wnd, ack_seq_outside_of_send_wnd_less_does_nothing)
{
    Fixture f;
    f.wnd.w.size = 4;
    f.wnd.w.seq = 16;
    arq__send_wnd_ack(&f.wnd, 15, 1);
    CHECK_EQUAL(0, f.msg[15].cur_ack_vec);
    CHECK_EQUAL(16, f.wnd.w.seq);
}

TEST(send_wnd, ack_empty_window_does_nothing)
{
    Fixture f;
    f.wnd.w.size = 0;
    f.wnd.w.seq = 5;
    arq__send_wnd_ack(&f.wnd, 5, 1);
    CHECK_EQUAL(0, f.msg[5].cur_ack_vec);
    CHECK_EQUAL(0, f.wnd.w.size);
}

TEST(send_wnd, ack_partial_ack_vec_doesnt_slide_window)
{
    Fixture f;
    arq__send_wnd_ack(&f.wnd, f.wnd.w.seq, f.wnd.w.full_ack_vec - 1);
    CHECK_EQUAL(0, f.wnd.w.seq);
}

TEST(send_wnd, ack_first_seq_slides_and_increments_base_seq)
{
    Fixture f;
    f.wnd.w.size = 1;
    arq__send_wnd_ack(&f.wnd, f.wnd.w.seq, f.wnd.w.full_ack_vec);
    CHECK_EQUAL(1, f.wnd.w.seq);
}

TEST(send_wnd, ack_full_msg_decrements_size)
{
    Fixture f;
    f.wnd.w.size = 1;
    arq__send_wnd_ack(&f.wnd, f.wnd.w.seq, f.wnd.w.full_ack_vec);
    CHECK_EQUAL(0, f.wnd.w.size);
}

TEST(send_wnd, ack_sliding_increments_base_seq)
{
    Fixture f;
    f.wnd.w.size = 1;
    arq__send_wnd_ack(&f.wnd, f.wnd.w.seq, f.wnd.w.full_ack_vec);
    CHECK_EQUAL(1, f.wnd.w.seq);
}

TEST(send_wnd, ack_resets_len_and_cur_ack_vec_when_sliding)
{
    Fixture f;
    f.wnd.w.size = 1;
    f.wnd.w.msg[0].cur_ack_vec = f.wnd.w.full_ack_vec;
    f.wnd.w.msg[0].len = 1234;
    arq__send_wnd_ack(&f.wnd, 0, f.wnd.w.full_ack_vec);
    CHECK_EQUAL(0, f.wnd.w.msg[0].cur_ack_vec);
    CHECK_EQUAL(0, f.wnd.w.msg[0].len);
}

TEST(send_wnd, ack_resets_full_ack_vec_when_sliding)
{
    Fixture f;
    f.wnd.w.size = 1;
    f.wnd.w.msg[0].cur_ack_vec = f.wnd.w.msg[0].full_ack_vec = 0b1111;
    arq__send_wnd_ack(&f.wnd, 0, 0b1111);
    CHECK_EQUAL(f.wnd.w.full_ack_vec, f.wnd.w.msg[0].full_ack_vec);
}

TEST(send_wnd, ack_second_seq_doesnt_slide)
{
    Fixture f;
    f.wnd.w.size = 2;
    arq__send_wnd_ack(&f.wnd, f.wnd.w.seq + 1, f.wnd.w.full_ack_vec);
    CHECK_EQUAL(0, f.wnd.w.seq);
}

TEST(send_wnd, ack_max_seq_num_wraps_to_zero)
{
    Fixture f;
    f.wnd.w.size = 1;
    f.wnd.w.seq = ARQ__FRAME_MAX_SEQ_NUM;
    arq__send_wnd_ack(&f.wnd, f.wnd.w.seq, f.wnd.w.full_ack_vec);
    CHECK_EQUAL(0, f.wnd.w.seq);
}

TEST(send_wnd, ack_first_seq_copies_cur_ack_vec)
{
    Fixture f;
    f.wnd.w.size = 1;
    arq__send_wnd_ack(&f.wnd, f.wnd.w.seq, 1u);
    CHECK_EQUAL(1u, f.wnd.w.msg[0].cur_ack_vec);
}

TEST(send_wnd, ack_slides_to_first_unackd_message)
{
    Fixture f;
    f.wnd.w.size = 5;
    f.wnd.w.msg[0].cur_ack_vec = 0;
    f.wnd.w.msg[1].cur_ack_vec = f.wnd.w.full_ack_vec;
    f.wnd.w.msg[2].cur_ack_vec = f.wnd.w.full_ack_vec;
    f.wnd.w.msg[3].cur_ack_vec = f.wnd.w.full_ack_vec;
    f.wnd.w.msg[4].cur_ack_vec = 0;
    arq__send_wnd_ack(&f.wnd, 0, f.wnd.w.full_ack_vec);
    CHECK_EQUAL(4, f.wnd.w.seq);
}

TEST(send_wnd, ack_the_entire_send_window)
{
    Fixture f;
    f.wnd.w.size = f.wnd.w.cap + 1;
    for (auto &m : f.msg) {
        m.cur_ack_vec = m.full_ack_vec = f.wnd.w.full_ack_vec;
        m.len = f.wnd.w.msg_len;
    }
    --f.wnd.w.msg[0].cur_ack_vec;
    arq__send_wnd_ack(&f.wnd, 0, f.wnd.w.full_ack_vec);
    CHECK_EQUAL(1, f.wnd.w.size);
    CHECK_EQUAL(f.wnd.w.cap, f.wnd.w.seq);
}

TEST(send_wnd, flush_does_nothing_on_empty_window)
{
    Fixture f;
    arq__send_wnd_flush(&f.wnd);
    CHECK_EQUAL(0, f.wnd.w.size);
}

TEST(send_wnd, flush_does_nothing_if_current_message_has_zero_length)
{
    Fixture f;
    f.wnd.w.size = 2;
    arq__send_wnd_flush(&f.wnd);
    CHECK_EQUAL(2, f.wnd.w.size);
}

TEST(send_wnd, flush_increments_size_if_current_message_has_nonzero_length)
{
    Fixture f;
    f.msg[0].len = 12;
    arq__send_wnd_flush(&f.wnd);
    CHECK_EQUAL(1, f.wnd.w.size);
}

TEST(send_wnd, flush_does_nothing_on_full_window)
{
    Fixture f;
    f.wnd.w.size = f.wnd.w.cap + 1;
    for (auto &m : f.msg) {
        m.len = f.wnd.w.msg_len;
    }
    arq__send_wnd_flush(&f.wnd);
    CHECK_EQUAL(f.wnd.w.cap + 1, f.wnd.w.size);
}

TEST(send_wnd, flush_sets_full_ack_vector_to_one_if_less_than_one_segment)
{
    Fixture f;
    f.msg[0].len = 1;
    arq__send_wnd_flush(&f.wnd);
    CHECK_EQUAL(0b1, f.msg[0].full_ack_vec);
}

TEST(send_wnd, flush_number_of_bits_in_ack_vector_is_number_of_segments)
{
    Fixture f;
    f.msg[0].len = f.wnd.w.seg_len * 5;
    arq__send_wnd_flush(&f.wnd);
    CHECK_EQUAL(0b11111, f.msg[0].full_ack_vec);
}

TEST(send_wnd, flush_makes_msg_full_ack_vector_from_current_message_size)
{
    Fixture f;
    f.msg[0].len = f.wnd.w.seg_len * 2 + 1;
    arq__send_wnd_flush(&f.wnd);
    CHECK_EQUAL(0b111, f.msg[0].full_ack_vec);
}

TEST(send_wnd, flush_acts_on_final_incomplete_message)
{
    Fixture f;
    f.wnd.w.size = 3;
    f.msg[2].len = 1;
    arq__send_wnd_flush(&f.wnd);
    CHECK_EQUAL(0b1, f.msg[2].full_ack_vec);
}

TEST(send_wnd, step_decrements_first_timer_in_window_by_dt)
{
    Fixture f;
    f.wnd.w.size = 1;
    f.wnd.rtx[0] = 100;
    arq__send_wnd_step(&f.wnd, 10);
    CHECK_EQUAL(90, f.wnd.rtx[0]);
}

TEST(send_wnd, step_saturates_rtx_to_zero)
{
    Fixture f;
    f.wnd.w.size = 1;
    f.wnd.rtx[0] = 100;
    arq__send_wnd_step(&f.wnd, f.wnd.rtx[0] + 1);
    CHECK_EQUAL(0, f.wnd.rtx[0]);
}

TEST(send_wnd, step_decrements_all_timers_in_window)
{
    Fixture f;
    f.wnd.w.size = 3;
    f.wnd.rtx[0] = 100;
    f.wnd.rtx[1] = 90;
    f.wnd.rtx[2] = 80;
    f.wnd.rtx[3] = 1000;
    arq__send_wnd_step(&f.wnd, 80);
    CHECK_EQUAL(20, f.wnd.rtx[0]);
    CHECK_EQUAL(10, f.wnd.rtx[1]);
    CHECK_EQUAL(0, f.wnd.rtx[2]);
    CHECK_EQUAL(1000, f.wnd.rtx[3]);
}

TEST(send_wnd, step_operates_on_messages_in_window)
{
    Fixture f;
    f.wnd.w.size = 1;
    f.wnd.w.seq = f.wnd.w.cap / 2;
    f.wnd.rtx[f.wnd.w.seq] = 100;
    arq__send_wnd_step(&f.wnd, 10);
    CHECK_EQUAL(90, f.wnd.rtx[f.wnd.w.seq]);
}

TEST(send_wnd, step_wraps_around_when_window_wraps)
{
    Fixture f;
    f.wnd.w.seq = f.wnd.w.cap - 2;
    f.wnd.w.size = 4;
    f.wnd.rtx[f.wnd.w.seq] = 100;
    f.wnd.rtx[f.wnd.w.seq + 1] = 90;
    f.wnd.rtx[0] = 80;
    f.wnd.rtx[1] = 70;
    arq__send_wnd_step(&f.wnd, 10);
    CHECK_EQUAL(90, f.wnd.rtx[f.wnd.w.seq]);
    CHECK_EQUAL(80, f.wnd.rtx[f.wnd.w.seq + 1]);
    CHECK_EQUAL(70, f.wnd.rtx[0]);
    CHECK_EQUAL(60, f.wnd.rtx[1]);
}

struct SegFixture : Fixture
{
    int n = 0;
    void const *p = nullptr;
};

TEST(send_wnd, seg_with_base_idx_zero_returns_buf)
{
    SegFixture f;
    arq__send_wnd_seg(&f.wnd, 0, 0, &f.p, &f.n);
    CHECK_EQUAL((void const *)f.buf.data(), f.p);
}

TEST(send_wnd, seg_message_less_than_one_segment_returns_message_size)
{
    SegFixture f;
    f.wnd.w.msg_len = f.wnd.w.msg[0].len = 13;
    arq__send_wnd_seg(&f.wnd, 0, 0, &f.p, &f.n);
    CHECK_EQUAL(f.wnd.w.msg[0].len, f.n);
}

TEST(send_wnd, seg_message_greater_than_one_segment_returns_seg_len_for_non_final_segment_index)
{
    SegFixture f;
    f.wnd.w.msg[0].len = f.wnd.w.msg_len;
    arq__send_wnd_seg(&f.wnd, 0, 0, &f.p, &f.n);
    CHECK_EQUAL(f.wnd.w.seg_len, f.n);
}

TEST(send_wnd, seg_points_at_segment_len_times_segment_index)
{
    SegFixture f;
    f.wnd.w.msg[0].len = f.wnd.w.msg_len;
    arq__send_wnd_seg(&f.wnd, 0, 3, &f.p, &f.n);
    CHECK_EQUAL((void const *)&f.buf[3 * f.wnd.w.seg_len], f.p);
}

TEST(send_wnd, seg_returns_msg_len_remainder_for_final_segment)
{
    SegFixture f;
    f.wnd.w.msg[0].len = f.wnd.w.msg_len - (f.wnd.w.seg_len / 2);
    arq__send_wnd_seg(&f.wnd, 0, (f.wnd.w.msg_len / f.wnd.w.seg_len) - 1, &f.p, &f.n);
    CHECK_EQUAL(f.wnd.w.seg_len / 2, f.n);
}

TEST(send_wnd, seg_size_looks_up_message_by_message_index)
{
    SegFixture f;
    f.wnd.w.size = 2;
    f.wnd.w.msg[1].len = 5;
    arq__send_wnd_seg(&f.wnd, 1, 0, &f.p, &f.n);
    CHECK_EQUAL(5, f.n);
}

TEST(send_wnd, seg_buf_looks_up_message_by_message_index)
{
    SegFixture f;
    f.wnd.w.size = 2;
    f.wnd.w.msg[1].len = 5;
    arq__send_wnd_seg(&f.wnd, 1, 0, &f.p, &f.n);
    CHECK_EQUAL((void const *)&f.buf[f.wnd.w.msg_len], f.p);
}

TEST(send_wnd, seg_buf_pointer_is_message_offset_plus_segment_offset)
{
    SegFixture f;
    f.wnd.w.size = 3;
    f.wnd.w.msg[2].len = f.wnd.w.seg_len + 1;
    arq__send_wnd_seg(&f.wnd, 2, 1, &f.p, &f.n);
    CHECK_EQUAL((void const *)&f.buf[(f.wnd.w.msg_len * 2) + f.wnd.w.seg_len], f.p);
}

}

