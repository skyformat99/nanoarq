#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTestExt/MockSupport.h>
#include <CppUTest/TestHarness.h>
#include <array>
#include <vector>
#include <cstring>

TEST_GROUP(recv_wnd) {};

namespace {

void MockWndRst(arq__wnd_t *w)
{
    mock().actualCall("arq__wnd_rst").withParameter("w", w);
}

struct UninitializedFixture
{
    UninitializedFixture()
    {
        rw.w.cap = (arq_uint16_t)msg.size();
        buf.resize(1024 * 1024);
        rw.ack = ack.data();
        rw.w.msg = msg.data();
        rw.w.buf = buf.data();
        recv.resize(buf.size());
        std::fill(std::begin(buf), std::end(buf), 0xFE);
        std::fill(std::begin(recv), std::end(recv), 0xFE);
        std::fill(std::begin(ack), std::end(ack), 0xFE);
    }

    arq__recv_wnd_t rw;
    std::array< arq__msg_t, 64 > msg;
    std::array< arq_uchar_t, 64 > ack;
    std::vector< arq_uchar_t > buf;
    std::vector< arq_uchar_t > seg;
    std::vector< arq_uchar_t > recv;
};

TEST(recv_wnd, rst_calls_wnd_rst)
{
    UninitializedFixture f;
    ARQ_MOCK_HOOK(arq__wnd_rst, MockWndRst);
    mock().expectOneCall("arq__wnd_rst").withParameter("w", &f.rw.w);
    arq__recv_wnd_rst(&f.rw);
}

TEST(recv_wnd, rst_sets_copy_seq_and_copy_ofs_to_zero)
{
    UninitializedFixture f;
    ARQ_MOCK_HOOK(arq__wnd_rst, MockWndRst);
    mock().ignoreOtherCalls();
    f.rw.w.cap = 0;
    f.rw.copy_ofs = 123u;
    f.rw.copy_seq = 321u;
    arq__recv_wnd_rst(&f.rw);
    CHECK_EQUAL(0, f.rw.copy_seq);
    CHECK_EQUAL(0, f.rw.copy_ofs);
}

TEST(recv_wnd, rst_sets_slide_to_zero)
{
    UninitializedFixture f;
    ARQ_MOCK_HOOK(arq__wnd_rst, MockWndRst);
    mock().ignoreOtherCalls();
    f.rw.w.cap = 0;
    f.rw.slide = 1234;
    arq__recv_wnd_rst(&f.rw);
    CHECK_EQUAL(0, f.rw.slide);
}

TEST(recv_wnd, rst_clears_ack_array)
{
    UninitializedFixture f;
    ARQ_MOCK_HOOK(arq__wnd_rst, MockWndRst);
    mock().ignoreOtherCalls();
    arq__recv_wnd_rst(&f.rw);
    for (auto const &ack : f.ack) {
        CHECK_EQUAL(0, ack);
    }
}

TEST(recv_wnd, rst_resets_inter_seg_ack_timer)
{
    UninitializedFixture f;
    ARQ_MOCK_HOOK(arq__wnd_rst, MockWndRst);
    mock().ignoreOtherCalls();
    f.rw.inter_seg_ack_on = ARQ_TRUE;
    f.rw.inter_seg_ack = 1234;
    f.rw.inter_seg_ack_seq = 5;
    arq__recv_wnd_rst(&f.rw);
    CHECK_EQUAL(ARQ_FALSE, f.rw.inter_seg_ack_on);
    CHECK_EQUAL(ARQ_TIME_INFINITY, f.rw.inter_seg_ack);
    CHECK_EQUAL(0, f.rw.inter_seg_ack_seq);
}

struct Fixture : UninitializedFixture
{
    Fixture()
    {
        arq__wnd_init(&rw.w, msg.size(), 128, 32);
        arq__recv_wnd_rst(&rw);
    }
};

TEST(recv_wnd, frame_writes_len_to_msg)
{
    Fixture f;
    f.seg.resize(30);
    arq__recv_wnd_frame(&f.rw, 0, 0, 1, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(f.seg.size(), f.msg[0].len);
}

TEST(recv_wnd, frame_accumulates_seg_len_to_partially_full_msg)
{
    Fixture f;
    f.seg.resize(29);
    f.msg[0].len = 17;
    arq__recv_wnd_frame(&f.rw, 0, 1, 2, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(17 + 29, f.msg[0].len);
}

TEST(recv_wnd, frame_indexes_msg_by_seq)
{
    Fixture f;
    f.seg.resize(1);
    f.rw.w.seq = 12344;
    arq__recv_wnd_frame(&f.rw, 12345, 0, 1, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(1, f.msg[12345 % f.rw.w.cap].len);
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
    f.rw.w.seq = 120;
    int const seq = 123;
    int const seg = 456;
    for (auto i = 0; i < 7; ++i) {
        f.seg.emplace_back((unsigned char)i);
    }
    void *seat = &f.buf[9];
    ARQ_MOCK_HOOK(arq__wnd_seg, MockWndSeg);
    mock().expectOneCall("arq__wnd_seg")
          .withParameter("w", &f.rw.w)
          .withParameter("seq", seq)
          .withParameter("seg", seg)
          .withOutputParameterReturning("out_seg", &seat, sizeof(void *))
          .ignoreOtherParameters();
    arq__recv_wnd_frame(&f.rw, seq, seg, 1, f.seg.data(), f.seg.size(), 0);
    MEMCMP_EQUAL(f.seg.data(), seat, f.seg.size());
}

TEST(recv_wnd, frame_writes_full_ack_vec_to_msg)
{
    Fixture f;
    f.seg.resize(1);
    f.msg[0].full_ack_vec = 0;
    arq__recv_wnd_frame(&f.rw, 0, 0, 7, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(0b1111111, f.msg[0].full_ack_vec);
}

TEST(recv_wnd, frame_ors_in_current_seg_to_cur_ack_vector)
{
    Fixture f;
    f.seg.resize(1);
    f.msg[0].full_ack_vec = 0b1111;
    f.msg[0].cur_ack_vec = 0b0101;
    arq__recv_wnd_frame(&f.rw, 0, 3, 4, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(0b1101, f.msg[0].cur_ack_vec);
}

TEST(recv_wnd, frame_does_not_update_window_size_if_segment_in_existing_message)
{
    Fixture f;
    f.rw.w.size = 1;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.rw, f.rw.w.seq, 1, 2, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(1, f.rw.w.size);
}

TEST(recv_wnd, frame_increments_window_size_for_first_segment_in_next_message)
{
    Fixture f;
    f.rw.w.size = 1;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.rw, f.rw.w.seq + 1, 0, 1, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(2, f.rw.w.size);
}

TEST(recv_wnd, frame_increments_window_size_for_non_first_segment_in_next_message)
{
    Fixture f;
    f.rw.w.size = 1;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.rw, f.rw.w.seq + 1, 1, 2, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(2, f.rw.w.size);
}

TEST(recv_wnd, frame_updates_window_size_for_first_segment_in_non_adjacent_message)
{
    Fixture f;
    f.rw.w.size = 0;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.rw, f.rw.w.seq + 4, 0, 1, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(5, f.rw.w.size);
}

TEST(recv_wnd, frame_updates_window_size_for_non_first_segment_in_non_adjacent_message)
{
    Fixture f;
    f.rw.w.size = 0;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.rw, f.rw.w.seq + 4, 2, 4, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(5, f.rw.w.size);
}

TEST(recv_wnd, frame_updates_window_size_correctly_when_new_segment_wraps_sequence_number_space)
{
    Fixture f;
    f.rw.w.size = 1;
    f.rw.w.seq = ARQ__FRAME_MAX_SEQ_NUM;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.rw, 0, 1, 1, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(2, f.rw.w.size);
}

TEST(recv_wnd, frame_returns_segment_length_if_inside_window_and_valid)
{
    Fixture f;
    f.seg.resize(19);
    unsigned const len = arq__recv_wnd_frame(&f.rw, 0, 0, 1, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(f.seg.size(), len);
}

TEST(recv_wnd, frame_returns_zero_if_segment_already_received)
{
    Fixture f;
    f.seg.resize(1);
    f.msg[0].cur_ack_vec = 0b010;
    unsigned const len = arq__recv_wnd_frame(&f.rw, 0, 1, 3, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(0, len);
}

TEST(recv_wnd, frame_doesnt_set_ack_entry_for_first_segment_of_message_in_window)
{
    Fixture f;
    f.seg.resize(1);
    CHECK_EQUAL(ARQ_FALSE, f.rw.ack[0]);
    arq__recv_wnd_frame(&f.rw, 0, 0, 2, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(ARQ_FALSE, f.rw.ack[0]);
}

TEST(recv_wnd, frame_doesnt_set_ack_entry_for_internal_segment_of_message_in_window)
{
    Fixture f;
    f.seg.resize(1);
    CHECK_EQUAL(ARQ_FALSE, f.rw.ack[0]);
    arq__recv_wnd_frame(&f.rw, 0, 1, 3, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(ARQ_FALSE, f.rw.ack[0]);
}

TEST(recv_wnd, frame_sets_ack_set_entry_to_true_if_last_segment_of_message_is_in_window)
{
    Fixture f;
    f.seg.resize(1);
    CHECK_EQUAL(ARQ_FALSE, f.rw.ack[0]);
    arq__recv_wnd_frame(&f.rw, 0, 6, 7, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(ARQ_TRUE, f.rw.ack[0]);
}

TEST(recv_wnd, frame_sets_ack_set_entry_to_one_if_last_segment_of_message_is_in_window_big_seq)
{
    Fixture f;
    f.seg.resize(1);
    f.rw.w.seq = ARQ__FRAME_MAX_SEQ_NUM;
    CHECK_EQUAL(ARQ_FALSE, f.rw.ack[f.rw.w.seq % f.rw.w.cap]);
    arq__recv_wnd_frame(&f.rw, ARQ__FRAME_MAX_SEQ_NUM, 0, 1, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(ARQ_TRUE, f.rw.ack[f.rw.w.seq % f.rw.w.cap]);
}

TEST(recv_wnd, frame_doesnt_slide_window_when_msg_arrives_and_window_can_grow_to_hold_it)
{
    Fixture f;
    f.rw.w.size = f.rw.w.cap - 1;
    arq_uint16_t const slide = f.rw.w.size;
    f.rw.slide = slide;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.rw, f.rw.w.size, 0, 1, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(slide, f.rw.slide);
    CHECK_EQUAL(0, f.rw.w.seq);
}

TEST(recv_wnd, frame_doesnt_slide_window_when_msg_arrives_and_window_is_full_and_not_received_by_user)
{
    Fixture f;
    f.rw.w.size = f.rw.w.cap;
    f.rw.slide = 0;
    for (auto i = 0u; i < f.rw.w.cap; ++i) {
        f.rw.w.msg[i].len = f.rw.w.msg_len;
    }
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.rw, f.rw.w.size, 0, 1, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(0, f.rw.w.seq);
}

TEST(recv_wnd, frame_doesnt_slide_window_when_full_and_msg_requires_slide_but_havent_seen_base_seq_yet)
{
    Fixture f;
    f.rw.w.size = f.rw.w.cap;
    f.rw.slide = 0;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.rw, f.rw.w.size, 0, 1, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(0, f.rw.w.seq);
}

TEST(recv_wnd, frame_slides_window_by_one_when_full_and_first_message_is_received_and_next_seq_arrives)
{
    Fixture f;
    f.rw.w.size = f.rw.w.cap;
    f.rw.slide = 1;
    for (auto i = 1u; i < f.rw.w.cap; ++i) {
        f.rw.w.msg[i].len = f.rw.w.msg_len;
    }
    f.seg.resize(13);
    unsigned const new_seq = f.rw.w.size;
    arq__recv_wnd_frame(&f.rw, new_seq, 0, 1, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(1, f.rw.w.seq);
    CHECK_EQUAL(13, f.rw.w.msg[new_seq % f.rw.w.cap].len);
}

TEST(recv_wnd, frame_slides_window_by_two_when_full_and_first_2_messages_received_and_seq_plus_2_arrives)
{
    Fixture f;
    f.rw.w.size = f.rw.w.cap;
    f.rw.slide = 2;
    for (auto i = 2u; i < f.rw.w.cap; ++i) {
        f.rw.w.msg[i].len = f.rw.w.msg_len;
    }
    f.seg.resize(13);
    unsigned const new_seq = f.rw.w.size + 1;
    arq__recv_wnd_frame(&f.rw, new_seq, 0, 1, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(2, f.rw.w.seq);
    CHECK_EQUAL(13, f.rw.w.msg[new_seq % f.rw.w.cap].len);
}

TEST(recv_wnd, frame_slides_window_by_cap_when_full_and_all_messages_received_and_seq_cap_times_2_arrives)
{
    Fixture f;
    f.rw.w.size = f.rw.w.cap;
    f.rw.slide = f.rw.w.size;
    f.seg.resize(13);
    unsigned const new_seq = (f.rw.w.cap * 2) - 1;
    arq__recv_wnd_frame(&f.rw, new_seq, 0, 1, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(new_seq, f.rw.w.seq + f.rw.w.size - 1u);
    CHECK_EQUAL(13, f.rw.w.msg[new_seq % f.rw.w.cap].len);
}

TEST(recv_wnd, frame_leaves_ack_set_entry_to_one_if_ack_is_already_noticed)
{
    Fixture f;
    f.seg.resize(1);
    f.rw.ack[0] = ARQ_TRUE;
    arq__recv_wnd_frame(&f.rw, 0, 0, 1, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(ARQ_TRUE, f.rw.ack[0]);
}

TEST(recv_wnd, frame_sets_ack_entry_if_already_received_segment_is_received)
{
    Fixture f;
    f.seg.resize(1);
    f.rw.w.size = 1;
    f.rw.w.msg[0].cur_ack_vec = 1;
    f.rw.ack[0] = ARQ_FALSE;
    arq__recv_wnd_frame(&f.rw, 0, 0, 3, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(ARQ_TRUE, f.rw.ack[0]);
}

TEST(recv_wnd, frame_doesnt_enable_inter_seg_ack_timer_when_single_segment_message_arrives)
{
    Fixture f;
    f.seg.resize(1);
    f.rw.inter_seg_ack_on = ARQ_FALSE;
    arq__recv_wnd_frame(&f.rw, 0, 0, 1, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(ARQ_FALSE, f.rw.inter_seg_ack_on);
}

TEST(recv_wnd, frame_enables_inter_seg_ack_timer_when_first_segment_of_multi_seg_msg_arrives)
{
    Fixture f;
    f.seg.resize(1);
    f.rw.inter_seg_ack_on = ARQ_FALSE;
    arq__recv_wnd_frame(&f.rw, 0, 0, 2, f.seg.data(), f.seg.size(), 321);
    CHECK_EQUAL(ARQ_TRUE, f.rw.inter_seg_ack_on);
    CHECK_EQUAL(321, f.rw.inter_seg_ack);
}

TEST(recv_wnd, frame_sets_inter_seg_ack_seq_to_msg_seq)
{
    Fixture f;
    f.seg.resize(1);
    f.rw.inter_seg_ack_on = ARQ_FALSE;
    arq__recv_wnd_frame(&f.rw, 3, 0, 2, f.seg.data(), f.seg.size(), 321);
    CHECK_EQUAL(ARQ_TRUE, f.rw.inter_seg_ack_on);
    CHECK_EQUAL(3, f.rw.inter_seg_ack_seq);
}

TEST(recv_wnd, frame_disables_inter_seg_ack_timer_when_last_segment_of_multi_seg_msg_arrives)
{
    Fixture f;
    f.seg.resize(1);
    f.rw.inter_seg_ack_on = ARQ_TRUE;
    arq__recv_wnd_frame(&f.rw, 0, 1, 2, f.seg.data(), f.seg.size(), 0);
    CHECK_EQUAL(ARQ_FALSE, f.rw.inter_seg_ack_on);
}

TEST(recv_wnd, recv_empty_window_copies_nothing)
{
    Fixture f;
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    for (auto const &b : f.recv) {
        CHECK_EQUAL(0xFE, b);
    }
}

TEST(recv_wnd, recv_empty_window_returns_zero)
{
    Fixture f;
    unsigned const len = arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    CHECK_EQUAL(0, len);
}

void PopulateReceiveWindow(Fixture& f, int bytes)
{
    for (auto i = 0; i < bytes / 2; ++i) {
        arq_uint16_t const x = (arq_uint16_t)i;
        auto const offset = f.rw.w.seq * f.rw.w.msg_len;
        auto const max = f.rw.w.cap * f.rw.w.msg_len;
        std::memcpy(&f.buf[(offset + (i * 2)) % max], &x, sizeof(x));
    }
    f.rw.w.size = (arq_uint16_t)((bytes + f.rw.w.msg_len - 1) / f.rw.w.msg_len);
    int idx = 0;
    while (bytes) {
        arq__msg_t *m = &f.rw.w.msg[(f.rw.w.seq + idx++) % f.rw.w.cap];
        m->len = (arq_uint16_t)arq__min(bytes, f.rw.w.msg_len);
        m->cur_ack_vec = m->full_ack_vec = f.rw.w.full_ack_vec;
        bytes -= m->len;
    }
}

TEST(recv_wnd, PopulateReceiveWindow_empty)
{
    Fixture f;
    PopulateReceiveWindow(f, 0);
    CHECK_EQUAL(0, f.rw.w.size);
}

TEST(recv_wnd, PopulateReceiveWindow_one_full_msg)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len);
    CHECK_EQUAL(1, f.rw.w.size);
    for (int i = 0; i < f.rw.w.msg_len / 2; ++i) {
        arq_uint16_t x;
        std::memcpy(&x, &f.buf[i * 2], sizeof(x));
        CHECK_EQUAL((arq_uint16_t)i, x);
    }
    CHECK_EQUAL(0xFE, f.buf[f.rw.w.msg_len]);
}

TEST(recv_wnd, PopulateReceiveWindow_one_full_msg_one_partial_msg)
{
    Fixture f;
    int const size = f.rw.w.msg_len + (f.rw.w.msg_len / 2);
    PopulateReceiveWindow(f, size);
    CHECK_EQUAL(2, f.rw.w.size);
    for (int i = 0; i < size / 2; ++i) {
        arq_uint16_t x;
        std::memcpy(&x, &f.buf[i * 2], sizeof(x));
        CHECK_EQUAL((arq_uint16_t)i, x);
    }
    CHECK_EQUAL(0xFE, f.buf[size]);
}

TEST(recv_wnd, recv_dst_size_zero_does_nothing)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), 0);
    CHECK_EQUAL(0, f.rw.w.seq);
    CHECK_EQUAL(0, f.rw.copy_seq);
    CHECK_EQUAL(0, f.rw.copy_ofs);
    CHECK_EQUAL(1, f.rw.w.size);
}

TEST(recv_wnd, recv_entire_first_message_in_window_copies_msg_to_buf)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    MEMCMP_EQUAL(f.buf.data(), f.recv.data(), f.rw.w.msg_len);
}

TEST(recv_wnd, recv_entire_first_message_in_window_returns_msg_len)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len);
    unsigned const recv_size = arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    CHECK_EQUAL(f.rw.w.msg_len, recv_size);
}

TEST(recv_wnd, recv_dst_less_than_one_message_len_copies_partial_msg)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len - 1);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    MEMCMP_EQUAL(f.buf.data(), f.recv.data(), f.rw.w.msg_len - 1);
    CHECK_EQUAL(0xFE, f.buf[f.rw.w.msg_len - 1]);
}

TEST(recv_wnd, recv_entire_first_message_in_window_increments_slide)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len);
    f.rw.slide = 13;
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    CHECK_EQUAL(14, f.rw.slide);
}

TEST(recv_wnd, recv_tinygram_returns_tinygram_size)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len - 1);
    unsigned const recv_size = arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    CHECK_EQUAL(f.rw.w.msg_len - 1u, recv_size);
}

TEST(recv_wnd, recv_dst_two_full_messages_copies_both_messages_to_dst)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len * 2);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    MEMCMP_EQUAL(f.buf.data(), f.recv.data(), f.rw.w.msg_len * 2);
    CHECK_EQUAL(0xFE, f.buf[f.rw.w.msg_len * 2]);
}

TEST(recv_wnd, recv_dst_two_full_messages_increments_slide_by_two)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len * 2);
    f.rw.slide = 123;
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    CHECK_EQUAL(125, f.rw.slide);
}

