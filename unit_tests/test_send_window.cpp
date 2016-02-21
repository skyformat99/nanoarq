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
        wnd.msg_len = 128;
        wnd.full_ack_vec = 0xFFFF;
        wnd.cap = msg.size();
        buf.resize((size_t)(wnd.msg_len * wnd.cap));
        wnd.buf = buf.data();
        wnd.msg = msg.data();
        arq__send_wnd_rst(&wnd);
    }

    arq__send_wnd_t wnd;
    std::array< arq__msg_t, 64 > msg;
    std::vector< unsigned char > buf;
    std::vector< unsigned char > snd;
};

TEST(window, rst_resets_window)
{
    Fixture f;
    f.wnd.size = 13;
    f.wnd.base_idx = 1234;
    f.wnd.base_seq = 4321;
    arq__send_wnd_rst(&f.wnd);
    CHECK_EQUAL(0, f.wnd.base_idx);
    CHECK_EQUAL(0, f.wnd.base_seq);
    CHECK_EQUAL(1, f.wnd.size);
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

TEST(window, send_size_remains_if_payload_doesnt_fill_message)
{
    Fixture f;
    f.snd.resize(5);
    CHECK_EQUAL(1, f.wnd.size);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(1, f.wnd.size);
}

TEST(window, send_with_empty_window_returns_full_size_written)
{
    Fixture f;
    f.snd.resize(5);
    int const written = arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL((int)f.snd.size(), written);
}

TEST(window, send_small_with_empty_message_sets_message_size)
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

TEST(window, send_more_than_one_messages_increments_size_by_number_of_messages)
{
    Fixture f;
    f.snd.resize((size_t)f.wnd.msg_len * 4);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(5, f.wnd.size);
}

TEST(window, send_perfectly_completing_a_message_increments_size)
{
    Fixture f;
    f.wnd.size = 2;
    f.wnd.msg[0].len = f.wnd.msg_len;
    f.wnd.msg[1].len = f.wnd.msg_len - 12;
    f.snd.resize(12);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(3, f.wnd.size);
}

TEST(window, send_when_cur_index_greater_than_zero_updates_correct_message_len)
{
    Fixture f;
    f.wnd.size = 1;
    f.wnd.base_idx = 1;
    f.msg[1].len = 3;
    f.snd.resize(15);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(3 + 15, f.msg[1].len);
}

TEST(window, send_when_cur_index_greater_than_zero_more_than_one_message_updates_sizes)
{
    Fixture f;
    f.wnd.base_idx = 2;
    f.snd.resize((size_t)f.wnd.msg_len + 1);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(f.wnd.msg_len, f.msg[2].len);
    CHECK_EQUAL(1, f.msg[3].len);
    CHECK_EQUAL(0, f.msg[4].len);
}

TEST(window, send_updates_messages_at_beginning_of_msg_array_when_copy_wraps_around)
{
    Fixture f;
    f.wnd.base_idx = f.wnd.cap - 1;
    f.snd.resize((size_t)f.wnd.msg_len * 3);
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(f.wnd.msg_len, f.msg[(size_t)f.wnd.cap - 1].len);
    CHECK_EQUAL(f.wnd.msg_len, f.msg[0].len);
    CHECK_EQUAL(f.wnd.msg_len, f.msg[1].len);
    CHECK_EQUAL(0, f.msg[2].len);
}

TEST(window, send_more_data_than_window_space_returns_bytes_sent)
{
    Fixture f;
    f.snd.resize((size_t)((f.wnd.msg_len * f.wnd.cap) + 1));
    int const written = arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(f.wnd.msg_len * f.wnd.cap, written);
}

TEST(window, send_more_data_than_window_space_with_partial_msg_returns_rest_of_window)
{
    Fixture f;
    f.wnd.size = 2;
    f.wnd.msg[0].len = f.wnd.msg_len;
    f.wnd.msg[1].len = 27;
    int const cap_in_bytes = f.wnd.cap * f.wnd.msg_len;
    int const size_in_bytes = f.wnd.msg[0].len + f.wnd.msg[1].len;
    f.snd.resize((size_t)cap_in_bytes);
    int const written = arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(cap_in_bytes - size_in_bytes, written);
}

TEST(window, send_copies_data_to_current_message_space_in_buf)
{
    Fixture f;
    f.wnd.base_idx = 1;
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
    int const orig_msg_idx = f.wnd.cap - 1;
    f.wnd.base_idx = orig_msg_idx;
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
    f.snd.resize((size_t)f.wnd.msg_len);
    for (size_t i = 0; i < f.snd.size(); ++i) {
        f.snd[i] = (unsigned char)i;
    }
    std::fill(std::begin(f.buf), std::end(f.buf), 0xFE);
    int const orig_msg_idx = f.wnd.cap - 1;
    f.wnd.base_idx = orig_msg_idx;
    f.msg[(size_t)orig_msg_idx].len = 3;
    arq__send_wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    MEMCMP_EQUAL(f.snd.data(), &f.buf[(size_t)(orig_msg_idx * f.wnd.msg_len) + 3], (size_t)f.wnd.msg_len - 3);
    MEMCMP_EQUAL(&f.snd[(size_t)f.wnd.msg_len - 3], f.buf.data(), 3);
}

TEST(window, ack_partial_ack_vec_doesnt_slide_window)
{
    Fixture f;
    arq__send_wnd_ack(&f.wnd, f.wnd.base_seq, f.wnd.full_ack_vec - 1);
    CHECK_EQUAL(0, f.wnd.base_idx);
}

TEST(window, ack_first_seq_slides_and_increments_base_idx)
{
    Fixture f;
    f.wnd.size = 1;
    arq__send_wnd_ack(&f.wnd, f.wnd.base_seq, f.wnd.full_ack_vec);
    CHECK_EQUAL(1, f.wnd.base_idx);
}

TEST(window, ack_full_msg_decrements_size)
{
    Fixture f;
    f.wnd.size = 1;
    arq__send_wnd_ack(&f.wnd, f.wnd.base_seq, f.wnd.full_ack_vec);
    CHECK_EQUAL(0, f.wnd.size);
}

TEST(window, ack_first_seq_wraps_base_idx_when_base_idx_is_last)
{
    Fixture f;
    f.wnd.size = 1;
    f.wnd.base_idx = f.wnd.cap - 1;
    arq__send_wnd_ack(&f.wnd, f.wnd.base_seq, f.wnd.full_ack_vec);
    CHECK_EQUAL(0, f.wnd.base_idx);
}

TEST(window, ack_sliding_increments_base_seq)
{
    Fixture f;
    f.wnd.size = 1;
    arq__send_wnd_ack(&f.wnd, f.wnd.base_seq, f.wnd.full_ack_vec);
    CHECK_EQUAL(1, f.wnd.base_seq);
}

TEST(window, ack_sets_ack_vector_and_len_to_zero_when_sliding)
{
    Fixture f;
    f.wnd.size = 1;
    f.wnd.msg[0].cur_ack_vec = f.wnd.full_ack_vec;
    f.wnd.msg[0].len = 1234;
    arq__send_wnd_ack(&f.wnd, 0, f.wnd.full_ack_vec);
    CHECK_EQUAL(0, f.wnd.msg[0].cur_ack_vec);
    CHECK_EQUAL(0, f.wnd.msg[0].len);
}

TEST(window, ack_second_seq_doesnt_slide)
{
    Fixture f;
    f.wnd.size = 2;
    arq__send_wnd_ack(&f.wnd, f.wnd.base_seq + 1, f.wnd.full_ack_vec);
    CHECK_EQUAL(0, f.wnd.base_seq);
}

TEST(window, ack_max_seq_num_wraps_to_zero)
{
    Fixture f;
    f.wnd.size = 1;
    f.wnd.base_seq = ARQ_FRAME_MAX_SEQ_NUM;
    arq__send_wnd_ack(&f.wnd, f.wnd.base_seq, f.wnd.full_ack_vec);
    CHECK_EQUAL(0, f.wnd.base_seq);
}

TEST(window, ack_first_seq_copies_cur_ack_vec)
{
    Fixture f;
    f.wnd.size = 1;
    arq__send_wnd_ack(&f.wnd, f.wnd.base_seq, 1u);
    CHECK_EQUAL(1u, f.wnd.msg[0].cur_ack_vec);
}

TEST(window, ack_slides_to_first_unackd_message)
{
    Fixture f;
    f.wnd.size = 5;
    f.wnd.msg[0].cur_ack_vec = 0;
    f.wnd.msg[1].cur_ack_vec = f.wnd.full_ack_vec;
    f.wnd.msg[2].cur_ack_vec = f.wnd.full_ack_vec;
    f.wnd.msg[3].cur_ack_vec = f.wnd.full_ack_vec;
    f.wnd.msg[4].cur_ack_vec = 0;
    arq__send_wnd_ack(&f.wnd, 0, f.wnd.full_ack_vec);
    CHECK_EQUAL(4, f.wnd.base_idx);
    CHECK_EQUAL(4, f.wnd.base_seq);
}

}

