#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(poll) {};

namespace {

template< int SequenceNumber, int SegmentID, bool SegmentPresent >
arq_bool_t MockSendPoll(arq__send_wnd_t *sw,
                        arq__send_frame_t *f,
                        arq__send_wnd_ptr_t *p,
                        arq__frame_hdr_t *sh,
                        arq__frame_hdr_t *rh,
                        arq_time_t dt,
                        arq_time_t rtx)
{
    if (sh) {
        sh->seq_num = SequenceNumber;
        sh->seg_id = SegmentID;
        sh->seg = SegmentPresent;
    }
    return (arq_bool_t)mock().actualCall("arq__send_poll").withParameter("sw", sw)
                                                          .withParameter("p", p)
                                                          .withParameter("f", f)
                                                          .withParameter("sh", sh)
                                                          .withParameter("rh", rh)
                                                          .withParameter("dt", dt)
                                                          .withParameter("rtx", rtx)
                                                          .returnUnsignedIntValue();
}

arq_bool_t MockRecvPoll(arq__recv_wnd_t *rw,
                        arq__recv_frame_t *f,
                        arq_checksum_t checksum,
                        arq__frame_hdr_t *sh,
                        arq__frame_hdr_t *rh,
                        arq_time_t dt)
{
    return (arq_bool_t)mock().actualCall("arq__recv_poll").withParameter("rw", rw)
                                                          .withParameter("f", f)
                                                          .withParameter("sh", sh)
                                                          .withParameter("rh", rh)
                                                          .withParameter("checksum", (void *)checksum)
                                                          .withParameter("dt", dt)
                                                          .returnUnsignedIntValue();
}

arq_bool_t MockConnPoll(arq__conn_t *conn, arq__frame_hdr_t *sh, arq__frame_hdr_t const *rh, arq_time_t dt)
{
    return (arq_bool_t)mock().actualCall("arq__conn_poll").withParameter("conn", conn)
                                                          .withParameter("sh", sh)
                                                          .withParameter("rh", rh)
                                                          .withParameter("dt", dt)
                                                          .returnUnsignedIntValue();
}

void MockFrameHdrInit(arq__frame_hdr_t *h)
{
    mock().actualCall("arq__frame_hdr_init").withParameter("h", h);
}

arq_bool_t MockRecvWndPending(arq__recv_wnd_t *rw)
{
    return (arq_bool_t)mock().actualCall("arq__recv_wnd_pending").withParameter("rw", rw).returnIntValue();
}

unsigned MockFrameWrite(arq__frame_hdr_t const *h,
                   void const *seg,
                   arq_checksum_t checksum,
                   void *out_frame,
                   unsigned frame_max)
{
    return mock().actualCall("arq__frame_write").withParameter("h", h)
                                                .withParameter("seg", seg)
                                                .withParameter("checksum", (void *)checksum)
                                                .withParameter("out_frame", out_frame)
                                                .withParameter("frame_max", frame_max)
                                                .returnUnsignedIntValue();
}

void MockWndSeg(arq__wnd_t *w, unsigned seq, unsigned seg, void **out_seg, int *out_seg_len)
{
    mock().actualCall("arq__wnd_seg").withParameter("w", w)
                                     .withParameter("seg", seg)
                                     .withParameter("seq", seq)
                                     .withOutputParameter("out_seg", out_seg)
                                     .withOutputParameter("out_seg_len", out_seg_len);
}

arq_time_t MockNextPoll(arq__send_wnd_t *sw, arq__recv_wnd_t *rw)
{
    return (arq_time_t)mock().actualCall("arq__next_poll").withParameter("sw", sw)
                                                          .withParameter("rw", rw)
                                                          .returnUnsignedIntValue();
}

struct Fixture
{
    Fixture()
    {
        ARQ_MOCK_HOOK(arq__frame_hdr_init, MockFrameHdrInit);
        ARQ_MOCK_HOOK(arq__recv_poll, MockRecvPoll);
        ARQ_MOCK_HOOK(arq__recv_wnd_pending, MockRecvWndPending);
        ARQ_MOCK_HOOK(arq__frame_write, MockFrameWrite);
        ARQ_MOCK_HOOK(arq__wnd_seg, MockWndSeg);
        ARQ_MOCK_HOOK(arq__next_poll, MockNextPoll);
        ARQ_MOCK_HOOK(arq__conn_poll, MockConnPoll);
        arq.cfg.checksum = &arq_crc32;
        arq__send_frame_init(&arq.send_frame, 100);
    }
    arq_t arq;
    arq_bool_t send_ready = ARQ_FALSE;
    arq_bool_t recv_ready = ARQ_FALSE;
    arq_event_t event;
    arq_time_t time = 0;
};

struct DefaultMocksFixture : Fixture
{
    DefaultMocksFixture()
    {
        void *msp = reinterpret_cast< void * >(&MockSendPoll<0, 0, false>);
        ARQ_MOCK_HOOK(arq__send_poll, msp);
    }
};

TEST(poll, invalid_params)
{
    DefaultMocksFixture f;
    arq_err_t const e = ARQ_ERR_INVALID_PARAM;
    CHECK_EQUAL(e, arq_backend_poll(nullptr, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time));
    CHECK_EQUAL(e, arq_backend_poll(&f.arq,  0, nullptr,  &f.send_ready, &f.recv_ready, &f.time));
    CHECK_EQUAL(e, arq_backend_poll(&f.arq,  0, &f.event, nullptr,       &f.recv_ready, &f.time));
    CHECK_EQUAL(e, arq_backend_poll(&f.arq,  0, &f.event, &f.send_ready, nullptr,       &f.time));
    CHECK_EQUAL(e, arq_backend_poll(&f.arq,  0, &f.event, &f.send_ready, &f.recv_ready, nullptr));
}

TEST(poll, initializes_frame_headers)
{
    DefaultMocksFixture f;
    f.arq.send_frame.len = 0;
    f.arq.send_frame.state = ARQ__SEND_FRAME_STATE_FREE;
    mock().expectNCalls(2, "arq__frame_hdr_init").ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
}

TEST(poll, only_initializes_receive_header_if_unable_to_emit_frame_because_user_holding_it)
{
    DefaultMocksFixture f;
    f.arq.send_frame.state = ARQ__SEND_FRAME_STATE_HELD;
    mock().expectOneCall("arq__frame_hdr_init").ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
}

TEST(poll, only_initializes_receive_header_if_unable_to_emit_frame_because_frame_is_not_empty)
{
    DefaultMocksFixture f;
    f.arq.send_frame.len = 10;
    mock().expectOneCall("arq__frame_hdr_init").ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
}

TEST(poll, calls_recv_poll_with_arq_context)
{
    DefaultMocksFixture f;
    f.arq.cfg.checksum = (arq_checksum_t)0x12345678;
    mock().expectOneCall("arq__recv_poll").withParameter("rw", &f.arq.recv_wnd)
                                          .withParameter("f", &f.arq.recv_frame)
                                          .withParameter("checksum", (void *)f.arq.cfg.checksum)
                                          .withParameter("dt", f.time)
                                          .ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
}

TEST(poll, calls_recv_poll_with_null_send_header_if_unable_to_emit_frame_because_user_holding_it)
{
    DefaultMocksFixture f;
    f.arq.send_frame.state = ARQ__SEND_FRAME_STATE_HELD;
    mock().expectOneCall("arq__recv_poll").withParameter("sh", (void *)NULL).ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
}

TEST(poll, calls_recv_poll_with_null_send_header_if_unable_to_emit_frame_because_frame_is_not_empty)
{
    DefaultMocksFixture f;
    f.arq.send_frame.len = 1;
    mock().expectOneCall("arq__recv_poll").withParameter("sh", (void *)NULL).ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
}

TEST(poll, calls_send_poll_with_arq_context)
{
    DefaultMocksFixture f;
    f.arq.cfg.retransmission_timeout = 7654;
    int const dt = 1234;
    mock().expectOneCall("arq__send_poll").withParameter("sw", &f.arq.send_wnd)
                                          .withParameter("f", &f.arq.send_frame)
                                          .withParameter("p", &f.arq.send_wnd_ptr)
                                          .withParameter("dt", dt)
                                          .withParameter("rtx", f.arq.cfg.retransmission_timeout)
                                          .ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, dt, &f.event, &f.send_ready, &f.recv_ready, &f.time);
}