TEST(recv_wnd, recv_dst_two_full_messages_returns_two_times_msg_len)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len * 2);
    unsigned const recv_size = arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    CHECK_EQUAL((unsigned)(f.rw.w.msg_len * 2), recv_size);
}

TEST(recv_wnd, recv_full_msg_sets_copy_seq_to_base_seq_plus_one)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.rw.w.msg_len);
    CHECK_EQUAL(1, f.rw.copy_seq);
}

TEST(recv_wnd, recv_full_msg_sets_copy_ofs_to_zero)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.rw.w.msg_len);
    CHECK_EQUAL(0, f.rw.copy_ofs);
}

TEST(recv_wnd, recv_partial_msg_leaves_copy_seq_untouched)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), 3);
    CHECK_EQUAL(0, f.rw.copy_seq);
}

TEST(recv_wnd, recv_partial_msg_sets_copy_ofs_to_message_offset_for_next_recv_call)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), 3);
    CHECK_EQUAL(3, f.rw.copy_ofs);
}

TEST(recv_wnd, recv_one_and_a_half_messages_leaves_copy_vars_halfway_through_message)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len * 2);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.rw.w.msg_len + (f.rw.w.msg_len / 2));
    CHECK_EQUAL(1, f.rw.copy_seq);
    CHECK_EQUAL(f.rw.w.msg_len / 2, f.rw.copy_ofs);
}

