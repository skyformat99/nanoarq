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

void MockSendWndAck(arq__send_wnd_t *sw, unsigned seq, arq_uint16_t cur_ack_vec)
{
    mock().actualCall("arq__send_wnd_ack").withParameter("sw", sw)
                                          .withParameter("seq", seq)
                                          .withParameter("cur_ack_vec", cur_ack_vec);
}

void MockSendWndFlush(arq__send_wnd_t *sw)
{
    mock().actualCall("arq__send_wnd_flush").withParameter("sw", sw);
}

struct Fixture
{
    Fixture()
    {
        arq__frame_hdr_init(&sh);
        arq__frame_hdr_init(&rh);
        sw.w.msg = m.data();
        sw.rtx = rtx.data();
        arq__wnd_init(&sw.w, m.size(), 128, 16);
        arq__send_frame_init(&f, 128);
        arq__send_wnd_ptr_init(&p);
        ARQ_MOCK_HOOK(arq__send_wnd_step, MockSendWndStep);
        ARQ_MOCK_HOOK(arq__send_wnd_ptr_next, MockSendWndPtrNext);
        ARQ_MOCK_HOOK(arq__send_wnd_ack, MockSendWndAck);
        ARQ_MOCK_HOOK(arq__send_wnd_flush, MockSendWndFlush);
    }

    arq_time_t const rtx_timeout = 37;
    arq__send_wnd_t sw;
    arq__send_frame_t f;
    arq__send_wnd_ptr_t p;
    arq__frame_hdr_t sh;
    arq__frame_hdr_t rh;
    std::array< arq__msg_t, 16 > m;
    std::array< arq_time_t, 16 > rtx;
};

TEST(send_poll, calls_ack_with_recv_header_fields_if_recv_header_contains_ack)
{
    Fixture f;
    f.rh.ack = 1;
    f.rh.ack_num = 123;
    f.rh.cur_ack_vec = 456;
    mock().expectOneCall("arq__send_wnd_ack").withParameter("sw", &f.sw)
                                             .withParameter("seq", f.rh.ack_num)
                                             .withParameter("cur_ack_vec", f.rh.cur_ack_vec);
    mock().ignoreOtherCalls();
    arq__send_poll(&f.sw, &f.f, &f.p, &f.sh, &f.rh, 10, f.rtx_timeout);
}

TEST(send_poll, calls_step_with_dt)
{
    Fixture f;
    arq_time_t const dt = 1234;
    mock().expectOneCall("arq__send_wnd_step").withParameter("sw", &f.sw).withParameter("dt", dt);
    mock().ignoreOtherCalls();
    arq__send_poll(&f.sw, &f.f, &f.p, &f.sh, &f.rh, dt, f.rtx_timeout);
}

TEST(send_poll, returns_zero_after_stepping_if_send_frame_in_use)
{
    Fixture f;
    f.p.valid = 1;
    f.f.state = ARQ__SEND_FRAME_STATE_HELD;
    mock().expectOneCall("arq__send_wnd_step").ignoreOtherParameters();
    int const emit = arq__send_poll(&f.sw, &f.f, &f.p, nullptr, &f.rh, 1, f.rtx_timeout);
    CHECK_EQUAL(0, emit);
}

struct ExpectSendWndStepFixture : Fixture
{
    ExpectSendWndStepFixture()
    {
        mock().expectOneCall("arq__send_wnd_step").ignoreOtherParameters();
    }
};

TEST(send_poll, flushes_send_window_if_tinygram_timer_enabled_and_expired)
{
    ExpectSendWndStepFixture f;
    f.sw.tiny_on = 1;
    f.sw.tiny = 0;
    mock().expectOneCall("arq__send_wnd_flush").withParameter("sw", &f.sw);
    mock().ignoreOtherCalls();
    arq__send_poll(&f.sw, &f.f, &f.p, &f.sh, &f.rh, 1, f.rtx_timeout);
}

TEST(send_poll, disables_tinygram_timer_if_tinygram_timer_enabled_and_expired)
{
    ExpectSendWndStepFixture f;
    f.sw.tiny_on = 1;
    f.sw.tiny = 0;
    mock().ignoreOtherCalls();
    arq__send_poll(&f.sw, &f.f, &f.p, &f.sh, &f.rh, 1, f.rtx_timeout);
    CHECK_EQUAL(0, f.sw.tiny_on);
}

TEST(send_poll, doesnt_flush_send_window_if_tinygram_timer_enabled_but_not_expired)
{
    ExpectSendWndStepFixture f;
    f.sw.tiny_on = 1;
    f.sw.tiny = 100;
    mock().expectNoCall("arq__send_wnd_flush");
    mock().ignoreOtherCalls();
    arq__send_poll(&f.sw, &f.f, &f.p, &f.sh, &f.rh, 1, f.rtx_timeout);
}

