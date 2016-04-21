#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTestExt/MockSupport.h>
#include <CppUTest/TestHarness.h>
#include <cstring>
#include <vector>
#include <array>
#include <algorithm>

TEST_GROUP(send_wnd) {};

namespace {

struct NoResetFixture
{
    NoResetFixture()
    {
        sw.rtx = rtx.data();
        sw.w.msg = msg.data();
        arq__wnd_init(&sw.w, msg.size(), 128, 16);
        buf.resize(sw.w.msg_len * sw.w.cap);
        sw.w.buf = buf.data();
    }

    arq__send_wnd_t sw;
    std::array< arq__msg_t, 64 > msg;
    std::array< arq_time_t, 64 > rtx;
    std::vector< unsigned char > buf;
    std::vector< unsigned char > snd;
};

TEST(send_wnd, rst_resets_window)
{
    NoResetFixture f;
    f.sw.w.size = 13;
    f.sw.w.seq = 4321;
    arq__send_wnd_rst(&f.sw);
    CHECK_EQUAL(0, f.sw.w.seq);
    CHECK_EQUAL(0, f.sw.w.size);
}

TEST(send_wnd, rst_resets_tinygram)
{
    NoResetFixture f;
    f.sw.tiny = 123u;
    f.sw.tiny_on = 0xAA;
    arq__send_wnd_rst(&f.sw);
    CHECK_EQUAL(0, f.sw.tiny);
    CHECK_EQUAL(0, f.sw.tiny_on);
}

TEST(send_wnd, rst_resets_messages_to_default)
{
    NoResetFixture f;
    for (auto &m : f.msg) {
        m.len = 1234;
        m.cur_ack_vec = 100;
        m.full_ack_vec = 123;
    }
    f.sw.w.full_ack_vec = 0xABCD;
    arq__send_wnd_rst(&f.sw);
    for (auto const &m : f.msg) {
        CHECK_EQUAL(0, m.len);
        CHECK_EQUAL(0, m.cur_ack_vec);
        CHECK_EQUAL(f.sw.w.full_ack_vec, m.full_ack_vec);
    }
}

TEST(send_wnd, rst_resets_rtx_to_default)
{
    NoResetFixture f;
    for (auto &t : f.rtx) {
        t = 1234;
    }
    arq__send_wnd_rst(&f.sw);
    for (auto const &t : f.rtx) {
        CHECK_EQUAL(0, t);
    }
}

struct Fixture : NoResetFixture
{
    Fixture()
    {
        arq__send_wnd_rst(&sw);
    }
};

TEST(send_wnd, send_size_increases_if_payload_uses_new_message)
{
    Fixture f;
    f.snd.resize(1);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(1, f.sw.w.size);
}

TEST(send_wnd, send_with_empty_window_returns_full_size_written)
{
    Fixture f;
    f.snd.resize(5);
    int const written = arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(5, written);
}

TEST(send_wnd, send_small_with_empty_message_sets_message_size)
{
    Fixture f;
    f.snd.resize(5);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(5, f.msg[0].len);
}

TEST(send_wnd, send_with_empty_window_copies_to_start_of_buf)
{
    Fixture f;
    f.snd.resize(5);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
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
    f.sw.w.msg[0].len = 3;
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    MEMCMP_EQUAL(f.snd.data(), &f.buf[3], f.snd.size());
}

TEST(send_wnd, send_with_partially_full_window_updates_message_length)
{
    Fixture f;
    f.sw.w.size = 1;
    f.sw.w.msg[0].len = 12;
    f.snd.resize(15);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(12 + 15, f.msg[0].len);
}

TEST(send_wnd, send_more_than_one_message_updates_lengths_in_first_two_messages)
{
    Fixture f;
    f.snd.resize(f.sw.w.msg_len + 7);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(f.sw.w.msg_len, f.msg[0].len);
    CHECK_EQUAL((int)f.snd.size() % f.sw.w.msg_len, f.msg[1].len);
    CHECK_EQUAL(0, f.msg[2].len);
}

TEST(send_wnd, send_one_full_message_sets_rtx_to_zero)
{
    Fixture f;
    f.snd.resize(f.sw.w.msg_len);
    f.sw.w.size = 1;
    f.sw.rtx[0] = 1;
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(0, f.sw.rtx[0]);
}

TEST(send_wnd, send_two_messages_sets_rtx_to_zero_for_full_message)
{
    Fixture f;
    f.snd.resize(f.sw.w.msg_len + 1);
    f.sw.w.size = 1;
    f.sw.rtx[0] = 1;
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(0, f.sw.rtx[0]);
}

TEST(send_wnd, send_more_than_two_messages_updates_lengths_in_first_three_messages)
{
    Fixture f;
    f.snd.resize(f.sw.w.msg_len * 2 + 7);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(f.sw.w.msg_len, f.msg[0].len);
    CHECK_EQUAL(f.sw.w.msg_len, f.msg[1].len);
    CHECK_EQUAL((int)f.snd.size() % f.sw.w.msg_len, f.msg[2].len);
    CHECK_EQUAL(0, f.msg[3].len);
}

TEST(send_wnd, send_three_messages_sets_rtx_to_zero_for_first_two_messages)
{
    Fixture f;
    f.snd.resize(f.sw.w.msg_len * 2 + 1);
    f.sw.rtx[0] = f.sw.rtx[1] = 1;
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(0, f.sw.rtx[0]);
    CHECK_EQUAL(0, f.sw.rtx[1]);
}

TEST(send_wnd, send_more_than_one_messages_increments_size_by_number_of_messages)
{
    Fixture f;
    f.snd.resize(f.sw.w.msg_len * 4);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(4, f.sw.w.size);
}

TEST(send_wnd, send_perfectly_completing_a_message_doesnt_increment_size)
{
    Fixture f;
    f.sw.w.size = 2;
    f.sw.w.msg[0].len = f.sw.w.msg_len;
    f.sw.w.msg[1].len = f.sw.w.msg_len - 12;
    f.snd.resize(12);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(2, f.sw.w.size);
}

TEST(send_wnd, send_when_sequence_number_greater_than_zero_updates_correct_message_len)
{
    Fixture f;
    f.sw.w.size = 2;
    f.sw.w.seq = 1;
    f.msg[2].len = 3;
    f.snd.resize(15);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(3 + 15, f.msg[2].len);
}

TEST(send_wnd, send_when_sequence_number_greater_than_zero_more_than_one_message_updates_sizes)
{
    Fixture f;
    f.sw.w.seq = 2;
    f.snd.resize(f.sw.w.msg_len + 1);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(f.sw.w.msg_len, f.msg[2].len);
    CHECK_EQUAL(1, f.msg[3].len);
    CHECK_EQUAL(0, f.msg[4].len);
}

TEST(send_wnd, send_updates_messages_at_beginning_of_msg_array_when_copy_wraps_around)
{
    Fixture f;
    f.sw.w.seq = f.sw.w.cap - 1;
    f.snd.resize(f.sw.w.msg_len * 3);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(f.sw.w.msg_len, f.msg[f.sw.w.cap - 1].len);
    CHECK_EQUAL(f.sw.w.msg_len, f.msg[0].len);
    CHECK_EQUAL(f.sw.w.msg_len, f.msg[1].len);
    CHECK_EQUAL(0, f.msg[2].len);
}

TEST(send_wnd, send_more_data_than_window_space_returns_bytes_sent)
{
    Fixture f;
    f.snd.resize(((f.sw.w.msg_len * f.sw.w.cap) + 1));
    int const written = arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(f.sw.w.msg_len * f.sw.w.cap, written);
}

TEST(send_wnd, send_to_full_window_returns_zero)
{
    Fixture f;
    f.sw.w.size = f.sw.w.cap;
    std::for_each(std::begin(f.msg), std::end(f.msg), [&](arq__msg_t& m) { m.len = f.sw.w.msg_len; });
    f.snd.resize(16);
    int const written = arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(0, written);
}

TEST(send_wnd, send_more_data_than_window_space_with_partial_msg_returns_rest_of_window)
{
    Fixture f;
    f.sw.w.cap = 3;
    f.sw.w.size = 3;
    f.sw.w.msg[0].len = f.sw.w.msg_len;
    f.sw.w.msg[1].len = f.sw.w.msg_len;
    f.sw.w.msg[2].len = 27;
    int const cap_in_bytes = f.sw.w.cap * f.sw.w.msg_len;
    int const size_in_bytes = f.sw.w.msg[0].len + f.sw.w.msg[1].len + f.sw.w.msg[2].len;
    f.snd.resize(cap_in_bytes);
    int const written = arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(cap_in_bytes - size_in_bytes, written);
}

TEST(send_wnd, send_copies_data_to_current_message_space_in_buf)
{
    Fixture f;
    f.sw.w.seq = 1;
    f.snd.resize(19);
    for (size_t i = 0; i < f.snd.size(); ++i) {
        f.snd[i] = (unsigned char)i;
    }
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    MEMCMP_EQUAL(f.snd.data(), &f.buf[f.sw.w.msg_len], f.snd.size());
}

TEST(send_wnd, send_wraps_copy_around_if_inside_window_at_end_of_buf)
{
    Fixture f;
    int const orig_msg_idx = f.sw.w.cap - 1;
    f.sw.w.seq = orig_msg_idx;
    f.snd.resize(f.sw.w.msg_len * 2);
    for (size_t i = 0; i < f.snd.size(); ++i) {
        f.snd[i] = (unsigned char)i;
    }
    std::fill(std::begin(f.buf), std::end(f.buf), 0xFE);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    MEMCMP_EQUAL(f.snd.data(), &f.buf[(orig_msg_idx * f.sw.w.msg_len)], f.sw.w.msg_len);
    MEMCMP_EQUAL(&f.snd[f.sw.w.msg_len], f.buf.data(), f.sw.w.msg_len);
}

TEST(send_wnd, send_wraps_copy_around_and_respects_partially_filled_starting_message)
{
    Fixture f;
    f.snd.resize(f.sw.w.msg_len);
    for (size_t i = 0; i < f.snd.size(); ++i) {
        f.snd[i] = (unsigned char)i;
    }
    std::fill(std::begin(f.buf), std::end(f.buf), 0xFE);
    int const orig_msg_idx = f.sw.w.cap - 1;
    f.sw.w.seq = orig_msg_idx;
    f.msg[orig_msg_idx].len = 3;
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    MEMCMP_EQUAL(f.snd.data(), &f.buf[(orig_msg_idx * f.sw.w.msg_len) + 3], f.sw.w.msg_len - 3);
    MEMCMP_EQUAL(&f.snd[f.sw.w.msg_len - 3], f.buf.data(), 3);
}

TEST(send_wnd, send_payload_is_message_size_wrap_window)
{
    // this edge case teased out a bug in arq__send_wnd_send
    Fixture f;
    f.sw.w.cap = 4;
    f.sw.w.size = 3;
    f.sw.w.seq = 1;
    f.sw.w.msg_len = 32;
    f.sw.w.full_ack_vec = 0b11111;
    f.sw.w.msg[1].len = 32;
    f.sw.w.msg[2].len = 32;
    f.sw.w.msg[3].len = 32;
    f.snd.resize(32);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(32, f.sw.w.msg[0].len);
}

TEST(send_wnd, send_partial_message_turns_tinygram_timer_on)
{
    Fixture f;
    f.snd.resize(1);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(1, f.sw.tiny_on);
}

TEST(send_wnd, send_partial_message_sets_tinygram_timer_to_default)
{
    Fixture f;
    f.snd.resize(1);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 123);
    CHECK_EQUAL(123, f.sw.tiny);
}