TEST(recv_wnd, recv_two_and_a_half_messages_leaves_copy_vars_halfway_through_third_message)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len * 3);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.rw.w.msg_len + (f.rw.w.msg_len / 2));
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.rw.w.msg_len);
    CHECK_EQUAL(2, f.rw.copy_seq);
    CHECK_EQUAL(f.rw.w.msg_len / 2, f.rw.copy_ofs);
}

TEST(recv_wnd, recv_after_receiving_partial_msg_resumes_copy_from_last_byte_copied)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), 2);
    arq__recv_wnd_recv(&f.rw, &f.recv[2], f.rw.w.msg_len - 2);
    MEMCMP_EQUAL(f.buf.data(), f.recv.data(), f.rw.w.msg_len);
}

TEST(recv_wnd, recv_entire_msg_one_byte_at_a_time)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len);
    for (auto i = 0u; i < f.rw.w.msg_len; ++i) {
        unsigned const n = arq__recv_wnd_recv(&f.rw, &f.recv[i], 1);
        CHECK_EQUAL(1, n);
    }
    MEMCMP_EQUAL(f.buf.data(), f.recv.data(), f.rw.w.msg_len);
}

TEST(recv_wnd, recv_two_messages_one_byte_at_a_time)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len * 2);
    CHECK_EQUAL(2, f.rw.w.size);
    for (auto i = 0u; i < (unsigned)(f.rw.w.msg_len * 2); ++i) {
        unsigned const n = arq__recv_wnd_recv(&f.rw, &f.recv[i], 1);
        CHECK_EQUAL(1, n);
    }
    MEMCMP_EQUAL(f.buf.data(), f.recv.data(), f.rw.w.msg_len * 2);
}

