#include "nanoarq_in_test_project.h"
#include "nanoarq_hook_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(send_frame) {};

namespace {

TEST(send_frame, init_writes_cap_and_zeroes_len)
{
    arq__send_frame_t f;
    f.len = 1;
    arq__send_frame_init(&f, 123);
    CHECK_EQUAL(123, f.cap);
    CHECK_EQUAL(0, f.len);
}

TEST(send_frame, init_sets_state_to_free)
{
    arq__send_frame_t f;
    f.state = (arq__send_frame_state_t)100;
    arq__send_frame_init(&f, 123);
    CHECK_EQUAL(ARQ__SEND_FRAME_STATE_FREE, f.state);
}

}