TEST(send_wnd, send_partial_messages_sets_message_rtx_to_tinygram_timeout)
{
    Fixture f;
    f.snd.resize(1);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 79);
    CHECK_EQUAL(79, f.sw.rtx[0]);
}

TEST(send_wnd, send_partial_messages_nonzero_sets_message_rtx_to_tinygram_timeout)
{
    Fixture f;
    f.sw.w.cap = 8;
    f.sw.w.size = 6;
    for (auto i = 0u; i < f.sw.w.size; ++i) {
        f.sw.w.msg[i].len = f.sw.w.msg_len;
    }
    f.snd.resize(1);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 79);
    CHECK_EQUAL(79, f.sw.rtx[6]);
}

TEST(send_wnd, send_accumulating_incomplete_message_doesnt_reset_tinygram_timer)
{
    Fixture f;
    f.sw.tiny_on = 1;
    f.sw.tiny = 123;
    f.sw.w.size = 1;
    f.sw.w.msg[0].len = 1;
    f.snd.resize(1);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(123, f.sw.tiny);
    CHECK_EQUAL(1, f.sw.tiny_on);
}

TEST(send_wnd, send_accumulating_complete_message_turns_tinygram_timer_off)
{
    Fixture f;
    f.sw.tiny_on = 1;
    f.sw.tiny = 123;
    f.sw.w.size = 1;
    f.sw.w.msg[0].len = 1;
    f.snd.resize(f.sw.w.msg_len - 1);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(0, f.sw.tiny_on);
}

