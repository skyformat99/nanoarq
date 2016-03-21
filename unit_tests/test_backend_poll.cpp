#include "nanoarq_unit_test.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(poll) {};

namespace {

int MockSendPoll(arq__send_wnd_t *sw,
                 arq__send_wnd_ptr_t *p,
                 arq__send_frame_t *f,
                 arq__frame_hdr_t *h,
                 arq_checksum_cb_t checksum_cb,
                 arq_time_t rtx,
                 arq_time_t dt)
{
    return mock().actualCall("arq__send_poll").withParameter("sw", sw)
                                              .withParameter("p", p)
                                              .withParameter("f", f)
                                              .withParameter("h", h)
                                              .withParameter("checksum_cb", (void *)checksum_cb)
                                              .withParameter("rtx", rtx)
                                              .withParameter("dt", dt)
                                              .returnIntValue();
}

unsigned MockRecvPoll(arq__recv_wnd_t *rw,
                      arq__recv_wnd_ptr_t *p,
                      arq__recv_frame_t *f,
                      arq__frame_hdr_t *h,
                      arq_checksum_cb_t checksum)
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

struct Fixture
{
    Fixture()
    {
        ARQ_MOCK_HOOK(arq__frame_hdr_init, MockFrameHdrInit);
        ARQ_MOCK_HOOK(arq__send_poll, MockSendPoll);
        ARQ_MOCK_HOOK(arq__recv_poll, MockRecvPoll);
        ARQ_MOCK_HOOK(arq__recv_wnd_ptr_next, MockRecvWndPtrNext);
    }
    arq_t arq;
    int send_size;
    arq_event_t event;
    arq_time_t time;
};

TEST(poll, invalid_params)
{
    Fixture f;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_poll(nullptr, 0, &f.send_size, &f.event,  &f.time));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_poll(&f.arq,  0, nullptr,      &f.event,  &f.time));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_poll(&f.arq,  0, &f.send_size, nullptr,   &f.time));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_poll(&f.arq,  0, &f.send_size, &f.event,  nullptr));
}

TEST(poll, initializes_frame_header)
{
    Fixture f;
    mock().expectOneCall("arq__frame_hdr_init").ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 1, &f.send_size, &f.event, &f.time);
}

TEST(poll, calls_recv_poll_with_arq_context)
{
    Fixture f;
    f.arq.cfg.checksum_cb = (arq_checksum_cb_t)0x12345678;
    mock().expectOneCall("arq__recv_poll").withParameter("rw", &f.arq.recv_wnd)
                                          .withParameter("f", &f.arq.recv_frame)
                                          .withParameter("checksum", (void *)f.arq.cfg.checksum_cb)
                                          .ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 1, &f.send_size, &f.event, &f.time);
}

TEST(poll, calls_send_poll_with_arq_context)
{
    Fixture f;
    f.arq.cfg.checksum_cb = (arq_checksum_cb_t)0x12345678;
    f.arq.cfg.retransmission_timeout = 7654;
    int const dt = 1234;
    mock().expectOneCall("arq__send_poll").withParameter("sw", &f.arq.send_wnd)
                                          .withParameter("p", &f.arq.send_wnd_ptr)
                                          .withParameter("f", &f.arq.send_frame)
                                          .withParameter("checksum_cb", (void *)f.arq.cfg.checksum_cb)
                                          .withParameter("rtx", f.arq.cfg.retransmission_timeout)
                                          .withParameter("dt", dt)
                                          .ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, dt, &f.send_size, &f.event, &f.time);
}

TEST(poll, writes_return_value_of_send_poll_to_send_size)
{
    Fixture f;
    mock().expectOneCall("arq__send_poll").ignoreOtherParameters().andReturnValue(1234);
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.send_size, &f.event, &f.time);
    CHECK_EQUAL(1234, f.send_size);
}

TEST(poll, advances_recv_wnd_ack_if_send_poll_has_frame_to_send)
{
    Fixture f;
    mock().expectOneCall("arq__send_poll").ignoreOtherParameters().andReturnValue(32);
    mock().expectOneCall("arq__recv_wnd_ptr_next").withParameter("rw", &f.arq.recv_wnd)
                                                  .withParameter("p", &f.arq.recv_wnd_ptr);
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.send_size, &f.event, &f.time);
}

TEST(poll, does_not_advance_recv_wnd_ack_if_send_poll_has_nothing_to_send)
{
    Fixture f;
    mock().expectOneCall("arq__frame_hdr_init").ignoreOtherParameters();
    mock().expectOneCall("arq__recv_poll").ignoreOtherParameters();
    mock().expectOneCall("arq__send_poll").ignoreOtherParameters().andReturnValue(0);
    arq_backend_poll(&f.arq, 0, &f.send_size, &f.event, &f.time);
}

TEST(poll, calls_frame_init_then_recv_poll_then_send_poll)
{
    Fixture f;
    mock().expectOneCall("arq__frame_hdr_init").ignoreOtherParameters();
    mock().expectOneCall("arq__recv_poll").ignoreOtherParameters();
    mock().expectOneCall("arq__send_poll").ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_backend_poll(&f.arq, 0, &f.send_size, &f.event, &f.time);
}

}

