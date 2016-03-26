#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <array>

TEST_GROUP(recv_poll) {};

namespace {

arq_uint32_t StubChecksum(void const *, int)
{
    return 0;
}

arq__frame_read_result_t MockFrameRead(void *frame,
                                       int frame_len,
                                       arq_checksum_cb_t checksum,
                                       arq__frame_hdr_t *out_hdr,
                                       void const **out_seg)
{
    return (arq__frame_read_result_t)mock().actualCall("arq__frame_read")
                                           .withParameter("frame", frame)
                                           .withParameter("frame_len", frame_len)
                                           .withParameter("checksum", (void *)checksum)
                                           .withOutputParameter("out_hdr", out_hdr)
                                           .withParameter("out_seg", (void const *)out_seg)
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
        arq__frame_hdr_init(&h);
    }
    arq_t arq;
    arq__frame_hdr_t h;
    arq_uchar_t dummy;
};

TEST(recv_poll, returns_zero_without_making_any_calls_if_recv_frame_doesnt_have_a_frame)
{
    Fixture f;
    f.arq.recv_frame.state = ARQ__RECV_FRAME_STATE_ACCUMULATING;
    f.arq.recv_wnd_ptr.valid = 0;
    int const recvd = arq__recv_poll(&f.arq.recv_wnd,
                                     &f.arq.recv_frame,
                                     &f.arq.recv_wnd_ptr,
                                     StubChecksum,
                                     &f.h);
    CHECK_EQUAL(0, recvd);
}

TEST(recv_poll, calls_frame_read_if_recv_frame_has_a_full_frame)
{
    Fixture f;
    f.arq.recv_frame.state = ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT;
    f.arq.recv_frame.len = 123;
    mock().expectOneCall("arq__frame_read").withParameter("frame", f.arq.recv_frame.buf)
                                           .withParameter("frame_len", f.arq.recv_frame.len)
                                           .withParameter("checksum", (void *)StubChecksum)
                                           .ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.arq.recv_wnd_ptr, StubChecksum, &f.h);
}

TEST(recv_poll, resets_recv_frame_after_frame_read)
{
    Fixture f;
    f.arq.recv_frame.state = ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT;
    f.arq.recv_frame.len = 123;
    mock().expectOneCall("arq__frame_read").ignoreOtherParameters();
    mock().expectOneCall("arq__recv_frame_rst").withParameter("f", &f.arq.recv_frame);
    mock().ignoreOtherCalls();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.arq.recv_wnd_ptr, StubChecksum, &f.h);
}

TEST(recv_poll, calls_recv_wnd_frame_if_frame_read_returns_success)
{
    Fixture f;
    f.arq.recv_frame.state = ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT;
    f.h.seq_num = 2;
    f.h.seg_id = 3;
    f.h.msg_len = 64;
    f.h.seg_len = 16;
    mock().expectOneCall("arq__frame_read").withOutputParameterReturning("out_hdr", &f.h, sizeof(f.h))
                                           .ignoreOtherParameters()
                                           .andReturnValue(ARQ__FRAME_READ_RESULT_SUCCESS);
    mock().expectOneCall("arq__recv_wnd_frame").withParameter("rw", &f.arq.recv_wnd)
                                               .withParameter("seq", f.h.seq_num)
                                               .withParameter("seg", f.h.seg_id)
                                               .withParameter("seg_cnt", f.h.msg_len)
                                               .withParameter("len", f.h.seg_len)
                                               .ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.arq.recv_wnd_ptr, StubChecksum, &f.h);
}

TEST(recv_poll, returns_zero_if_recv_wnd_ptr_isnt_valid)
{
    Fixture f;
    f.arq.recv_wnd_ptr.valid = 0;
    mock().ignoreOtherCalls();
    int const recvd = arq__recv_poll(&f.arq.recv_wnd,
                                     &f.arq.recv_frame,
                                     &f.arq.recv_wnd_ptr,
                                     StubChecksum,
                                     &f.h);
    CHECK_EQUAL(0, recvd);
}

TEST(recv_poll, returns_zero_if_recv_wnd_ptr_isnt_pending)
{
    Fixture f;
    f.arq.recv_wnd_ptr.valid = 1;
    f.arq.recv_wnd_ptr.pending = 0;
    mock().ignoreOtherCalls();
    int const recvd = arq__recv_poll(&f.arq.recv_wnd,
                                     &f.arq.recv_frame,
                                     &f.arq.recv_wnd_ptr,
                                     StubChecksum,
                                     &f.h);
    CHECK_EQUAL(0, recvd);
}

TEST(recv_poll, returns_one_if_recv_wnd_ptr_is_pending)
{
    Fixture f;
    f.arq.recv_frame.state = ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT;
    mock().expectOneCall("arq__frame_read").ignoreOtherParameters()
                                           .andReturnValue(ARQ__FRAME_READ_RESULT_SUCCESS);
    mock().ignoreOtherCalls();
    f.arq.recv_wnd_ptr.valid = 1;
    f.arq.recv_wnd_ptr.pending = 1;
    int const recvd = arq__recv_poll(&f.arq.recv_wnd,
                                     &f.arq.recv_frame,
                                     &f.arq.recv_wnd_ptr,
                                     StubChecksum,
                                     &f.h);
    CHECK_EQUAL(1, recvd);
}

TEST(recv_poll, copies_ack_vec_from_ptr_if_valid)
{
    Fixture f;
    f.arq.recv_wnd_ptr.valid = 1;
    f.arq.recv_wnd_ptr.pending = 1;
    f.arq.recv_wnd_ptr.cur_ack_vec = 1234;
    mock().ignoreOtherCalls();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.arq.recv_wnd_ptr, StubChecksum, &f.h);
    CHECK_EQUAL(1234, f.h.cur_ack_vec);
}

TEST(recv_poll, sets_ack_flag_if_read_ptr_valid)
{
    Fixture f;
    f.arq.recv_wnd_ptr.valid = 1;
    f.arq.recv_wnd_ptr.pending = 1;
    mock().ignoreOtherCalls();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.arq.recv_wnd_ptr, StubChecksum, &f.h);
    CHECK(f.h.ack);
}

TEST(recv_poll, writes_ack_ptr_to_header)
{
    Fixture f;
    f.arq.recv_wnd_ptr.seq = 1234;
    f.arq.recv_wnd_ptr.valid = 1;
    f.arq.recv_wnd_ptr.pending = 1;
    mock().ignoreOtherCalls();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.arq.recv_wnd_ptr, StubChecksum, &f.h);
    CHECK_EQUAL(1234, f.h.ack_num);
}

}