TEST(send_wnd, send_full_message_leaves_tinygram_timer_off)
{
    Fixture f;
    f.snd.resize(f.sw.w.msg_len);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(0, f.sw.tiny_on);
}

TEST(send_wnd, send_multiple_messages_in_one_call_leaves_tinygram_timer_off)
{
    Fixture f;
    f.snd.resize(f.sw.w.msg_len * 3);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 1);
    CHECK_EQUAL(0, f.sw.tiny_on);
}

TEST(send_wnd, send_multiple_calls_accumulating_inside_one_message_doesnt_reset_tinygram_timer)
{
    Fixture f;
    f.snd.resize(1);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 13);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 13);
    arq__send_wnd_send(&f.sw, f.snd.data(), f.snd.size(), 13);
    CHECK_EQUAL(1, f.sw.tiny_on);
    CHECK_EQUAL(13, f.sw.tiny);
    CHECK_EQUAL(13, f.sw.rtx[0]);
}

TEST(send_wnd, ack_seq_outside_of_send_wnd_greater_does_nothing)
{
    Fixture f;
    f.sw.w.size = 1;
    f.sw.w.cap = 2;
    f.sw.w.seq = 0;
    arq__send_wnd_ack(&f.sw, 1, 1);
    CHECK_EQUAL(0, f.msg[1].cur_ack_vec);
    CHECK_EQUAL(0, f.sw.w.seq);
}

