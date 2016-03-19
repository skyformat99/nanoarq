#include "nanoarq_unit_test.h"
#include "nanoarq_hook_plugin.h"
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

TEST(recv_wnd, rst_calls_wnd_rst)
{
    ARQ_MOCK_HOOK(arq__wnd_rst, MockWndRst);
    arq__recv_wnd_t rw;
    rw.w.cap = 0;
    mock().expectOneCall("arq__wnd_rst").withParameter("w", &rw.w);
    arq__recv_wnd_rst(&rw);
}

TEST(recv_wnd, rst_sets_recv_ptr_and_ack_ptr_to_zero)
{
    ARQ_MOCK_HOOK(arq__wnd_rst, MockWndRst);
    mock().ignoreOtherCalls();
    arq__recv_wnd_t rw;
    rw.w.cap = 0;
    rw.recv_ptr = rw.ack_ptr = 1234;
    arq__recv_wnd_rst(&rw);
    CHECK_EQUAL(0, rw.ack_ptr);
    CHECK_EQUAL(0, rw.recv_ptr);
}

TEST(recv_wnd, rst_clears_ack_array)
{
    ARQ_MOCK_HOOK(arq__wnd_rst, MockWndRst);
    mock().ignoreOtherCalls();
    std::array< arq_uchar_t, 4 > ack;
    std::fill(std::begin(ack), std::end(ack), 0xFE);
    arq__recv_wnd_t rw;
    rw.ack = ack.data();
    rw.w.cap = 4;
    arq__recv_wnd_rst(&rw);
    for (auto const &f : ack) {
        CHECK_EQUAL(0, f);
    }
}

struct Fixture
{
    Fixture()
    {
        rw.ack = ack.data();
        rw.w.msg = msg.data();
        arq__wnd_init(&rw.w, msg.size(), 128, 32);
        buf.resize(rw.w.msg_len * rw.w.cap);
        rw.w.buf = buf.data();
        arq__recv_wnd_rst(&rw);
        recv.resize(buf.size());
        std::fill(std::begin(buf), std::end(buf), 0xFE);
        std::fill(std::begin(recv), std::end(recv), 0xFE);
    }

    arq__recv_wnd_t rw;
    std::array< arq__msg_t, 64 > msg;
    std::array< arq_uchar_t, 64 > ack;
    std::vector< arq_uchar_t > buf;
    std::vector< arq_uchar_t > seg;
    std::vector< arq_uchar_t > recv;
};

TEST(recv_wnd, frame_writes_len_to_msg)
{
    Fixture f;
    f.seg.resize(30);
    arq__recv_wnd_frame(&f.rw, 0, 0, 1, f.seg.data(), f.seg.size());
    CHECK_EQUAL(f.seg.size(), f.msg[0].len);
}

TEST(recv_wnd, frame_accumulates_seg_len_to_partially_full_msg)
{
    Fixture f;
    f.seg.resize(29);
    f.msg[0].len = 17;
    arq__recv_wnd_frame(&f.rw, 0, 1, 2, f.seg.data(), f.seg.size());
    CHECK_EQUAL(17 + 29, f.msg[0].len);
}

TEST(recv_wnd, frame_indexes_msg_by_seq)
{
    Fixture f;
    f.seg.resize(1);
    f.rw.w.seq = 12344;
    arq__recv_wnd_frame(&f.rw, 12345, 0, 1, f.seg.data(), f.seg.size());
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
    arq__recv_wnd_frame(&f.rw, seq, seg, 1, f.seg.data(), f.seg.size());
    MEMCMP_EQUAL(f.seg.data(), seat, f.seg.size());
}

TEST(recv_wnd, frame_writes_full_ack_vec_to_msg)
{
    Fixture f;
    f.seg.resize(1);
    f.msg[0].full_ack_vec = 0;
    arq__recv_wnd_frame(&f.rw, 0, 0, 7, f.seg.data(), f.seg.size());
    CHECK_EQUAL(0b1111111, f.msg[0].full_ack_vec);
}

TEST(recv_wnd, frame_ors_in_current_seg_to_cur_ack_vector)
{
    Fixture f;
    f.seg.resize(1);
    f.msg[0].full_ack_vec = 0b1111;
    f.msg[0].cur_ack_vec = 0b0101;
    arq__recv_wnd_frame(&f.rw, 0, 3, 4, f.seg.data(), f.seg.size());
    CHECK_EQUAL(0b1101, f.msg[0].cur_ack_vec);
}

TEST(recv_wnd, frame_does_not_update_window_size_if_segment_in_existing_message)
{
    Fixture f;
    f.rw.w.size = 1;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.rw, f.rw.w.seq, 1, 2, f.seg.data(), f.seg.size());
    CHECK_EQUAL(1, f.rw.w.size);
}

