#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(poll) {};

namespace {

template< int SequenceNumber, int SegmentID, bool SegmentPresent >
int MockSendPoll(arq__send_wnd_t *sw,
                 arq__send_frame_t *f,
                 arq__send_wnd_ptr_t *p,
                 arq__frame_hdr_t *sh,
                 arq__frame_hdr_t *rh,
                 arq_time_t dt,
                 arq_time_t rtx)
{
    sh->seq_num = SequenceNumber;
    sh->seg_id = SegmentID;
    sh->seg = SegmentPresent;
    return mock().actualCall("arq__send_poll").withParameter("sw", sw)
                                              .withParameter("p", p)
                                              .withParameter("f", f)
                                              .withParameter("sh", sh)
                                              .withParameter("rh", rh)
                                              .withParameter("dt", dt)
                                              .withParameter("rtx", rtx)
                                              .returnIntValue();
}

int MockRecvPoll(arq__recv_wnd_t *rw,
                 arq__recv_frame_t *f,
                 arq__recv_wnd_ptr_t *p,
                 arq_checksum_cb_t checksum,
                 arq__frame_hdr_t *h)
{
    return mock().actualCall("arq__recv_poll").withParameter("rw", rw)
                                              .withParameter("p", p)
                                              .withParameter("f", f)
                                              .withParameter("h", h)
                                              .withParameter("checksum", (void *)checksum)
                                              .returnUnsignedIntValue();
}

void MockFrameHdrInit(arq__frame_hdr_t *h)
{
    mock().actualCall("arq__frame_hdr_init").withParameter("h", h);
}

int MockRecvWndPtrNext(arq__recv_wnd_ptr_t *p, arq__recv_wnd_t *rw)
{
    return mock().actualCall("arq__recv_wnd_ptr_next").withParameter("p", p)
                                                      .withParameter("rw", rw)
                                                      .returnIntValue();
}

int MockFrameWrite(arq__frame_hdr_t const *h,
                   void const *seg,
                   arq_checksum_cb_t checksum,
                   void *out_frame,
                   int frame_max)
{
    return mock().actualCall("arq__frame_write").withParameter("h", h)
                                                .withParameter("seg", seg)
                                                .withParameter("checksum", (void *)checksum)
                                                .withParameter("out_frame", out_frame)
                                                .withParameter("frame_max", frame_max)
                                                .returnIntValue();
}

void MockWndSeg(arq__wnd_t *w, unsigned seq, unsigned seg, void **out_seg, int *out_seg_len)
{
    mock().actualCall("arq__wnd_seg").withParameter("w", w)
                                     .withParameter("seg", seg)
                                     .withParameter("seq", seq)
                                     .withOutputParameter("out_seg", out_seg)
                                     .withOutputParameter("out_seg_len", out_seg_len);
}

struct Fixture
{
    Fixture()
    {
        ARQ_MOCK_HOOK(arq__frame_hdr_init, MockFrameHdrInit);
        ARQ_MOCK_HOOK(arq__recv_poll, MockRecvPoll);
        ARQ_MOCK_HOOK(arq__recv_wnd_ptr_next, MockRecvWndPtrNext);
        ARQ_MOCK_HOOK(arq__frame_write, MockFrameWrite);
        ARQ_MOCK_HOOK(arq__wnd_seg, MockWndSeg);
        arq.cfg.checksum_cb = &arq_crc32;
    }
    arq_t arq;
    int send_size;
    arq_event_t event;
    arq_time_t time;
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
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_poll(nullptr, 0, &f.send_size, &f.event,  &f.time));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_poll(&f.arq,  0, nullptr,      &f.event,  &f.time));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_poll(&f.arq,  0, &f.send_size, nullptr,   &f.time));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_poll(&f.arq,  0, &f.send_size, &f.event,  nullptr));
}

TEST(poll, initializes_frame_headers)
{
    DefaultMocksFixture f;
    mock().expectNCalls(2, "arq__frame_hdr_init").ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 1, &f.send_size, &f.event, &f.time);
}