TEST(send_wnd, ack_seq_outside_of_send_wnd_less_does_nothing)
{
    Fixture f;
    f.sw.w.size = 4;
    f.sw.w.seq = 16;
    arq__send_wnd_ack(&f.sw, 15, 1);
    CHECK_EQUAL(0, f.msg[15].cur_ack_vec);
    CHECK_EQUAL(16, f.sw.w.seq);
}

TEST(send_wnd, ack_empty_window_does_nothing)
{
    Fixture f;
    f.sw.w.size = 0;
    f.sw.w.seq = 5;
    arq__send_wnd_ack(&f.sw, 5, 1);
    CHECK_EQUAL(0, f.msg[5].cur_ack_vec);
    CHECK_EQUAL(0, f.sw.w.size);
}

TEST(send_wnd, ack_partial_ack_vec_doesnt_slide_window)
{
    Fixture f;
    arq__send_wnd_ack(&f.sw, f.sw.w.seq, f.sw.w.full_ack_vec - 1);
    CHECK_EQUAL(0, f.sw.w.seq);
}

TEST(send_wnd, ack_first_seq_slides_and_increments_base_seq)
{
    Fixture f;
    f.sw.w.size = 1;
    arq__send_wnd_ack(&f.sw, f.sw.w.seq, f.sw.w.full_ack_vec);
    CHECK_EQUAL(1, f.sw.w.seq);
}

TEST(send_wnd, ack_full_msg_decrements_size)
{
    Fixture f;
    f.sw.w.size = 1;
    arq__send_wnd_ack(&f.sw, f.sw.w.seq, f.sw.w.full_ack_vec);
    CHECK_EQUAL(0, f.sw.w.size);
}

TEST(send_wnd, ack_sliding_increments_base_seq)
{
    Fixture f;
    f.sw.w.size = 1;
    arq__send_wnd_ack(&f.sw, f.sw.w.seq, f.sw.w.full_ack_vec);
    CHECK_EQUAL(1, f.sw.w.seq);
}

TEST(send_wnd, ack_resets_len_and_cur_ack_vec_when_sliding)
{
    Fixture f;
    f.sw.w.size = 1;
    f.sw.w.msg[0].cur_ack_vec = f.sw.w.full_ack_vec;
    f.sw.w.msg[0].len = 1234;
    arq__send_wnd_ack(&f.sw, 0, f.sw.w.full_ack_vec);
    CHECK_EQUAL(0, f.sw.w.msg[0].cur_ack_vec);
    CHECK_EQUAL(0, f.sw.w.msg[0].len);
}

TEST(send_wnd, ack_resets_full_ack_vec_when_sliding)
{
    Fixture f;
    f.sw.w.size = 1;
    f.sw.w.msg[0].cur_ack_vec = f.sw.w.msg[0].full_ack_vec = 0b1111;
    arq__send_wnd_ack(&f.sw, 0, 0b1111);
    CHECK_EQUAL(f.sw.w.full_ack_vec, f.sw.w.msg[0].full_ack_vec);
}

