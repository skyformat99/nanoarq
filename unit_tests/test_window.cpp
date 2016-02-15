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
        wnd.base_seq = 0;
        wnd.base_idx = 0;
        wnd.cur_idx = 0;

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

TEST(window, send_with_empty_window_copies_to_start_of_buf)
{
    Fixture f;
    f.snd.resize(5);
    arq__wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    MEMCMP_EQUAL(f.snd.data(), f.buf.data(), f.snd.size());
}

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
    CHECK_EQUAL(1, f.wnd.cur_idx);
}

TEST(window, cur_index_wraps_maybe_figure_out_if_its_an_offset_or_an_index)
{
}

TEST(window, send_copies_data_to_current_message_space_in_buf)
{
    Fixture f;
    f.wnd.cur_idx = 1;
    f.snd.resize(19);
    for (size_t i = 0; i < f.snd.size(); ++i) {
        f.snd[i] = (unsigned char)i;
    }
    arq__wnd_send(&f.wnd, f.snd.data(), f.snd.size());
    MEMCMP_EQUAL(f.snd.data(), &f.buf[(size_t)f.wnd.msg_len], f.snd.size());
}

}

