#include "nanoarq_unit_test.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <vector>
#include <array>

TEST_GROUP(recv_wnd_ptr) {};

namespace {

TEST(recv_wnd_ptr, init_sets_seq_to_zero)
{
    arq__recv_wnd_ptr_t p;
    p.seq = 1;
    arq__recv_wnd_ptr_init(&p);
    CHECK_EQUAL(0, p.seq);
}

TEST(recv_wnd_ptr, init_sets_ack_to_zero)
{
    arq__recv_wnd_ptr_t p;
    p.cur_ack_vec = 123;
    arq__recv_wnd_ptr_init(&p);
    CHECK_EQUAL(0, p.seq);
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
        std::fill(std::begin(ack), std::end(ack), 0);
    }

    arq__recv_wnd_t rw;
    arq__recv_wnd_ptr_t p;
    std::array< arq__msg_t, 64 > msg;
    std::array< arq_uchar_t, 64 > ack;
    std::vector< arq_uchar_t > buf;
    std::vector< arq_uchar_t > seg;
    std::vector< arq_uchar_t > recv;
};

TEST(recv_wnd_ptr, next_sets_seq_to_base_if_ack_vector_is_all_zeroes)
{
    Fixture f;
    f.p.seq = 0;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(0, f.p.seq);
}

TEST(recv_wnd_ptr, next_sets_seq_to_base_if_ack_vector_all_zeroes_and_ptr_not_base)
{
    Fixture f;
    f.p.seq = (f.rw.w.seq + 1) % (ARQ__FRAME_MAX_SEQ_NUM + 1);
    f.rw.w.seq = 9;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(f.rw.w.seq, f.p.seq);
}

TEST(recv_wnd_ptr, next_sets_ack_vector_element_to_zero)
{
    Fixture f;
    f.p.seq = 3;
    f.rw.ack[f.p.seq] = 1;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(0, f.rw.ack[1234 % f.rw.w.cap]);
}

TEST(recv_wnd_ptr, next_increments_ptr)
{
    Fixture f;
    f.p.seq = 2;
    f.rw.ack[f.p.seq + 1] = 1;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(3, f.p.seq);
}

TEST(recv_wnd_ptr, next_increments_ack_ptr_and_wraps_around_if_max_seq)
{
    Fixture f;
    f.p.seq = ARQ__FRAME_MAX_SEQ_NUM;
    f.rw.ack[0] = 1;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(0, f.p.seq);
}

TEST(recv_wnd_ptr, next_skips_sequence_numbers_that_havent_been_received)
{
    Fixture f;
    f.p.seq = 1;
    f.rw.w.cap = 5;
    f.rw.ack[1] = 0;
    f.rw.ack[2] = 0;
    f.rw.ack[3] = 1;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(3, f.p.seq);
}

TEST(recv_wnd_ptr, next_returns_dropped_ack_sequence_number)
{
    Fixture f;
    f.rw.w.cap = 8;
    f.rw.w.seq = 8;
    f.p.seq = 4;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    //CHECK_EQUAL(4, ack_seq);
}

TEST(recv_wnd_ptr, next_returns_dropped_ack_sequence_number_when_dropped_seq_wraps_around_seq_space)
{
    Fixture f;
    f.rw.w.cap = 8;
    f.rw.w.seq = 3;
    f.p.seq = ARQ__FRAME_MAX_SEQ_NUM - 2;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(3, f.p.seq);
}

TEST(recv_wnd_ptr, next_sets_ptr_to_window_base_seq_from_dropped_ack_sequence_number_when_all_acks_zero)
{
    Fixture f;
    f.rw.w.seq = 8;
    f.rw.w.cap = 1;
    f.p.seq = 3;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(8, f.p.seq);
}

TEST(recv_wnd_ptr, next_sets_ptr_to_first_unackd_item_in_wnd_from_dropped_sequence_number)
{
    Fixture f;
    f.rw.w.cap = 11;
    f.rw.w.seq = 5;
    f.rw.ack[10] = 1;
    f.p.seq = 3;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(10, f.p.seq);
}

TEST(recv_wnd_ptr, next_sets_ptr_to_window_base_seq_from_dropped_ack_seq_num_wrapped_around)
{
    Fixture f;
    f.rw.w.cap = 8;
    f.rw.w.seq = 6;
    f.p.seq = ARQ__FRAME_MAX_SEQ_NUM;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(6, f.p.seq);
}

}

