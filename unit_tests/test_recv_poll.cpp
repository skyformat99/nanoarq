#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <array>

TEST_GROUP(recv_poll) {};

namespace {

arq_uint32_t csum(void const *, int) { return 0; }

arq__frame_read_result_t MockFrameRead(void *frame,
                                       int frame_len,
                                       arq_checksum_t checksum,
                                       arq__frame_hdr_t *out_hdr,
                                       void const **out_seg)
{
    return (arq__frame_read_result_t)mock().actualCall("arq__frame_read")
                                           .withParameter("frame", frame)
                                           .withParameter("frame_len", frame_len)
                                           .withParameter("checksum", (void *)checksum)
                                           .withOutputParameter("out_hdr", out_hdr)
                                           .withOutputParameter("out_seg", (void *)out_seg)
                                           .returnIntValue();
}

void MockRecvFrameRst(arq__recv_frame_t *f)
{
    mock().actualCall("arq__recv_frame_rst").withParameter("f", f);
}

unsigned MockRecvWndFrame(arq__recv_wnd_t *rw,
                          unsigned seq,
                          unsigned seg,
                          unsigned seg_cnt,
                          void const *p,
                          unsigned len)
{
    return mock().actualCall("arq__recv_wnd_frame").withParameter("rw", rw)
                                                   .withParameter("seq", seq)
                                                   .withParameter("seg", seg)
                                                   .withParameter("seg_cnt", seg_cnt)
                                                   .withParameter("p", p)
                                                   .withParameter("len", len)
                                                   .returnUnsignedIntValue();
}

void MockRecvWndPtrNext(arq__recv_wnd_ptr_t *p, arq__recv_wnd_t const *rw)
{
    mock().actualCall("arq__recv_wnd_ptr_next").withParameter("p", p).withParameter("rw", rw);
}

struct Fixture
{
    Fixture()
    {
        ARQ_MOCK_HOOK(arq__recv_frame_rst, MockRecvFrameRst);
        ARQ_MOCK_HOOK(arq__frame_read, MockFrameRead);
        ARQ_MOCK_HOOK(arq__recv_wnd_frame, MockRecvWndFrame);
        ARQ_MOCK_HOOK(arq__recv_wnd_ptr_next, MockRecvWndPtrNext);
        arq.recv_frame.buf = &dummy;
        arq__frame_hdr_init(&sh);
        arq__frame_hdr_init(&rh);
    }
    arq_t arq;
    arq__frame_hdr_t sh, rh;
    arq_uchar_t dummy;
};

TEST(recv_poll, calls_frame_read_if_recv_frame_has_a_full_frame)
{
    Fixture f;
    f.arq.recv_frame.state = ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT;
    f.arq.recv_frame.len = 123;
    mock().expectOneCall("arq__frame_read").withParameter("frame", f.arq.recv_frame.buf)
                                           .withParameter("frame_len", f.arq.recv_frame.len)
                                           .withParameter("checksum", (void *)csum)
                                           .ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.arq.recv_wnd_ptr, csum, &f.sh, &f.rh);
}

TEST(recv_poll, resets_recv_frame_after_frame_read)
{
    Fixture f;
    f.arq.recv_frame.state = ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT;
    f.arq.recv_frame.len = 123;
    mock().expectOneCall("arq__frame_read").ignoreOtherParameters();
    mock().expectOneCall("arq__recv_frame_rst").withParameter("f", &f.arq.recv_frame);
    mock().ignoreOtherCalls();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.arq.recv_wnd_ptr, csum, &f.sh, &f.rh);
}

TEST(recv_poll, calls_recv_wnd_frame_if_frame_read_returns_success)
{
    Fixture f;
    f.arq.recv_frame.state = ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT;
    f.rh.seg = 1;
    f.rh.seq_num = 2;
    f.rh.seg_id = 3;
    f.rh.msg_len = 64;
    f.rh.seg_len = 16;
    mock().expectOneCall("arq__frame_read").withOutputParameterReturning("out_hdr", &f.rh, sizeof(f.rh))
                                           .ignoreOtherParameters()
                                           .andReturnValue(ARQ__FRAME_READ_RESULT_SUCCESS);
    mock().expectOneCall("arq__recv_wnd_frame").withParameter("rw", &f.arq.recv_wnd)
                                               .withParameter("seq", f.rh.seq_num)
                                               .withParameter("seg", f.rh.seg_id)
                                               .withParameter("seg_cnt", f.rh.msg_len)
                                               .withParameter("len", f.rh.seg_len)
                                               .ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.arq.recv_wnd_ptr, csum, &f.sh, &f.rh);
}