TEST(poll, calls_recv_poll_with_arq_context)
{
    DefaultMocksFixture f;
    f.arq.cfg.checksum_cb = (arq_checksum_cb_t)0x12345678;
    mock().expectOneCall("arq__recv_poll").withParameter("rw", &f.arq.recv_wnd)
                                          .withParameter("f", &f.arq.recv_frame)
                                          .withParameter("p", &f.arq.recv_wnd_ptr)
                                          .withParameter("checksum", (void *)f.arq.cfg.checksum_cb)
                                          .ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 1, &f.send_size, &f.event, &f.time);
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
    arq_backend_poll(&f.arq, dt, &f.send_size, &f.event, &f.time);
}

TEST(poll, writes_send_frame_length_to_output_size_parameter)
{
    DefaultMocksFixture f;
    mock().ignoreOtherCalls();
    f.arq.send_frame.len = 1234;
    arq_backend_poll(&f.arq, 0, &f.send_size, &f.event, &f.time);
    CHECK_EQUAL(1234, f.send_size);
}

TEST(poll, loads_segment_from_send_window_if_header_segment_flag_is_set)
{
    Fixture f;
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
    arq_backend_poll(&f.arq, 0, &f.send_size, &f.event, &f.time);
}

TEST(poll, doesnt_load_segment_from_send_window_if_header_segment_flag_isnt_set)
{
    DefaultMocksFixture f;
    mock().expectNCalls(2, "arq__frame_hdr_init").ignoreOtherParameters();
    mock().expectOneCall("arq__recv_poll").ignoreOtherParameters();
    mock().expectOneCall("arq__send_poll").ignoreOtherParameters().andReturnValue(1);
    mock().expectOneCall("arq__frame_write").ignoreOtherParameters();
    mock().expectOneCall("arq__recv_wnd_ptr_next").ignoreOtherParameters();
    arq_backend_poll(&f.arq, 0, &f.send_size, &f.event, &f.time);
}

TEST(poll, emits_a_frame_if_send_poll_returns_one)
{
    DefaultMocksFixture f;
    f.arq.send_frame.cap = 123;
    mock().expectOneCall("arq__send_poll").ignoreOtherParameters().andReturnValue(1);
    mock().expectOneCall("arq__frame_write").withParameter("checksum", (void *)f.arq.cfg.checksum_cb)
                                            .withParameter("out_frame", (void *)f.arq.send_frame.buf)
                                            .withParameter("frame_max", f.arq.send_frame.cap)
                                            .ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.send_size, &f.event, &f.time);
}

TEST(poll, emits_a_frame_if_recv_poll_returns_one)
{
    DefaultMocksFixture f;
    mock().expectOneCall("arq__recv_poll").ignoreOtherParameters().andReturnValue(1);
    mock().expectOneCall("arq__frame_write").ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.send_size, &f.event, &f.time);
}

TEST(poll, frame_write_return_value_becomes_send_frame_length)
{
    DefaultMocksFixture f;
    f.arq.send_frame.cap = 123;
    mock().expectOneCall("arq__send_poll").ignoreOtherParameters().andReturnValue(1);
    mock().expectOneCall("arq__frame_write").withParameter("checksum", (void *)f.arq.cfg.checksum_cb)
                                            .withParameter("out_frame", (void *)f.arq.send_frame.buf)
                                            .withParameter("frame_max", f.arq.send_frame.cap)
                                            .ignoreOtherParameters()
                                            .andReturnValue(543);
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.send_size, &f.event, &f.time);
    CHECK_EQUAL(543, f.arq.send_frame.len);
}

TEST(poll, advances_recv_wnd_ptr_if_emitting_frame)
{
    DefaultMocksFixture f;
    f.arq.send_frame.cap = 123;
    mock().expectOneCall("arq__send_poll").ignoreOtherParameters().andReturnValue(1);
    mock().expectOneCall("arq__recv_wnd_ptr_next").withParameter("p", &f.arq.recv_wnd_ptr)
                                                  .withParameter("rw", &f.arq.recv_wnd);
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.send_size, &f.event, &f.time);
}

TEST(poll, returns_ok_completed)
{
    DefaultMocksFixture f;
    mock().ignoreOtherCalls();
    arq_err_t const e = arq_backend_poll(&f.arq, 0, &f.send_size, &f.event, &f.time);
    CHECK_EQUAL(ARQ_OK_COMPLETED, e);
}

}

