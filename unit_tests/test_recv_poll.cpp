#include "nanoarq_unit_test.h"
#include "nanoarq_hook_plugin.h"
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
                                           .withParameter("out_hdr", out_hdr)
                                           .withParameter("out_seg", (void const *)out_seg)
                                           .returnIntValue();
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

struct Fixture
{
    Fixture()
    {
        ARQ_MOCK_HOOK(arq__frame_read, MockFrameRead);
        ARQ_MOCK_HOOK(arq__recv_wnd_frame, MockRecvWndFrame);
        arq.recv_frame.buf = &dummy;
    }
    arq_t arq;
    arq__frame_hdr_t h;
    arq_uchar_t dummy;
};

TEST(recv_poll, returns_zero_without_making_any_calls_if_recv_frame_doesnt_have_a_frame)
{
    Fixture f;
    f.arq.recv_frame.state = ARQ__RECV_FRAME_STATE_ACCUMULATING;
    unsigned const recvd = arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.h, StubChecksum);
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
                                           .withParameter("out_hdr", &f.h)
                                           .ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.h, StubChecksum);
}

TEST(recv_poll, returns_zero_after_calling_frame_read_if_frame_read_returns_failure)
{
    Fixture f;
    f.arq.recv_frame.state = ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT;
    f.arq.recv_frame.len = 123;
    mock().expectOneCall("arq__frame_read").ignoreOtherParameters()
                                           .andReturnValue(ARQ__FRAME_READ_RESULT_ERR_CHECKSUM);
    unsigned const recvd = arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.h, StubChecksum);
    CHECK_EQUAL(0, recvd);
}

TEST(recv_poll, calls_recv_wnd_frame_if_frame_read_returns_success)
{
    Fixture f;
    f.arq.recv_frame.state = ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT;
    f.h.seq_num = 2;
    f.h.seg_id = 3;
    f.h.msg_len = 64;
    f.h.seg_len = 16;
    mock().expectOneCall("arq__frame_read").ignoreOtherParameters()
                                           .andReturnValue(ARQ__FRAME_READ_RESULT_SUCCESS);
    mock().expectOneCall("arq__recv_wnd_frame").withParameter("rw", &f.arq.recv_wnd)
                                               .withParameter("seq", f.h.seq_num)
                                               .withParameter("seg", f.h.seg_id)
                                               .withParameter("seg_cnt", f.h.msg_len)
                                               .withParameter("len", f.h.seg_len)
                                               .ignoreOtherParameters();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.h, StubChecksum);
}

TEST(recv_poll, returns_result_of_recv_wnd_frame_if_frame_read_returns_success)
{
    Fixture f;
    f.arq.recv_frame.state = ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT;
    mock().expectOneCall("arq__frame_read").ignoreOtherParameters()
                                           .andReturnValue(ARQ__FRAME_READ_RESULT_SUCCESS);
    mock().expectOneCall("arq__recv_wnd_frame").ignoreOtherParameters() .andReturnValue(1234);
    unsigned const recvd = arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, &f.h, StubChecksum);
    CHECK_EQUAL(1234, recvd);
}

}

