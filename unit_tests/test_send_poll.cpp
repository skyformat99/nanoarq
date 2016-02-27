#include "nanoarq_in_test_project.h"
#include "nanoarq_hook_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(send_poll) {};

namespace
{

arq_uint32_t StubChecksum(void const *, int)
{
    return 0;
}

void MockSendWndStep(arq__send_wnd_t *w, arq_time_t dt)
{
    mock().actualCall("arq__send_wnd_step").withParameter("w", w).withParameter("dt", dt);
}

TEST(send_poll, calls_step_with_dt)
{
    arq__send_wnd_t w;
    arq__send_frame_t f;
    arq__send_wnd_ptr_t p;
    arq_time_t const dt = 1234;
    ARQ_MOCK_HOOK(arq__send_wnd_step, MockSendWndStep);
    mock().expectOneCall("arq__send_wnd_step").withParameter("w", &w).withParameter("dt", dt);
    arq__send_poll(&w, &p, &f, &StubChecksum, dt);
}

}