TEST(send_wnd, ack_second_seq_doesnt_slide)
{
    Fixture f;
    f.sw.w.size = 2;
    arq__send_wnd_ack(&f.sw, f.sw.w.seq + 1, f.sw.w.full_ack_vec);
    CHECK_EQUAL(0, f.sw.w.seq);
}

TEST(send_wnd, ack_max_seq_num_wraps_to_zero)
{
    Fixture f;
    f.sw.w.size = 1;
    f.sw.w.seq = ARQ__FRAME_MAX_SEQ_NUM;
    arq__send_wnd_ack(&f.sw, f.sw.w.seq, f.sw.w.full_ack_vec);
    CHECK_EQUAL(0, f.sw.w.seq);
}

TEST(send_wnd, ack_first_seq_copies_cur_ack_vec)
{
    Fixture f;
    f.sw.w.size = 1;
    arq__send_wnd_ack(&f.sw, f.sw.w.seq, 1u);
    CHECK_EQUAL(1u, f.sw.w.msg[0].cur_ack_vec);
}

TEST(send_wnd, ack_slides_to_first_unackd_message)
{
    Fixture f;
    f.sw.w.size = 5;
    f.sw.w.msg[0].cur_ack_vec = 0;
    f.sw.w.msg[1].cur_ack_vec = f.sw.w.full_ack_vec;
    f.sw.w.msg[2].cur_ack_vec = f.sw.w.full_ack_vec;
    f.sw.w.msg[3].cur_ack_vec = f.sw.w.full_ack_vec;
    f.sw.w.msg[4].cur_ack_vec = 0;
    arq__send_wnd_ack(&f.sw, 0, f.sw.w.full_ack_vec);
    CHECK_EQUAL(4, f.sw.w.seq);
}

TEST(send_wnd, ack_the_entire_send_window)
{
    Fixture f;
    f.sw.w.size = f.sw.w.cap + 1;
    for (auto &m : f.msg) {
        m.cur_ack_vec = m.full_ack_vec = f.sw.w.full_ack_vec;
        m.len = f.sw.w.msg_len;
    }
    --f.sw.w.msg[0].cur_ack_vec;
    arq__send_wnd_ack(&f.sw, 0, f.sw.w.full_ack_vec);
    CHECK_EQUAL(1, f.sw.w.size);
    CHECK_EQUAL(f.sw.w.cap, f.sw.w.seq);
}

TEST(send_wnd, ack_nak_sets_associated_rtx_to_zero)
{
    Fixture f;
    f.sw.w.size = 1;
    f.sw.w.msg[0].cur_ack_vec = 0;
    f.sw.rtx[0] = 100;
    arq__send_wnd_ack(&f.sw, 0, f.sw.w.full_ack_vec - 1);
    CHECK_EQUAL(0, f.sw.rtx[0]);
}

TEST(send_wnd, flush_does_nothing_on_empty_window)
{
    Fixture f;
    arq__send_wnd_flush(&f.sw);
    CHECK_EQUAL(0, f.sw.w.size);
}

TEST(send_wnd, flush_does_nothing_if_current_message_has_zero_length)
{
    Fixture f;
    f.sw.w.size = 2;
    arq__send_wnd_flush(&f.sw);
    CHECK_EQUAL(2, f.sw.w.size);
}

TEST(send_wnd, flush_does_not_increment_size_if_current_message_has_nonzero_length)
{
    Fixture f;
    f.sw.w.size = 1;
    f.msg[0].len = 12;
    arq__send_wnd_flush(&f.sw);
    CHECK_EQUAL(1, f.sw.w.size);
}

TEST(send_wnd, flush_does_nothing_on_full_window)
{
    Fixture f;
    f.sw.w.size = f.sw.w.cap + 1;
    for (auto &m : f.msg) {
        m.len = f.sw.w.msg_len;
    }
    arq__send_wnd_flush(&f.sw);
    CHECK_EQUAL(f.sw.w.cap + 1, f.sw.w.size);
}

TEST(send_wnd, flush_sets_full_ack_vector_to_one_if_less_than_one_segment)
{
    Fixture f;
    f.sw.w.size = 1;
    f.msg[0].len = 1;
    arq__send_wnd_flush(&f.sw);
    CHECK_EQUAL(0b1, f.msg[0].full_ack_vec);
}

TEST(send_wnd, flush_number_of_bits_in_ack_vector_is_number_of_segments)
{
    Fixture f;
    f.sw.w.size = 1;
    f.msg[0].len = f.sw.w.seg_len * 5;
    arq__send_wnd_flush(&f.sw);
    CHECK_EQUAL(0b11111, f.msg[0].full_ack_vec);
}

