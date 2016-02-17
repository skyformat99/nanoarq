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
        buf.resize(1024 * 32);
        wnd.buf = buf.data();
        wnd.msg = msg.data();
        wnd.size_in_msgs = msg.size();
        wnd.msg_len = 100;
        wnd.base_msg_seq = 0;
        wnd.base_msg_idx = 0;
        wnd.cur_msg_idx = 0;

        for (auto &m : msg) {
            m.len = 0;
            m.ack_seg_mask = 0;
        }
    }

    arq__send_wnd_t wnd;
    std::array< arq__msg_t, 64 > msg;
    std::vector< unsigned char > buf;
    std::vector< unsigned char > snd;
};

TEST(window, send_with_empty_window_returns_full_size_written)
{
    Fixture f;
    f.snd.resize(5);
    int const written = arq__wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL((int)f.snd.size(), written);
}

TEST(window, small_send_with_empty_message_sets_message_size)
{
    Fixture f;
    f.snd.resize(5);
    arq__wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL((int)f.snd.size(), f.msg[0].len);
}

TEST(window, send_with_empty_window_copies_to_start_of_buf)
{
    Fixture f;
    f.snd.resize(5);
    arq__wnd_send(&f.wnd, f.snd.data(), f.snd.size());
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
    arq__wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    MEMCMP_EQUAL(f.snd.data(), &f.buf[3], f.snd.size());
}

TEST(window, send_with_partially_full_window_updates_message_length)
{
    Fixture f;
    f.snd.resize(15);
    f.wnd.msg[0].len = 12;
    arq__wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(12 + 15, f.msg[0].len);
}

TEST(window, send_more_than_one_message_updates_sizes_in_first_two_messages)
{
    Fixture f;
    f.snd.resize((size_t)f.wnd.msg_len + 7);
    arq__wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(f.wnd.msg_len, f.msg[0].len);
    CHECK_EQUAL((int)f.snd.size() % f.wnd.msg_len, f.msg[1].len);
    CHECK_EQUAL(0, f.msg[2].len);
}

TEST(window, send_more_than_two_messages_updates_sizes_in_first_three_messages)
{
    Fixture f;
    f.snd.resize((size_t)f.wnd.msg_len * 2 + 7);
    arq__wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(f.wnd.msg_len, f.msg[0].len);
    CHECK_EQUAL(f.wnd.msg_len, f.msg[1].len);
    CHECK_EQUAL((int)f.snd.size() % f.wnd.msg_len, f.msg[2].len);
    CHECK_EQUAL(0, f.msg[3].len);
}

TEST(window, filling_a_message_increments_cur_index)
{
    Fixture f;
    f.snd.resize((size_t)f.wnd.msg_len);
    arq__wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(1, f.wnd.cur_msg_idx);
}

TEST(window, send_when_cur_index_greater_than_zero_updates_correct_message_len)
{
    Fixture f;
    f.wnd.cur_msg_idx = 1;
    f.msg[(size_t)f.wnd.cur_msg_idx].len = 3;
    f.snd.resize(15);
    arq__wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(3 + 15, f.msg[(size_t)f.wnd.cur_msg_idx].len);
}

TEST(window, send_when_cur_index_greater_than_zero_more_than_one_message_updates_sizes)
{
    Fixture f;
    f.wnd.cur_msg_idx = 2;
    f.snd.resize((size_t)f.wnd.msg_len + 7);
    arq__wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(f.wnd.msg_len, f.msg[2].len);
    CHECK_EQUAL((int)f.snd.size() % f.wnd.msg_len, f.msg[3].len);
    CHECK_EQUAL(0, f.msg[4].len);
}

TEST(window, cur_index_wraps_if_send_wraps_around)
{
    Fixture f;
    f.wnd.cur_msg_idx = f.wnd.size_in_msgs - 1;
    f.snd.resize((size_t)f.wnd.msg_len);
    arq__wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(0, f.wnd.cur_msg_idx);
}

TEST(window, send_updates_messages_at_beginning_of_msg_array_when_copy_wraps_around)
{
    Fixture f;
    f.wnd.cur_msg_idx = f.wnd.size_in_msgs - 1;
    f.wnd.base_msg_idx = f.wnd.cur_msg_idx;
    f.snd.resize((size_t)f.wnd.msg_len * 3);
    arq__wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(f.wnd.msg_len, f.msg[(size_t)f.wnd.size_in_msgs - 1].len);
    CHECK_EQUAL(f.wnd.msg_len, f.msg[0].len);
    CHECK_EQUAL(f.wnd.msg_len, f.msg[1].len);
    CHECK_EQUAL(0, f.msg[2].len);
}

TEST(window, send_more_data_than_window_space_returns_bytes_sent)
{
    Fixture f;
    f.snd.resize((size_t)(f.wnd.msg_len * f.wnd.size_in_msgs + 1));
    int const written = arq__wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    CHECK_EQUAL(f.wnd.msg_len * f.wnd.size_in_msgs, written);
}

TEST(window, send_copies_data_to_current_message_space_in_buf)
{
    Fixture f;
    f.wnd.cur_msg_idx = 1;
    f.snd.resize(19);
    for (size_t i = 0; i < f.snd.size(); ++i) {
        f.snd[i] = (unsigned char)i;
    }
    arq__wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    MEMCMP_EQUAL(f.snd.data(), &f.buf[(size_t)f.wnd.msg_len], f.snd.size());
}

TEST(window, send_wraps_copy_around_if_inside_window_at_end_of_buf)
{
    Fixture f;
    int const orig_msg_idx = f.wnd.size_in_msgs - 1;
    f.wnd.cur_msg_idx = orig_msg_idx;
    f.wnd.base_msg_idx = orig_msg_idx;
    f.snd.resize((size_t)f.wnd.msg_len * 2);
    for (size_t i = 0; i < f.snd.size(); ++i) {
        f.snd[i] = (unsigned char)i;
    }
    std::fill(std::begin(f.buf), std::end(f.buf), 0xFE);
    arq__wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    MEMCMP_EQUAL(f.snd.data(), &f.buf[(size_t)(orig_msg_idx * f.wnd.msg_len)], (size_t)f.wnd.msg_len);
    MEMCMP_EQUAL(&f.snd[(size_t)f.wnd.msg_len], f.buf.data(), (size_t)f.wnd.msg_len);
}

TEST(window, send_wraps_copy_around_and_respects_partially_filled_starting_message)
{
    Fixture f;
    int const orig_msg_idx = f.wnd.size_in_msgs - 1;
    f.msg[(size_t)orig_msg_idx].len = 3;
    f.wnd.base_msg_idx = orig_msg_idx;
    f.wnd.cur_msg_idx = orig_msg_idx;
    f.snd.resize((size_t)f.wnd.msg_len + 8);
    for (size_t i = 0; i < f.snd.size(); ++i) {
        f.snd[i] = (unsigned char)i;
    }
    std::fill(std::begin(f.buf), std::end(f.buf), 0xFE);
    arq__wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    MEMCMP_EQUAL(f.snd.data(), &f.buf[(size_t)(orig_msg_idx * f.wnd.msg_len) + 3], (size_t)f.wnd.msg_len - 3);
    MEMCMP_EQUAL(&f.snd[(size_t)f.wnd.msg_len - 3], f.buf.data(), 11);
}

}