TEST(recv_wnd, frame_increments_window_size_for_first_segment_in_next_message)
{
    Fixture f;
    f.rw.w.size = 1;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.rw, f.rw.w.seq + 1, 0, 1, f.seg.data(), f.seg.size());
    CHECK_EQUAL(2, f.rw.w.size);
}

TEST(recv_wnd, frame_increments_window_size_for_non_first_segment_in_next_message)
{
    Fixture f;
    f.rw.w.size = 1;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.rw, f.rw.w.seq + 1, 1, 2, f.seg.data(), f.seg.size());
    CHECK_EQUAL(2, f.rw.w.size);
}

TEST(recv_wnd, frame_updates_window_size_for_first_segment_in_non_adjacent_message)
{
    Fixture f;
    f.rw.w.size = 0;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.rw, f.rw.w.seq + 4, 0, 1, f.seg.data(), f.seg.size());
    CHECK_EQUAL(5, f.rw.w.size);
}

TEST(recv_wnd, frame_updates_window_size_for_non_first_segment_in_non_adjacent_message)
{
    Fixture f;
    f.rw.w.size = 0;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.rw, f.rw.w.seq + 4, 2, 4, f.seg.data(), f.seg.size());
    CHECK_EQUAL(5, f.rw.w.size);
}

TEST(recv_wnd, frame_updates_window_size_correctly_when_new_segment_wraps_sequence_number_space)
{
    Fixture f;
    f.rw.w.size = 1;
    f.rw.w.seq = ARQ__FRAME_MAX_SEQ_NUM;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.rw, 0, 1, 1, f.seg.data(), f.seg.size());
    CHECK_EQUAL(2, f.rw.w.size);
}

TEST(recv_wnd, frame_ignores_sequence_numbers_greater_than_window_capacity)
{
    Fixture f;
    f.rw.w.cap = 10;
    f.rw.w.seq = 100;
    f.rw.w.size = 2;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.rw, 2000, 0, 1, f.seg.data(), f.seg.size());
    CHECK_EQUAL(2, f.rw.w.size);
}

TEST(recv_wnd, frame_ignores_sequence_numbers_less_than_window_base)
{
    Fixture f;
    f.rw.w.cap = 10;
    f.rw.w.seq = 100;
    f.rw.w.size = 2;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.rw, 9, 0, 1, f.seg.data(), f.seg.size());
    CHECK_EQUAL(2, f.rw.w.size);
}

TEST(recv_wnd, frame_returns_segment_length_if_inside_window_and_valid)
{
    Fixture f;
    f.seg.resize(19);
    unsigned const len = arq__recv_wnd_frame(&f.rw, 0, 0, 1, f.seg.data(), f.seg.size());
    CHECK_EQUAL(f.seg.size(), len);
}

TEST(recv_wnd, frame_returns_zero_if_outside_of_window)
{
    Fixture f;
    f.rw.w.seq = 123;
    f.seg.resize(1);
    unsigned const len = arq__recv_wnd_frame(&f.rw, f.rw.w.seq - 1, 0, 1, f.seg.data(), f.seg.size());
    CHECK_EQUAL(0, len);
}

TEST(recv_wnd, frame_returns_zero_if_segment_already_received)
{
    Fixture f;
    f.seg.resize(1);
    f.msg[0].cur_ack_vec = 0b010;
    unsigned const len = arq__recv_wnd_frame(&f.rw, 0, 1, 3, f.seg.data(), f.seg.size());
    CHECK_EQUAL(0, len);
}

TEST(recv_wnd, frame_doesnt_set_ack_entry_for_first_segment_of_message_in_window)
{
    CHECK(0);
}

TEST(recv_wnd, frame_doesnt_set_ack_entry_for_internal_segment_of_message_in_window)
{
    CHECK(0);
}

TEST(recv_wnd, frame_sets_ack_set_entry_to_one_if_last_segment_of_message_is_in_window)
{
    Fixture f;
    f.seg.resize(1);
    CHECK_EQUAL(0, f.rw.ack[0]);
    arq__recv_wnd_frame(&f.rw, 0, 0, 1, f.seg.data(), f.seg.size());
    CHECK_EQUAL(1, f.rw.ack[0]);
}

TEST(recv_wnd, frame_resets_ack_timer_when_segment_received)
{
    CHECK(0);
}

TEST(recv_wnd, frame_leaves_ack_set_entry_to_one_if_ack_is_already_noticed)
{
    Fixture f;
    f.seg.resize(1);
    f.rw.ack[0] = 1;
    arq__recv_wnd_frame(&f.rw, 0, 0, 1, f.seg.data(), f.seg.size());
    CHECK_EQUAL(1, f.rw.ack[0]);
}

