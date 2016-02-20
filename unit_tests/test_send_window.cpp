#include "nanoarq_in_test_project.h"
#include <CppUTest/TestHarness.h>

#include <cstring>
#include <vector>
#include <array>

TEST_GROUP(window) {};

namespace
{

struct Fixture
{
    Fixture()
    {
        wnd.msg = msg.data();
        wnd.wnd_size_in_msgs = msg.size();
        wnd.msg_len = 128;
        wnd.base_msg_seq = 0;
        wnd.base_msg_idx = 0;
        wnd.cur_msg_idx = 0;
        wnd.full_ack_vec = 0xFFFF;

        buf.resize((size_t)(wnd.msg_len * wnd.wnd_size_in_msgs));
        wnd.buf = buf.data();

        for (auto &m : msg) {
            m.len = 0;
            m.cur_ack_vec = 0;
            m.full_ack_vec = wnd.full_ack_vec;
        }
    }

    arq__send_wnd_t wnd;
    std::array< arq__msg_t, 64 > msg;
    std::vector< unsigned char > buf;
    std::vector< unsigned char > snd;
};

TEST(window, rst_resets_window_to_zero)
{
    Fixture f;
    f.wnd.base_msg_idx = 1234;
    f.wnd.base_msg_seq = 4321;
    f.wnd.cur_msg_idx = 13;
    arq__send_wnd_rst(&f.wnd);
    CHECK_EQUAL(0, f.wnd.base_msg_idx);
    CHECK_EQUAL(0, f.wnd.base_msg_seq);
    CHECK_EQUAL(0, f.wnd.cur_msg_idx);
}

TEST(window, rst_resets_messages_to_default)
{
    Fixture f;
    for (auto &m : f.msg) {
        m.len = 1234;
        m.cur_ack_vec = 100;
        m.full_ack_vec = 123;
    }
    f.wnd.full_ack_vec = 0xABCD;
    arq__send_wnd_rst(&f.wnd);
    for (auto const &m : f.msg) {
        CHECK_EQUAL(0, m.len);
        CHECK_EQUAL(0, m.cur_ack_vec);
        CHECK_EQUAL(f.wnd.full_ack_vec, m.full_ack_vec);
    }
}

TEST(window, send_with_empty_window_returns_full_size_written)
{
    Fixture f;
    f.snd.resize(5);
    int const written = arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL((int)f.snd.size(), written);
}

TEST(window, small_send_with_empty_message_sets_message_size)
{
    Fixture f;
    f.snd.resize(5);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL((int)f.snd.size(), f.msg[0].len);
}

TEST(window, send_with_empty_window_copies_to_start_of_buf)
{
    Fixture f;
    f.snd.resize(5);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    MEMCMP_EQUAL(f.snd.data(), f.buf.data(), f.snd.size());
}

TEST(window, send_with_partially_full_window_appends)
{
    Fixture f;
    f.snd.resize(10);
    for (size_t i = 0; i < f.snd.size(); ++i) {
        f.snd[i] = (unsigned char)i;
    }
    std::fill(std::begin(f.buf), std::end(f.buf), 0xFF);
    f.wnd.msg[0].len = 3;
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    MEMCMP_EQUAL(f.snd.data(), &f.buf[3], f.snd.size());
}

TEST(window, send_with_partially_full_window_updates_message_length)
{
    Fixture f;
    f.snd.resize(15);
    f.wnd.msg[0].len = 12;
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(12 + 15, f.msg[0].len);
}

TEST(window, send_more_than_one_message_updates_sizes_in_first_two_messages)
{
    Fixture f;
    f.snd.resize((size_t)f.wnd.msg_len + 7);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(f.wnd.msg_len, f.msg[0].len);
    CHECK_EQUAL((int)f.snd.size() % f.wnd.msg_len, f.msg[1].len);
    CHECK_EQUAL(0, f.msg[2].len);
}

TEST(window, send_more_than_two_messages_updates_sizes_in_first_three_messages)
{
    Fixture f;
    f.snd.resize((size_t)f.wnd.msg_len * 2 + 7);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(f.wnd.msg_len, f.msg[0].len);
    CHECK_EQUAL(f.wnd.msg_len, f.msg[1].len);
    CHECK_EQUAL((int)f.snd.size() % f.wnd.msg_len, f.msg[2].len);
    CHECK_EQUAL(0, f.msg[3].len);
}

TEST(window, filling_a_message_increments_cur_index)
{
    Fixture f;
    f.snd.resize((size_t)f.wnd.msg_len);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(1, f.wnd.cur_msg_idx);
}

TEST(window, send_when_cur_index_greater_than_zero_updates_correct_message_len)
{
    Fixture f;
    f.wnd.cur_msg_idx = 1;
    f.msg[(size_t)f.wnd.cur_msg_idx].len = 3;
    f.snd.resize(15);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(3 + 15, f.msg[(size_t)f.wnd.cur_msg_idx].len);
}

TEST(window, send_when_cur_index_greater_than_zero_more_than_one_message_updates_sizes)
{
    Fixture f;
    f.wnd.cur_msg_idx = 2;
    f.snd.resize((size_t)f.wnd.msg_len + 7);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(f.wnd.msg_len, f.msg[2].len);
    CHECK_EQUAL((int)f.snd.size() % f.wnd.msg_len, f.msg[3].len);
    CHECK_EQUAL(0, f.msg[4].len);
}

TEST(window, cur_index_wraps_if_send_wraps_around)
{
    Fixture f;
    f.wnd.cur_msg_idx = f.wnd.wnd_size_in_msgs - 1;
    f.snd.resize((size_t)f.wnd.msg_len);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(0, f.wnd.cur_msg_idx);
}

TEST(window, send_updates_messages_at_beginning_of_msg_array_when_copy_wraps_around)
{
    Fixture f;
    f.wnd.cur_msg_idx = f.wnd.wnd_size_in_msgs - 1;
    f.wnd.base_msg_idx = f.wnd.cur_msg_idx;
    f.snd.resize((size_t)f.wnd.msg_len * 3);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(f.wnd.msg_len, f.msg[(size_t)f.wnd.wnd_size_in_msgs - 1].len);
    CHECK_EQUAL(f.wnd.msg_len, f.msg[0].len);
    CHECK_EQUAL(f.wnd.msg_len, f.msg[1].len);
    CHECK_EQUAL(0, f.msg[2].len);
}

TEST(window, send_more_data_than_window_space_returns_bytes_sent)
{
    Fixture f;
    f.snd.resize((size_t)(f.wnd.msg_len * f.wnd.wnd_size_in_msgs + 1));
    int const written = arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(f.wnd.msg_len * f.wnd.wnd_size_in_msgs, written);
}

TEST(window, send_copies_data_to_current_message_space_in_buf)
{
    Fixture f;
    f.wnd.cur_msg_idx = 1;
    f.snd.resize(19);
    for (size_t i = 0; i < f.snd.size(); ++i) {
        f.snd[i] = (unsigned char)i;
    }
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    MEMCMP_EQUAL(f.snd.data(), &f.buf[(size_t)f.wnd.msg_len], f.snd.size());
}

TEST(window, send_wraps_copy_around_if_inside_window_at_end_of_buf)
{
    Fixture f;
    int const orig_msg_idx = f.wnd.wnd_size_in_msgs - 1;
    f.wnd.cur_msg_idx = orig_msg_idx;
    f.wnd.base_msg_idx = orig_msg_idx;
    f.snd.resize((size_t)f.wnd.msg_len * 2);
    for (size_t i = 0; i < f.snd.size(); ++i) {
        f.snd[i] = (unsigned char)i;
    }
    std::fill(std::begin(f.buf), std::end(f.buf), 0xFE);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    MEMCMP_EQUAL(f.snd.data(), &f.buf[(size_t)(orig_msg_idx * f.wnd.msg_len)], (size_t)f.wnd.msg_len);
    MEMCMP_EQUAL(&f.snd[(size_t)f.wnd.msg_len], f.buf.data(), (size_t)f.wnd.msg_len);
}

TEST(window, send_wraps_copy_around_and_respects_partially_filled_starting_message)
{
    Fixture f;
    int const orig_msg_idx = f.wnd.wnd_size_in_msgs - 1;
    f.msg[(size_t)orig_msg_idx].len = 3;
    f.wnd.base_msg_idx = orig_msg_idx;
    f.wnd.cur_msg_idx = orig_msg_idx;
    f.snd.resize((size_t)f.wnd.msg_len + 8);
    for (size_t i = 0; i < f.snd.size(); ++i) {
        f.snd[i] = (unsigned char)i;
    }
    std::fill(std::begin(f.buf), std::end(f.buf), 0xFE);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    MEMCMP_EQUAL(f.snd.data(), &f.buf[(size_t)(orig_msg_idx * f.wnd.msg_len) + 3], (size_t)f.wnd.msg_len - 3);
    MEMCMP_EQUAL(&f.snd[(size_t)f.wnd.msg_len - 3], f.buf.data(), 11);
}

TEST(window, ack_partial_ack_vec_doesnt_slide_window)
{
    Fixture f;
    CHECK_EQUAL(0, f.wnd.base_msg_idx);
    arq__send_wnd_ack(&f.wnd, f.wnd.base_msg_seq, f.wnd.full_ack_vec - 1);
    CHECK_EQUAL(0, f.wnd.base_msg_idx);
}

TEST(window, ack_first_seq_slides_and_increments_base_idx)
{
    Fixture f;
    CHECK_EQUAL(0, f.wnd.base_msg_idx);
    arq__send_wnd_ack(&f.wnd, f.wnd.base_msg_seq, f.wnd.full_ack_vec);
    CHECK_EQUAL(1, f.wnd.base_msg_idx);
}

TEST(window, ack_first_seq_wraps_base_idx_when_base_idx_is_last)
{
    Fixture f;
    f.wnd.base_msg_idx = f.wnd.wnd_size_in_msgs - 1;
    arq__send_wnd_ack(&f.wnd, f.wnd.base_msg_seq, f.wnd.full_ack_vec);
    CHECK_EQUAL(0, f.wnd.base_msg_idx);
}

TEST(window, ack_sliding_increments_base_seq)
{
    Fixture f;
    CHECK_EQUAL(0, f.wnd.base_msg_seq);
    arq__send_wnd_ack(&f.wnd, f.wnd.base_msg_seq, f.wnd.full_ack_vec);
    CHECK_EQUAL(1, f.wnd.base_msg_seq);
}

TEST(window, ack_sets_vector_and_len_to_zero_when_sliding)
{
    Fixture f;
    int const orig_idx = f.wnd.base_msg_idx;
    f.wnd.msg[orig_idx].cur_ack_vec = f.wnd.full_ack_vec;
    f.wnd.msg[orig_idx].len = 1234;
    arq__send_wnd_ack(&f.wnd, f.wnd.base_msg_seq, f.wnd.full_ack_vec);
    CHECK_EQUAL(0, f.wnd.msg[orig_idx].cur_ack_vec);
    CHECK_EQUAL(0, f.wnd.msg[orig_idx].len);
}

TEST(window, ack_second_seq_doesnt_slide)
{
    Fixture f;
    arq__send_wnd_ack(&f.wnd, f.wnd.base_msg_seq + 1, f.wnd.full_ack_vec);
    CHECK_EQUAL(0, f.wnd.base_msg_seq);
}

TEST(window, ack_max_seq_num_wraps_to_zero)
{
    Fixture f;
    f.wnd.base_msg_seq = ARQ_FRAME_MAX_SEQ_NUM;
    arq__send_wnd_ack(&f.wnd, f.wnd.base_msg_seq, f.wnd.full_ack_vec);
    CHECK_EQUAL(0, f.wnd.base_msg_seq);
}

TEST(window, ack_first_seq_copies_cur_ack_vec)
{
    Fixture f;
    arq__send_wnd_ack(&f.wnd, f.wnd.base_msg_seq, 1u);
    CHECK_EQUAL(1u, f.wnd.msg[0].cur_ack_vec);
}

TEST(window, ack_slides_to_first_unackd_message)
{
    Fixture f;
    f.wnd.msg[f.wnd.base_msg_idx + 1].cur_ack_vec = f.wnd.full_ack_vec;
    f.wnd.msg[f.wnd.base_msg_idx + 2].cur_ack_vec = f.wnd.full_ack_vec;
    f.wnd.msg[f.wnd.base_msg_idx + 3].cur_ack_vec = f.wnd.full_ack_vec;
    f.wnd.msg[f.wnd.base_msg_idx + 4].cur_ack_vec = f.wnd.full_ack_vec;
    arq__send_wnd_ack(&f.wnd, f.wnd.base_msg_seq, f.wnd.full_ack_vec);
    CHECK_EQUAL(5, f.wnd.base_msg_idx);
    CHECK_EQUAL(5, f.wnd.base_msg_seq);
}

}