TEST(recv_wnd, recv_after_receiving_partial_msg_returns_resumed_partial_message_length)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), 2);
    unsigned const recv_size = arq__recv_wnd_recv(&f.rw, f.recv.data(), f.rw.w.msg_len - 2);
    CHECK_EQUAL(f.rw.w.msg_len - 2u, recv_size);
}

TEST(recv_wnd, recv_copies_message_data_when_window_wraps_around_capacity)
{
    Fixture f;
    f.rw.w.seq = f.rw.w.cap - 1;
    f.rw.copy_seq = f.rw.w.seq;
    PopulateReceiveWindow(f, f.rw.w.msg_len * 2);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    MEMCMP_EQUAL(&f.buf[(f.rw.w.cap - 1) * f.rw.w.msg_len], f.recv.data(), f.rw.w.msg_len);
    MEMCMP_EQUAL(f.buf.data(), &f.recv[f.rw.w.msg_len], f.rw.w.msg_len);
}

TEST(recv_wnd, recv_resets_len_and_ack_vector_when_receiving)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len);
    CHECK_EQUAL(f.rw.w.msg_len, f.msg[0].len);
    CHECK_EQUAL(f.rw.w.full_ack_vec, f.msg[0].cur_ack_vec);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    CHECK_EQUAL(0, f.msg[0].len);
    CHECK_EQUAL(0, f.msg[0].cur_ack_vec);
}

