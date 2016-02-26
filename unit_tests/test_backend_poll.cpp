#include "nanoarq_in_test_project.h"
#include "nanoarq_hook_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(poll) {};

namespace
{

struct Fixture
{
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

int MockSendPoll(arq__send_wnd_t *w,
                 arq__send_wnd_ptr_t *p,
                 arq__send_frame_t *f,
                 arq_checksum_cb_t checksum_cb,
                 arq_time_t dt)
{
    return mock().actualCall("arq__send_poll").withParameter("w", w)
                                              .withParameter("p", p)
                                              .withParameter("f", f)
                                              .withParameter("checksum_cb", (void *)checksum_cb)
                                              .withParameter("dt", dt)
                                              .returnIntValue();
}

TEST(poll, calls_send_poll_with_arq_context)
{
    Fixture f;
    f.arq.cfg.checksum_cb = (arq_checksum_cb_t)0x12345678;
    int const dt = 1234;
    mock().expectOneCall("arq__send_poll").withParameter("w", &f.arq.send_wnd)
                                          .withParameter("p", &f.arq.send_wnd_ptr)
                                          .withParameter("f", &f.arq.send_frame)
                                          .withParameter("checksum_cb", (void *)f.arq.cfg.checksum_cb)
                                          .withParameter("dt", dt);
    ARQ_MOCK_HOOK(arq__send_poll, MockSendPoll);
    arq_backend_poll(&f.arq, dt, &f.send_size, &f.event, &f.time);
}

TEST(poll, writes_return_value_of_send_poll_to_send_size)
{
    Fixture f;
    mock().expectOneCall("arq__send_poll").ignoreOtherParameters().andReturnValue(1234);
    ARQ_MOCK_HOOK(arq__send_poll, MockSendPoll);
    arq_backend_poll(&f.arq, 0, &f.send_size, &f.event, &f.time);
    CHECK_EQUAL(1234, f.send_size);
}

}