TEST(poll, calls_send_poll_with_null_send_header_if_unable_to_emit_frame)
{
    DefaultMocksFixture f;
    f.arq.send_frame.state = ARQ__SEND_FRAME_STATE_HELD;
    mock().expectOneCall("arq__send_poll").withParameter("sh", (void *)NULL).ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
}

TEST(poll, calls_conn_poll)
{
    DefaultMocksFixture f;
    arq_time_t const dt = 1532;
    mock().expectOneCall("arq__conn_poll").withParameter("conn", &f.arq.conn)
                                          .withParameter("dt", dt)
                                          .ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, dt, &f.event, &f.send_ready, &f.recv_ready, &f.time);
}

TEST(poll, calls_conn_poll_with_null_send_header_if_unable_to_emit_frame)
{
    DefaultMocksFixture f;
    f.arq.send_frame.state = ARQ__SEND_FRAME_STATE_HELD;
    mock().expectOneCall("arq__conn_poll").withParameter("sh", (void *)NULL).ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
}

TEST(poll, calls_conn_poll_after_send_poll_and_recv_poll)
{
    DefaultMocksFixture f;
    f.arq.send_frame.state = ARQ__SEND_FRAME_STATE_HELD;
    mock().expectOneCall("arq__recv_poll").ignoreOtherParameters();
    mock().expectOneCall("arq__send_poll").ignoreOtherParameters();
    mock().expectOneCall("arq__conn_poll").ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
}

TEST(poll, non_zero_send_frame_length_sets_send_ready_flag)
{
    DefaultMocksFixture f;
    mock().ignoreOtherCalls();
    f.arq.send_frame.len = 1234;
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
    CHECK(f.send_ready);
}

