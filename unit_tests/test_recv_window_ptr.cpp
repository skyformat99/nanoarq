#include "arq_in_unit_tests.h"
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

TEST(recv_wnd_ptr, init_sets_pending_to_zero)
{
    arq__recv_wnd_ptr_t p;
    p.pending = 123;
    arq__recv_wnd_ptr_init(&p);
    CHECK_EQUAL(0, p.pending);
}

TEST(recv_wnd_ptr, init_sets_valid_to_zero)
{
    arq__recv_wnd_ptr_t p;
    p.valid = 123;
    arq__recv_wnd_ptr_init(&p);
    CHECK_EQUAL(0, p.valid);
}

struct Fixture
{
    Fixture()
    {
        arq__recv_wnd_ptr_init(&p);
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
    f.p.valid = 1;
    f.p.seq = (f.rw.w.seq + 1) % (ARQ__FRAME_MAX_SEQ_NUM + 1);
    f.rw.w.seq = 9;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(f.rw.w.seq, f.p.seq);
}

TEST(recv_wnd_ptr, next_sets_pending_to_zero_if_ack_vector_is_all_zeroes)
{
    Fixture f;
    f.p.seq = 0;
    f.p.pending = 1;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(0, f.p.pending);
}

TEST(recv_wnd_ptr, next_advances_to_first_nonzero_ack_entry)
{
    Fixture f;
    f.p.seq = 0;
    f.rw.ack[1] = 1;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(1, f.p.seq);
}

TEST(recv_wnd_ptr, next_sets_pending_to_1_upon_finding_nonzero_ack_entry)
{
    Fixture f;
    f.p.seq = 0;
    f.rw.ack[1] = 1;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(1, f.p.pending);
}

TEST(recv_wnd_ptr, next_sets_valid_to_1_upon_finding_nonzero_ack_entry)
{
    Fixture f;
    f.p.seq = 0;
    f.rw.ack[1] = 1;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(1, f.p.valid);
}

TEST(recv_wnd_ptr, next_sets_found_ack_element_to_zero)
{
    Fixture f;
    f.p.seq = 0;
    f.rw.ack[1] = 1;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(0, f.rw.ack[1]);
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

TEST(recv_wnd_ptr, next_increments_nonzero_ptr)
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

TEST(recv_wnd_ptr, next_starts_searching_for_ack_from_window_base_if_currently_out_of_bounds_before)
{
    Fixture f;
    f.rw.w.seq = 8;
    f.rw.ack[8] = 1;
    f.rw.ack[7] = 1;
    f.p.seq = 6;
    f.p.valid = 1;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(8, f.p.seq);
}

TEST(recv_wnd_ptr, next_starts_searching_for_ack_from_window_base_if_currently_out_of_bounds_after)
{
    Fixture f;
    f.rw.w.cap = 8;
    f.rw.w.seq = 3;
    f.p.seq = ARQ__FRAME_MAX_SEQ_NUM - 2;
    f.p.valid = 1;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(3, f.p.seq);
}

TEST(recv_wnd_ptr, next_sets_ptr_to_window_base_seq_from_dropped_ack_sequence_number_when_all_acks_zero)
{
    Fixture f;
    f.rw.w.seq = 8;
    f.rw.w.cap = 1;
    f.p.seq = 3;
    f.p.valid = 1;
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
    f.p.valid = 1;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(6, f.p.seq);
}

TEST(recv_wnd_ptr, next_sets_pending_to_0_when_starting_out_of_bounds_and_all_zero_ack_vector)
{
    Fixture f;
    f.rw.w.seq = 5;
    f.p.seq = 0;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(0, f.p.pending);
}

TEST(recv_wnd_ptr, next_sets_pending_to_1_when_starting_out_of_bounds_and_1_in_ack_vector)
{
    Fixture f;
    f.rw.w.seq = 5;
    f.rw.ack[5] = 1;
    f.p.seq = 0;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK_EQUAL(1, f.p.pending);
}

TEST(recv_wnd_ptr, next_sets_seq_to_base_when_ack_set_and_ptr_is_invalid)
{
    Fixture f;
    f.rw.w.size = 1;
    f.rw.ack[0] = 1;
    arq__recv_wnd_ptr_next(&f.p, &f.rw);
    CHECK(f.p.valid);
    CHECK_EQUAL(0, f.p.seq);
    CHECK(f.p.pending);
}

}