TEST(recv_wnd, frame_leaves_ack_entry_untouched_if_outside_of_window)
{
    Fixture f;
    f.seg.resize(1);
    f.rw.w.seq = 1;
    arq__recv_wnd_frame(&f.rw, 0, 0, 1, f.seg.data(), f.seg.size());
    for (auto const &b : f.ack) {
        CHECK_EQUAL(0, b);
    }
}

TEST(recv_wnd, frame_sets_ack_entry_for_every_segment_of_message_outside_of_window)
{
    CHECK(0);
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
    f.rw.w.size = (bytes + f.rw.w.msg_len - 1) / f.rw.w.msg_len;
    int idx = 0;
    while (bytes) {
        arq__msg_t *m = &f.rw.w.msg[(f.rw.w.seq + idx++) % f.rw.w.cap];
        m->len = arq__min(bytes, f.rw.w.msg_len);
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
    CHECK_EQUAL(0, f.rw.recv_ptr);
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

TEST(recv_wnd, recv_dst_two_full_messages_returns_two_times_msg_len)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len * 2);
    unsigned const recv_size = arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    CHECK_EQUAL(f.rw.w.msg_len * 2, recv_size);
}

TEST(recv_wnd, recv_full_msg_sets_recv_ptr_to_zero)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.rw.w.msg_len);
    CHECK_EQUAL(0, f.rw.recv_ptr);
}

TEST(recv_wnd, recv_partial_msg_sets_recv_ptr_to_message_offset_for_next_recv_call)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), 3);
    CHECK_EQUAL(3, f.rw.recv_ptr);
}

TEST(recv_wnd, recv_one_and_a_half_messages_leaves_recv_ptr_halfway_through_message)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len * 2);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.rw.w.msg_len + (f.rw.w.msg_len / 2));
    CHECK_EQUAL(1, f.rw.w.size);
    CHECK_EQUAL(f.rw.w.msg_len / 2, f.rw.recv_ptr);
}

TEST(recv_wnd, recv_two_and_a_half_messages_leaves_recv_ptr_halfway_through_third_message)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len * 3);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.rw.w.msg_len + (f.rw.w.msg_len / 2));
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.rw.w.msg_len);
    CHECK_EQUAL(1, f.rw.w.size);
    CHECK_EQUAL(f.rw.w.msg_len / 2, f.rw.recv_ptr);
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
    for (auto i = 0u; i < f.rw.w.msg_len * 2; ++i) {
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
    PopulateReceiveWindow(f, f.rw.w.msg_len * 2);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    MEMCMP_EQUAL(&f.buf[(f.rw.w.cap - 1) * f.rw.w.msg_len], f.recv.data(), f.rw.w.msg_len);
    MEMCMP_EQUAL(f.buf.data(), &f.recv[f.rw.w.msg_len], f.rw.w.msg_len);
}

TEST(recv_wnd, recv_slides_window_by_one_after_receiving_one_full_message)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len);
    CHECK_EQUAL(1, f.rw.w.size);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    CHECK_EQUAL(1, f.rw.w.seq);
    CHECK_EQUAL(0, f.rw.w.size);
}

TEST(recv_wnd, recv_resets_len_and_ack_vector_when_sliding)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len);
    CHECK_EQUAL(f.rw.w.msg_len, f.msg[0].len);
    CHECK_EQUAL(f.rw.w.full_ack_vec, f.msg[0].cur_ack_vec);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    CHECK_EQUAL(0, f.msg[0].len);
    CHECK_EQUAL(0, f.msg[0].cur_ack_vec);
}

TEST(recv_wnd, recv_resets_len_and_ack_vector_when_sliding_more_than_one_message)
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

TEST(recv_wnd, recv_slides_window_by_two_after_receiving_two_full_messages)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len * 2);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    CHECK_EQUAL(2, f.rw.w.seq);
    CHECK_EQUAL(0, f.rw.w.size);
}

TEST(recv_wnd, recv_slides_window_up_to_first_incomplete_message)
{
    Fixture f;
    PopulateReceiveWindow(f, (f.rw.w.msg_len * 2) + f.rw.w.seg_len);
    f.rw.w.msg[2].cur_ack_vec = 1;
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    CHECK_EQUAL(2, f.rw.w.seq);
    CHECK_EQUAL(1, f.rw.w.size);
}

TEST(recv_wnd, recv_slides_window_around_when_sequence_numbers_wrap)
{
    Fixture f;
    f.rw.w.seq = ARQ__FRAME_MAX_SEQ_NUM;
    PopulateReceiveWindow(f, f.rw.w.msg_len * 2);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), f.recv.size());
    CHECK_EQUAL(1, f.rw.w.seq);
    CHECK_EQUAL(0, f.rw.w.size);
}