TEST(poll, loads_segment_from_send_window_if_header_segment_flag_is_set)
{
    Fixture f;
    f.arq.send_frame.len = 0;
    f.arq.send_frame.state = ARQ__SEND_FRAME_STATE_FREE;

    {
        void *msp = reinterpret_cast< void * >(&MockSendPoll<12, 34, true>);
        ARQ_MOCK_HOOK(arq__send_poll, msp);
    }

    int out_seg_len = 1234;
    mock().expectOneCall("arq__send_poll").ignoreOtherParameters().andReturnValue(1);
    mock().expectOneCall("arq__wnd_seg").withParameter("w", &f.arq.send_wnd)
                                        .withParameter("seq", 12)
                                        .withParameter("seg", 34)
                                        .withOutputParameterReturning("out_seg_len", &out_seg_len, sizeof(int))
                                        .ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
}

TEST(poll, doesnt_load_segment_from_send_window_if_header_segment_flag_isnt_set)
{
    DefaultMocksFixture f;
    mock().expectNoCall("arq__wnd_seg");
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
}

TEST(poll, emits_a_frame_if_send_poll_returns_one_and_send_frame_available)
{
    DefaultMocksFixture f;
    f.arq.send_frame.cap = 123;
    mock().expectOneCall("arq__send_poll").ignoreOtherParameters().andReturnValue(1);
    mock().expectOneCall("arq__frame_write").withParameter("checksum", (void *)f.arq.cfg.checksum)
                                            .withParameter("out_frame", (void *)f.arq.send_frame.buf)
                                            .withParameter("frame_max", f.arq.send_frame.cap)
                                            .ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
}

TEST(poll, emits_a_frame_if_recv_poll_returns_one_and_send_frame_available)
{
    DefaultMocksFixture f;
    f.arq.send_frame.len = 0;
    f.arq.send_frame.state = ARQ__SEND_FRAME_STATE_FREE;
    mock().expectOneCall("arq__recv_poll").ignoreOtherParameters().andReturnValue(1);
    mock().expectOneCall("arq__frame_write").ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
}

TEST(poll, frame_write_return_value_becomes_send_frame_length)
{
    DefaultMocksFixture f;
    f.arq.send_frame.cap = 123;
    mock().expectOneCall("arq__send_poll").ignoreOtherParameters().andReturnValue(1);
    mock().expectOneCall("arq__frame_write").withParameter("checksum", (void *)f.arq.cfg.checksum)
                                            .withParameter("out_frame", (void *)f.arq.send_frame.buf)
                                            .withParameter("frame_max", f.arq.send_frame.cap)
                                            .ignoreOtherParameters()
                                            .andReturnValue(543);
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
    CHECK_EQUAL(543, f.arq.send_frame.len);
}

TEST(poll, recv_pending_flag_is_result_of_recv_pending_call_true)
{
    DefaultMocksFixture f;
    mock().expectOneCall("arq__recv_wnd_pending").withParameter("rw", &f.arq.recv_wnd)
                                                 .andReturnValue(ARQ_TRUE);
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
    CHECK_EQUAL(ARQ_TRUE, f.recv_ready);
}

TEST(poll, recv_pending_flag_is_result_of_recv_pending_call_false)
{
    DefaultMocksFixture f;
    mock().expectOneCall("arq__recv_wnd_pending").withParameter("rw", &f.arq.recv_wnd)
                                                 .andReturnValue(ARQ_FALSE);
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
    CHECK_EQUAL(ARQ_FALSE, f.recv_ready);
}

TEST(poll, sets_frame_state_to_free_if_emitting_frame)
{
    DefaultMocksFixture f;
    f.arq.send_frame.state = ARQ__SEND_FRAME_STATE_RELEASED;
    mock().expectOneCall("arq__recv_poll").ignoreOtherParameters().andReturnValue(1);
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
    CHECK_EQUAL(ARQ__SEND_FRAME_STATE_FREE, f.arq.send_frame.state);
}

TEST(poll, out_next_poll_is_result_of_arq_poll)
{
    DefaultMocksFixture f;
    f.arq.send_frame.state = ARQ__SEND_FRAME_STATE_RELEASED;
    mock().expectOneCall("arq__recv_poll").ignoreOtherParameters().andReturnValue(1);
    mock().expectOneCall("arq__next_poll").withParameter("sw", &f.arq.send_wnd)
                                          .withParameter("rw", &f.arq.recv_wnd)
                                          .andReturnValue(1234u);
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
    CHECK_EQUAL(1234u, f.time);
}

TEST(poll, returns_ok_completed)
{
    DefaultMocksFixture f;
    mock().ignoreOtherCalls();
    arq_err_t const e = arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
    CHECK_EQUAL(ARQ_OK_COMPLETED, e);
}

TEST(poll, sets_need_poll_to_false)
{
    DefaultMocksFixture f;
    mock().ignoreOtherCalls();
    f.arq.need_poll = ARQ_TRUE;
    arq_backend_poll(&f.arq, 0, &f.event, &f.send_ready, &f.recv_ready, &f.time);
    CHECK_EQUAL(ARQ_FALSE, f.arq.need_poll);
}

}