TEST(recv_wnd, recv_resets_len_and_ack_vector_when_receiving_more_than_one_message)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len * 3);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    CHECK_EQUAL(0, f.msg[0].len);
    CHECK_EQUAL(0, f.msg[0].cur_ack_vec);
    CHECK_EQUAL(0, f.msg[1].len);
    CHECK_EQUAL(0, f.msg[1].cur_ack_vec);
    CHECK_EQUAL(0, f.msg[2].len);
    CHECK_EQUAL(0, f.msg[2].cur_ack_vec);
}

TEST(recv_wnd, recv_one_full_msg_one_tinygram_one_full_msg)
{
    Fixture f;
    f.rw.w.size = 3;
    f.rw.w.msg[0].len = f.rw.w.msg_len;
    f.rw.w.msg[0].cur_ack_vec = f.rw.w.full_ack_vec;
    f.rw.w.msg[1].len = 1;
    f.rw.w.msg[1].cur_ack_vec = f.rw.w.msg[1].full_ack_vec = 1;
    f.rw.w.msg[2].len = f.rw.w.msg_len;
    f.rw.w.msg[2].cur_ack_vec = f.rw.w.full_ack_vec;
    std::memset(f.buf.data(), 0x11, f.rw.w.msg_len);
    f.buf[f.rw.w.msg_len] = 0x22;
    std::memset(&f.buf[f.rw.w.msg_len * 2], 0x33, f.rw.w.msg_len);
    unsigned const recvd = arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    CHECK_EQUAL(f.rw.w.msg_len * 2u + 1u, recvd);
    for (auto i = 0u; i < f.rw.w.msg_len; ++i) {
        CHECK_EQUAL(0x11, f.recv[i]);
    }
    CHECK_EQUAL(0x22, f.recv[f.rw.w.msg_len]);
    for (auto i = f.rw.w.msg_len + 2; i < f.rw.w.msg_len * 2 + 1; ++i) {
        CHECK_EQUAL(0x33, f.recv[i]);
    }
}

TEST(recv_wnd, pending_returns_false_if_window_is_empty)
{
    Fixture f;
    f.rw.w.size = 0;
    CHECK_EQUAL(ARQ_FALSE, arq__recv_wnd_pending(&f.rw));
}

TEST(recv_wnd, pending_returns_false_if_first_message_ack_vector_isnt_full)
{
    Fixture f;
    f.rw.w.size = 1;
    f.rw.w.msg[0].full_ack_vec = f.rw.w.full_ack_vec;
    f.rw.w.msg[0].cur_ack_vec = 0;
    CHECK_EQUAL(ARQ_FALSE, arq__recv_wnd_pending(&f.rw));
}

TEST(recv_wnd, pending_returns_true_if_first_message_ack_vector_is_full)
{
    Fixture f;
    f.rw.w.size = 1;
    f.rw.w.msg[0].full_ack_vec = 7;
    f.rw.w.msg[0].cur_ack_vec = 7;
    CHECK_EQUAL(ARQ_TRUE, arq__recv_wnd_pending(&f.rw));
}

}

