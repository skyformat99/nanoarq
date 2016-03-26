#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <array>

TEST_GROUP(send_poll) {};

namespace {

void MockSendWndStep(arq__send_wnd_t *sw, arq_time_t dt)
{
    mock().actualCall("arq__send_wnd_step").withParameter("sw", sw).withParameter("dt", dt);
}

arq__send_wnd_ptr_next_result_t MockSendWndPtrNext(arq__send_wnd_ptr_t *p, arq__send_wnd_t const *sw)
{
    return (arq__send_wnd_ptr_next_result_t)mock().actualCall("arq__send_wnd_ptr_next")
                                                  .withParameter("p", p)
                                                  .withParameter("sw", sw)
                                                  .returnIntValue();
}

struct Fixture
{
    Fixture()
    {
        arq__frame_hdr_init(&h);
        sw.w.msg = m.data();
        sw.rtx = rtx.data();
        arq__wnd_init(&sw.w, m.size(), 128, 16);
        arq__send_frame_init(&f, 128);
        arq__send_wnd_ptr_init(&p);
        ARQ_MOCK_HOOK(arq__send_wnd_step, MockSendWndStep);
        ARQ_MOCK_HOOK(arq__send_wnd_ptr_next, MockSendWndPtrNext);
    }

    arq_time_t const rtx_timeout = 37;
    arq__send_wnd_t sw;
    arq__send_frame_t f;
    arq__send_wnd_ptr_t p;
    arq__frame_hdr_t h;
    std::array< arq__msg_t, 16 > m;
    std::array< arq_time_t, 16 > rtx;
};

TEST(send_poll, calls_step_with_dt)
{
    Fixture f;
    arq_time_t const dt = 1234;
    mock().expectOneCall("arq__send_wnd_step").withParameter("sw", &f.sw).withParameter("dt", dt);
    mock().ignoreOtherCalls();
    arq__send_poll(&f.sw, &f.f, &f.p, &f.h, dt, f.rtx_timeout);
}

TEST(send_poll, returns_zero_after_stepping_if_send_ptr_valid_and_frame_not_released)
{
    Fixture f;
    f.p.valid = 1;
    f.f.state = ARQ__SEND_FRAME_STATE_HELD;
    mock().expectOneCall("arq__send_wnd_step").ignoreOtherParameters();
    int const emit = arq__send_poll(&f.sw, &f.f, &f.p, &f.h, 10, f.rtx_timeout);
    CHECK_EQUAL(0, emit);
}

TEST(send_poll, returns_zero_after_stepping_if_send_ptr_not_acquired_and_ptr_valid)
{
    Fixture f;
    f.p.valid = 1;
    f.f.state = ARQ__SEND_FRAME_STATE_FREE;
    mock().expectOneCall("arq__send_wnd_step").ignoreOtherParameters();
    int const written = arq__send_poll(&f.sw, &f.f, &f.p, &f.h, 10, f.rtx_timeout);
    CHECK_EQUAL(0, written);
}

struct ExpectSendWndStepFixture : Fixture
{
    ExpectSendWndStepFixture()
    {
        mock().expectOneCall("arq__send_wnd_step").ignoreOtherParameters();
    }
};

TEST(send_poll, advances_send_wnd_ptr_if_frame_is_ready)
{
    ExpectSendWndStepFixture f;
    mock().expectOneCall("arq__send_wnd_ptr_next")
          .withParameter("p", &f.p)
          .withParameter("sw", (void const *)&f.sw);
    arq__send_poll(&f.sw, &f.f, &f.p, &f.h, 1, f.rtx_timeout);
}

TEST(send_poll, doesnt_reset_message_retransmission_timer_if_havent_finished_message)
{
    ExpectSendWndStepFixture f;
    f.sw.rtx[0] = 1234;
    mock().expectOneCall("arq__send_wnd_ptr_next").ignoreOtherParameters().andReturnValue(0);
    arq__send_poll(&f.sw, &f.f, &f.p, &f.h, 1, f.rtx_timeout);
    CHECK_EQUAL(1234, f.sw.rtx[0]);
}

TEST(send_poll, resets_message_retransmission_timer_if_finished_sending_message)
{
    ExpectSendWndStepFixture f;
    f.sw.rtx[0] = 0;
    mock().expectOneCall("arq__send_wnd_ptr_next").ignoreOtherParameters().andReturnValue(1);
    arq__send_poll(&f.sw, &f.f, &f.p, &f.h, 1, f.rtx_timeout);
    CHECK_EQUAL(f.rtx_timeout, f.sw.rtx[0]);
}

struct ExpectSendWndPtrNextFixture : ExpectSendWndStepFixture
{
    ExpectSendWndPtrNextFixture()
    {
        mock().expectOneCall("arq__send_wnd_ptr_next").ignoreOtherParameters().andReturnValue(0);
    }
};

TEST(send_poll, resets_send_frame)
{
    ExpectSendWndPtrNextFixture f;
    f.f.len = 1;
    f.f.state = ARQ__SEND_FRAME_STATE_RELEASED;
    arq__send_poll(&f.sw, &f.f, &f.p, &f.h, 1, f.rtx_timeout);
    CHECK_EQUAL(0, f.f.len);
    CHECK_EQUAL(ARQ__SEND_FRAME_STATE_FREE, f.f.state);
}

TEST(send_poll, returns_zero_if_no_new_data_to_send)
{
    ExpectSendWndPtrNextFixture f;
    int const emit = arq__send_poll(&f.sw, &f.f, &f.p, &f.h, 1, f.rtx_timeout);
    CHECK_EQUAL(0, emit);
}

TEST(send_poll, zeroes_frame_length_if_frame_was_released)
{
    ExpectSendWndPtrNextFixture f;
    f.p.valid = 1;
    f.f.state = ARQ__SEND_FRAME_STATE_RELEASED;
    f.f.len = 123;
    mock().ignoreOtherCalls();
    arq__send_poll(&f.sw, &f.f, &f.p, &f.h, 1, f.rtx_timeout);
    CHECK_EQUAL(0, f.f.len);
}

TEST(send_poll, resets_frame_state_if_frame_was_released)
{
    ExpectSendWndPtrNextFixture f;
    f.p.valid = 1;
    f.f.state = ARQ__SEND_FRAME_STATE_RELEASED;
    mock().ignoreOtherCalls();
    arq__send_poll(&f.sw, &f.f, &f.p, &f.h, 1, f.rtx_timeout);
    CHECK_EQUAL(ARQ__SEND_FRAME_STATE_FREE, f.f.state);
}

TEST(send_poll, sets_seg_flag_if_send_ptr_valid)
{
    ExpectSendWndPtrNextFixture f;
    f.p.valid = 1;
    f.f.state = ARQ__SEND_FRAME_STATE_RELEASED;
    mock().ignoreOtherCalls();
    arq__send_poll(&f.sw, &f.f, &f.p, &f.h, 1, f.rtx_timeout);
    CHECK(f.h.seg);
}

TEST(send_poll, sets_seq_num_from_send_ptr)
{
    ExpectSendWndPtrNextFixture f;
    f.p.valid = 1;
    f.f.state = ARQ__SEND_FRAME_STATE_RELEASED;
    f.p.seq = 123;
    mock().ignoreOtherCalls();
    arq__send_poll(&f.sw, &f.f, &f.p, &f.h, 1, f.rtx_timeout);
    CHECK_EQUAL(123, f.h.seq_num);
}

TEST(send_poll, sets_segment_from_send_ptr)
{
    Fixture f;
    f.p.valid = 1;
    f.f.state = ARQ__SEND_FRAME_STATE_RELEASED;
    f.p.seg = 3;
    mock().ignoreOtherCalls();
    arq__send_poll(&f.sw, &f.f, &f.p, &f.h, 1, f.rtx_timeout);
    CHECK_EQUAL(3, f.h.seg_id);
}

TEST(send_poll, sets_msg_len_field_from_message_being_sent_message_is_longer_than_one_segment)
{
    Fixture f;
    f.p.seq = 0;
    f.p.seg = 0;
    f.p.valid = 1;
    f.f.state = ARQ__SEND_FRAME_STATE_RELEASED;
    f.sw.w.seq = 0;
    f.sw.w.seg_len = 100;
    f.sw.w.msg[0].len = 1000;
    mock().ignoreOtherCalls();
    arq__send_poll(&f.sw, &f.f, &f.p, &f.h, 1, f.rtx_timeout);
    CHECK_EQUAL(10, f.h.msg_len);
}

TEST(send_poll, sets_msg_len_field_from_message_being_sent_message_is_shorter_than_one_segment)
{
    Fixture f;
    f.p.seq = 0;
    f.p.seg = 0;
    f.p.valid = 1;
    f.f.state = ARQ__SEND_FRAME_STATE_RELEASED;
    f.sw.w.seq = 0;
    f.sw.w.seg_len = 100;
    f.sw.w.msg[0].len = 37;
    mock().ignoreOtherCalls();
    arq__send_poll(&f.sw, &f.f, &f.p, &f.h, 1, f.rtx_timeout);
    CHECK_EQUAL(1, f.h.msg_len);
}

TEST(send_poll, sets_msg_len_field_from_message_being_sent_message_is_exactly_one_segment)
{
    Fixture f;
    f.p.seq = 0;
    f.p.seg = 0;
    f.p.valid = 1;
    f.f.state = ARQ__SEND_FRAME_STATE_RELEASED;
    f.sw.w.seq = 0;
    f.sw.w.seg_len = 100;
    f.sw.w.msg[0].len = 100;
    mock().ignoreOtherCalls();
    arq__send_poll(&f.sw, &f.f, &f.p, &f.h, 1, f.rtx_timeout);
    CHECK_EQUAL(1, f.h.msg_len);
}

}