TEST(recv_poll, doesnt_call_recv_wnd_frame_if_frame_has_no_segment)
{
    Fixture f;
    f.arq.recv_frame.state = ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT;
    mock().expectOneCall("arq__frame_read").withOutputParameterReturning("out_hdr", &f.rh, sizeof(f.rh))
                                           .ignoreOtherParameters()
                                           .andReturnValue(ARQ__FRAME_READ_RESULT_SUCCESS);
    mock().expectOneCall("arq__recv_frame_rst").ignoreOtherParameters();
    mock().expectOneCall("arq__recv_wnd_ptr_next").ignoreOtherParameters();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.arq.recv_wnd_ptr, csum, &f.sh, &f.rh);
}

TEST(recv_poll, returns_zero_if_send_header_is_null)
{
    Fixture f;
    mock().ignoreOtherCalls();
    int const emit =
        arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.arq.recv_wnd_ptr, csum, nullptr, &f.rh);
    CHECK_EQUAL(0, emit);
}

TEST(recv_poll, returns_zero_if_recv_wnd_ptr_isnt_valid)
{
    Fixture f;
    f.arq.recv_wnd_ptr.valid = 0;
    mock().ignoreOtherCalls();
    int const emit =
        arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.arq.recv_wnd_ptr, csum, &f.sh, &f.rh);
    CHECK_EQUAL(0, emit);
}

TEST(recv_poll, returns_zero_if_recv_wnd_ptr_isnt_pending)
{
    Fixture f;
    f.arq.recv_wnd_ptr.valid = 1;
    f.arq.recv_wnd_ptr.pending = 0;
    mock().ignoreOtherCalls();
    int const emit =
        arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.arq.recv_wnd_ptr, csum, &f.sh, &f.rh);
    CHECK_EQUAL(0, emit);
}

TEST(recv_poll, returns_one_if_recv_wnd_ptr_is_pending)
{
    Fixture f;
    mock().expectOneCall("arq__frame_read").withParameter("frame", f.arq.recv_frame.buf)
                                           .withParameter("frame_len", f.arq.recv_frame.len)
                                           .ignoreOtherParameters()
                                           .andReturnValue(ARQ__FRAME_READ_RESULT_SUCCESS);
    mock().ignoreOtherCalls();
    f.arq.recv_wnd_ptr.valid = 1;
    f.arq.recv_wnd_ptr.pending = 1;
    f.arq.recv_frame.state = ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT;
    int const emit =
        arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.arq.recv_wnd_ptr, csum, &f.sh, &f.rh);
    CHECK_EQUAL(1, emit);
}

TEST(recv_poll, steps_receive_window_pointer_if_emitting_a_frame)
{
    Fixture f;
    f.arq.recv_wnd_ptr.valid = 1;
    f.arq.recv_wnd_ptr.pending = 1;
    f.arq.recv_frame.state = ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT;
    mock().expectOneCall("arq__recv_wnd_ptr_next").withParameter("p", &f.arq.recv_wnd_ptr)
                                                  .withParameter("rw", (void const *)&f.arq.recv_wnd);
    mock().ignoreOtherCalls();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.arq.recv_wnd_ptr, csum, &f.sh, &f.rh);
}

TEST(recv_poll, doesnt_step_receive_window_pointer_if_not_emitting_a_frame)
{
    Fixture f;
    mock().expectNoCall("arq__recv_wnd_ptr_next");
    mock().ignoreOtherCalls();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.arq.recv_wnd_ptr, csum, nullptr, &f.rh);
}

TEST(recv_poll, copies_ack_vec_from_ptr_if_valid)
{
    Fixture f;
    f.arq.recv_wnd_ptr.valid = 1;
    f.arq.recv_wnd_ptr.pending = 1;
    f.arq.recv_wnd_ptr.cur_ack_vec = 1234;
    mock().ignoreOtherCalls();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.arq.recv_wnd_ptr, csum, &f.sh, &f.rh);
    CHECK_EQUAL(1234, f.sh.cur_ack_vec);
}

TEST(recv_poll, sets_ack_flag_if_read_ptr_valid)
{
    Fixture f;
    f.arq.recv_wnd_ptr.valid = 1;
    f.arq.recv_wnd_ptr.pending = 1;
    mock().ignoreOtherCalls();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.arq.recv_wnd_ptr, csum, &f.sh, &f.rh);
    CHECK(f.sh.ack);
}

TEST(recv_poll, writes_ack_ptr_to_header)
{
    Fixture f;
    f.arq.recv_wnd_ptr.seq = 1234;
    f.arq.recv_wnd_ptr.valid = 1;
    f.arq.recv_wnd_ptr.pending = 1;
    mock().ignoreOtherCalls();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.arq.recv_wnd_ptr, csum, &f.sh, &f.rh);
    CHECK_EQUAL(1234, f.sh.ack_num);
}

}