TEST(recv_wnd, recv_doesnt_slide_if_dst_isnt_big_enough_to_hold_msg)
{
    Fixture f;
    PopulateReceiveWindow(f, f.rw.w.msg_len * 3);
    arq__recv_wnd_recv(&f.rw, f.recv.data(), 8);
    CHECK_EQUAL(0, f.rw.w.seq);
    CHECK_EQUAL(3, f.rw.w.size);
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

TEST(recv_wnd, next_ack_returns_negative_one_if_ack_vector_is_all_zeros)
{
    Fixture f;
    int const ack_seq = arq__recv_wnd_next_ack(&f.rw);
    CHECK_EQUAL(-1, ack_seq);
}

TEST(recv_wnd, next_ack_returns_ack_ptr_if_ack_vector_is_set)
{
    Fixture f;
    f.rw.ack_ptr = 2;
    f.rw.ack[f.rw.ack_ptr] = 1;
    int const actual = arq__recv_wnd_next_ack(&f.rw);
    CHECK_EQUAL(2, actual);
}

TEST(recv_wnd, next_ack_returns_ack_ptr_if_ack_vector_is_set_and_ack_ptr_greater_than_wnd_cap)
{
    Fixture f;
    f.rw.ack_ptr = 1234;
    f.rw.ack[f.rw.ack_ptr % f.rw.w.cap] = 1;
    int const actual = arq__recv_wnd_next_ack(&f.rw);
    CHECK_EQUAL(1234, actual);
}

TEST(recv_wnd, next_ack_sets_ack_vector_element_to_zero)
{
    Fixture f;
    f.rw.ack_ptr = 3;
    f.rw.ack[f.rw.ack_ptr] = 1;
    arq__recv_wnd_next_ack(&f.rw);
    CHECK_EQUAL(0, f.rw.ack[1234 % f.rw.w.cap]);
}

TEST(recv_wnd, next_ack_increments_ack_ptr)
{
    Fixture f;
    f.rw.ack_ptr = 2;
    f.rw.ack[f.rw.ack_ptr] = 1;
    arq__recv_wnd_next_ack(&f.rw);
    CHECK_EQUAL(3, f.rw.ack_ptr);
}

TEST(recv_wnd, next_ack_increments_ack_ptr_and_wraps_around_if_max_seq)
{
    Fixture f;
    f.rw.ack_ptr = ARQ__FRAME_MAX_SEQ_NUM;
    f.rw.ack[f.rw.ack_ptr % f.rw.w.cap] = 1;
    arq__recv_wnd_next_ack(&f.rw);
    CHECK_EQUAL(0, f.rw.ack_ptr);
}

TEST(recv_wnd, next_ack_skips_sequence_numbers_that_havent_been_received)
{
    Fixture f;
    f.rw.ack_ptr = 1;
    f.rw.w.cap = 5;
    f.rw.ack[1] = 0;
    f.rw.ack[2] = 0;
    f.rw.ack[3] = 1;
    arq__recv_wnd_next_ack(&f.rw);
    CHECK_EQUAL(4, f.rw.ack_ptr);
}

TEST(recv_wnd, next_ack_returns_dropped_ack_sequence_number)
{
    Fixture f;
    f.rw.w.cap = 8;
    f.rw.w.seq = 8;
    f.rw.ack_ptr = 4;
    int const ack_seq = arq__recv_wnd_next_ack(&f.rw);
    CHECK_EQUAL(4, ack_seq);
}

TEST(recv_wnd, next_ack_returns_dropped_ack_sequence_number_when_dropped_seq_wraps_around_seq_space)
{
    Fixture f;
    f.rw.w.cap = 8;
    f.rw.w.seq = 3;
    f.rw.ack_ptr = ARQ__FRAME_MAX_SEQ_NUM - 2;
    arq__recv_wnd_next_ack(&f.rw);
    CHECK_EQUAL(3, f.rw.ack_ptr);
}

TEST(recv_wnd, next_ack_sets_ptr_to_window_base_seq_from_dropped_ack_sequence_number)
{
    Fixture f;
    f.rw.w.cap = 8;
    f.rw.w.seq = 8;
    f.rw.ack_ptr = 3;
    arq__recv_wnd_next_ack(&f.rw);
    CHECK_EQUAL(8, f.rw.ack_ptr);
}

TEST(recv_wnd, next_ack_sets_ptr_to_window_base_seq_from_dropped_ack_seq_num_wrapped_around)
{
    Fixture f;
    f.rw.w.cap = 8;
    f.rw.w.seq = 6;
    f.rw.ack_ptr = ARQ__FRAME_MAX_SEQ_NUM;
    arq__recv_wnd_next_ack(&f.rw);
    CHECK_EQUAL(6, f.rw.ack_ptr);
}

}

