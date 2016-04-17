#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <array>

TEST_GROUP(recv_poll) {};

namespace {

arq_uint32_t csum(void const *, unsigned) { return 0; }

arq__frame_read_result_t MockFrameRead(void *frame,
                                       unsigned frame_len,
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

arq_bool_t MockRecvWndAck(arq__recv_wnd_t const *rw, unsigned *out_ack_seq, arq_uint16_t* out_ack_vec)
{
    return (arq_bool_t)mock().actualCall("arq__recv_wnd_ack").withParameter("rw", rw)
                                                             .withOutputParameter("out_ack_seq", out_ack_seq)
                                                             .withOutputParameter("out_ack_vec", out_ack_vec)
                                                             .returnIntValue();
}

struct Fixture
{
    Fixture()
    {
        ARQ_MOCK_HOOK(arq__recv_frame_rst, MockRecvFrameRst);
        ARQ_MOCK_HOOK(arq__frame_read, MockFrameRead);
        ARQ_MOCK_HOOK(arq__recv_wnd_frame, MockRecvWndFrame);
        ARQ_MOCK_HOOK(arq__recv_wnd_ack, MockRecvWndAck);
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
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, csum, &f.sh, &f.rh);
}

TEST(recv_poll, resets_recv_frame_after_frame_read)
{
    Fixture f;
    f.arq.recv_frame.state = ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT;
    f.arq.recv_frame.len = 123;
    mock().expectOneCall("arq__frame_read").ignoreOtherParameters();
    mock().expectOneCall("arq__recv_frame_rst").withParameter("f", &f.arq.recv_frame);
    mock().ignoreOtherCalls();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, csum, &f.sh, &f.rh);
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
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, csum, &f.sh, &f.rh);
}

TEST(recv_poll, doesnt_call_recv_wnd_frame_if_frame_has_no_segment)
{
    Fixture f;
    f.arq.recv_frame.state = ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT;
    mock().expectOneCall("arq__frame_read").withOutputParameterReturning("out_hdr", &f.rh, sizeof(f.rh))
                                           .ignoreOtherParameters()
                                           .andReturnValue(ARQ__FRAME_READ_RESULT_SUCCESS);
    mock().expectOneCall("arq__recv_frame_rst").ignoreOtherParameters();
    mock().expectOneCall("arq__recv_wnd_ack").withParameter("rw", (void const *)&f.arq.recv_wnd)
                                             .ignoreOtherParameters();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, csum, &f.sh, &f.rh);
}

TEST(recv_poll, result_of_ack_is_written_into_send_header)
{
    Fixture f;
    unsigned ack_seq = 1234;
    arq_uint16_t ack_vec = 4321;
    mock().expectOneCall("arq__recv_wnd_ack")
          .withParameter("rw", (void const *)&f.arq.recv_wnd)
          .withOutputParameterReturning("out_ack_seq", &ack_seq, sizeof(ack_seq))
          .withOutputParameterReturning("out_ack_vec", &ack_vec, sizeof(ack_vec))
          .andReturnValue(1);
    mock().ignoreOtherCalls();
    arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, csum, &f.sh, &f.rh);
    CHECK_EQUAL(ack_seq, f.sh.ack_num);
    CHECK_EQUAL(ack_vec, f.sh.cur_ack_vec);
    CHECK(f.sh.ack);
}

TEST(recv_poll, returns_zero_if_send_header_is_null)
{
    Fixture f;
    mock().ignoreOtherCalls();
    int const emit = arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, csum, nullptr, &f.rh);
    CHECK_EQUAL(0, emit);
}

TEST(recv_poll, returns_one_if_ack_call_returns_one)
{
    Fixture f;
    mock().expectOneCall("arq__recv_wnd_ack").withParameter("rw", (void const *)&f.arq.recv_wnd)
                                             .ignoreOtherParameters()
                                             .andReturnValue(1);
    mock().ignoreOtherCalls();
    int const emit = arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, csum, &f.sh, &f.rh);
    CHECK_EQUAL(1, emit);
}

TEST(recv_poll, returns_zero_if_ack_call_returns_zero)
{
    Fixture f;
    mock().expectOneCall("arq__recv_wnd_ack").withParameter("rw", (void const *)&f.arq.recv_wnd)
                                             .ignoreOtherParameters()
                                             .andReturnValue(0);
    mock().ignoreOtherCalls();
    int const emit = arq__recv_poll(&f.arq.recv_wnd, &f.arq.recv_frame, csum, &f.sh, &f.rh);
    CHECK_EQUAL(0, emit);
}

}