TEST(send_poll, doesnt_flush_send_window_if_tinygram_timer_disabled)
{
    ExpectSendWndStepFixture f;
    f.sw.tiny_on = 0;
    mock().expectNoCall("arq__send_wnd_flush");
    mock().ignoreOtherCalls();
    arq__send_poll(&f.sw, &f.f, &f.p, &f.sh, &f.rh, 1, f.rtx_timeout);
}

TEST(send_poll, advances_send_wnd_ptr_if_frame_is_available)
{
    ExpectSendWndStepFixture f;
    mock().expectOneCall("arq__send_wnd_ptr_next")
          .withParameter("p", &f.p)
          .withParameter("sw", (void const *)&f.sw);
    arq__send_poll(&f.sw, &f.f, &f.p, &f.sh, &f.rh, 10, f.rtx_timeout);
}

TEST(send_poll, doesnt_reset_message_retransmission_timer_if_havent_finished_message)
{
    ExpectSendWndStepFixture f;
    f.sw.rtx[0] = 1234;
    mock().expectOneCall("arq__send_wnd_ptr_next").ignoreOtherParameters().andReturnValue(0);
    arq__send_poll(&f.sw, &f.f, &f.p, &f.sh, &f.rh, 10, f.rtx_timeout);
    CHECK_EQUAL(1234, f.sw.rtx[0]);
}

TEST(send_poll, resets_message_retransmission_timer_if_finished_sending_message)
{
    ExpectSendWndStepFixture f;
    f.sw.rtx[0] = 0;
    mock().expectOneCall("arq__send_wnd_ptr_next").ignoreOtherParameters().andReturnValue(1);
    arq__send_poll(&f.sw, &f.f, &f.p, &f.sh, &f.rh, 10, f.rtx_timeout);
    CHECK_EQUAL(f.rtx_timeout, f.sw.rtx[0]);
}

struct ExpectSendWndPtrNextFixture : ExpectSendWndStepFixture
{
    ExpectSendWndPtrNextFixture()
    {
        mock().expectOneCall("arq__send_wnd_ptr_next").ignoreOtherParameters().andReturnValue(0);
    }
};

TEST(send_poll, returns_zero_if_no_new_data_to_send)
{
    ExpectSendWndPtrNextFixture f;
    int const emit = arq__send_poll(&f.sw, &f.f, &f.p, &f.sh, &f.rh, 10, f.rtx_timeout);
    CHECK_EQUAL(0, emit);
}

TEST(send_poll, sets_seg_flag_if_send_ptr_valid)
{
    ExpectSendWndPtrNextFixture f;
    f.p.valid = 1;
    f.f.state = ARQ__SEND_FRAME_STATE_RELEASED;
    mock().ignoreOtherCalls();
    arq__send_poll(&f.sw, &f.f, &f.p, &f.sh, &f.rh, 10, f.rtx_timeout);
    CHECK(f.sh.seg);
}

TEST(send_poll, sets_seq_num_from_send_ptr)
{
    ExpectSendWndPtrNextFixture f;
    f.p.valid = 1;
    f.f.state = ARQ__SEND_FRAME_STATE_RELEASED;
    f.p.seq = 123;
    mock().ignoreOtherCalls();
    arq__send_poll(&f.sw, &f.f, &f.p, &f.sh, &f.rh, 10, f.rtx_timeout);
    CHECK_EQUAL(123, f.sh.seq_num);
}

TEST(send_poll, sets_segment_from_send_ptr)
{
    Fixture f;
    f.p.valid = 1;
    f.f.state = ARQ__SEND_FRAME_STATE_RELEASED;
    f.p.seg = 3;
    mock().ignoreOtherCalls();
    arq__send_poll(&f.sw, &f.f, &f.p, &f.sh, &f.rh, 10, f.rtx_timeout);
    CHECK_EQUAL(3, f.sh.seg_id);
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
    arq__send_poll(&f.sw, &f.f, &f.p, &f.sh, &f.rh, 10, f.rtx_timeout);
    CHECK_EQUAL(10, f.sh.msg_len);
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
    arq__send_poll(&f.sw, &f.f, &f.p, &f.sh, &f.rh, 10, f.rtx_timeout);
    CHECK_EQUAL(1, f.sh.msg_len);
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
    arq__send_poll(&f.sw, &f.f, &f.p, &f.sh, &f.rh, 10, f.rtx_timeout);
    CHECK_EQUAL(1, f.sh.msg_len);
}

}