TEST(send_wnd, flush_makes_msg_full_ack_vector_from_current_message_size)
{
    Fixture f;
    f.sw.w.size = 1;
    f.msg[0].len = f.sw.w.seg_len * 2 + 1;
    arq__send_wnd_flush(&f.sw);
    CHECK_EQUAL(0b111, f.msg[0].full_ack_vec);
}

TEST(send_wnd, flush_acts_on_final_incomplete_message)
{
    Fixture f;
    f.sw.w.size = 3;
    f.msg[2].len = 1;
    arq__send_wnd_flush(&f.sw);
    CHECK_EQUAL(0b1, f.msg[2].full_ack_vec);
}

TEST(send_wnd, flush_sets_associated_rtx_to_zero)
{
    Fixture f;
    f.sw.w.size = 1;
    f.msg[0].len = 10;
    f.rtx[0] = 100;
    arq__send_wnd_flush(&f.sw);
    CHECK_EQUAL(0, f.rtx[0]);
}

TEST(send_wnd, step_decrements_first_timer_in_window_by_dt)
{
    Fixture f;
    f.sw.w.size = 1;
    f.sw.rtx[0] = 100;
    arq__send_wnd_step(&f.sw, 10);
    CHECK_EQUAL(90, f.sw.rtx[0]);
}

TEST(send_wnd, step_saturates_rtx_to_zero)
{
    Fixture f;
    f.sw.w.size = 1;
    f.sw.rtx[0] = 100;
    arq__send_wnd_step(&f.sw, f.sw.rtx[0] + 1);
    CHECK_EQUAL(0, f.sw.rtx[0]);
}

TEST(send_wnd, step_decrements_all_timers_in_window)
{
    Fixture f;
    f.sw.w.size = 3;
    f.sw.rtx[0] = 100;
    f.sw.rtx[1] = 90;
    f.sw.rtx[2] = 80;
    f.sw.rtx[3] = 1000;
    arq__send_wnd_step(&f.sw, 80);
    CHECK_EQUAL(20, f.sw.rtx[0]);
    CHECK_EQUAL(10, f.sw.rtx[1]);
    CHECK_EQUAL(0, f.sw.rtx[2]);
    CHECK_EQUAL(1000, f.sw.rtx[3]);
}

TEST(send_wnd, step_operates_on_messages_in_window)
{
    Fixture f;
    f.sw.w.size = 1;
    f.sw.w.seq = f.sw.w.cap / 2;
    f.sw.rtx[f.sw.w.seq] = 100;
    arq__send_wnd_step(&f.sw, 10);
    CHECK_EQUAL(90, f.sw.rtx[f.sw.w.seq]);
}

TEST(send_wnd, step_wraps_around_when_window_wraps)
{
    Fixture f;
    f.sw.w.seq = f.sw.w.cap - 2;
    f.sw.w.size = 4;
    f.sw.rtx[f.sw.w.seq] = 100;
    f.sw.rtx[f.sw.w.seq + 1] = 90;
    f.sw.rtx[0] = 80;
    f.sw.rtx[1] = 70;
    arq__send_wnd_step(&f.sw, 10);
    CHECK_EQUAL(90, f.sw.rtx[f.sw.w.seq]);
    CHECK_EQUAL(80, f.sw.rtx[f.sw.w.seq + 1]);
    CHECK_EQUAL(70, f.sw.rtx[0]);
    CHECK_EQUAL(60, f.sw.rtx[1]);
}

TEST(send_wnd, step_decrements_tinygram_timer_if_active)
{
    Fixture f;
    f.sw.tiny_on = 1;
    f.sw.tiny = 10;
    arq__send_wnd_step(&f.sw, 3);
    CHECK_EQUAL(7, f.sw.tiny);
}

TEST(send_wnd, step_ignores_tinygram_timer_if_inactive)
{
    Fixture f;
    f.sw.tiny_on = 0;
    f.sw.tiny = 13;
    arq__send_wnd_step(&f.sw, 1000);
    CHECK_EQUAL(13, f.sw.tiny);
}

TEST(send_wnd, step_saturates_tiny_to_zero)
{
    Fixture f;
    f.sw.tiny_on = 1;
    f.sw.tiny = 12;
    arq__send_wnd_step(&f.sw, 100);
    CHECK_EQUAL(0, f.sw.tiny);
}

}

