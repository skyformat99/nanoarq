#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <array>
#include <cstring>

TEST_GROUP(next_poll) {};

namespace {

arq_time_t MockConnNextPoll(arq__conn_t const *c)
{
    return (arq_time_t)mock().actualCall("arq__conn_next_poll")
                             .withParameter("c", (void const *)c)
                             .returnUnsignedIntValue();
}

struct Fixture
{
    Fixture()
    {
        sw.rtx = rtx.data();
        std::memset(rtx.data(), 0, sizeof(arq_time_t) * rtx.size());
        sw.w.cap = (arq_uint16_t)rtx.size();
        sw.w.size = 0;
        sw.w.seq = 0;
        sw.tiny_on = ARQ_FALSE;
        rw.inter_seg_ack_on = ARQ_FALSE;
        rw.inter_seg_ack_seq = 0;
        conn.state = ARQ_CONN_STATE_CLOSED;
        conn.u.rst_sent.tmr = ARQ_TIME_INFINITY;
    };
    arq__send_wnd_t sw;
    std::array< arq_time_t, 16 > rtx;
    arq__recv_wnd_t rw;
    arq__conn_t conn;
};

TEST(next_poll, returns_infinity_if_send_window_empty_and_tinygram_timer_off_and_ack_seg_timer_off)
{
    Fixture f;
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw, &f.conn);
    CHECK_EQUAL(ARQ_TIME_INFINITY, t);
}

TEST(next_poll, returns_only_rtx_timer)
{
    Fixture f;
    f.sw.w.size = 1;
    f.rtx[0] = 13;
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw, &f.conn);
    CHECK_EQUAL(13, t);
}

TEST(next_poll, returns_smallest_rtx_timer)
{
    Fixture f;
    f.sw.w.size = 14;
    for (auto i = 0u; i < f.sw.w.size; ++i) {
        f.rtx[i] = 14 - i;
    }
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw, &f.conn);
    CHECK_EQUAL(1, t);
}

TEST(next_poll, looks_at_rtx_timers_inside_of_active_send_window)
{
    Fixture f;
    f.sw.w.size = (arq_uint16_t)f.rtx.size();
    for (auto i = 0u; i < f.sw.w.size; ++i) {
        f.rtx[i] = 2;
    }
    f.sw.w.seq = 5;
    f.sw.w.size = 2;
    f.rtx[5] = 0;
    f.rtx[6] = 1;
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw, &f.conn);
    CHECK_EQUAL(1, t);
}

TEST(next_poll, doesnt_return_expired_rtx_timers)
{
    Fixture f;
    f.sw.w.size = 10;
    f.rtx[7] = 123;
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw, &f.conn);
    CHECK_EQUAL(123, t);
}

TEST(next_poll, returns_tinygram_expiry_time_if_tinygram_timer_on)
{
    Fixture f;
    f.sw.tiny_on = ARQ_TRUE;
    f.sw.tiny = 93;
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw, &f.conn);
    CHECK_EQUAL(93, t);
}

TEST(next_poll, returns_tinygram_if_smaller_than_rtx)
{
    Fixture f;
    f.sw.tiny_on = ARQ_TRUE;
    f.sw.tiny = 100;
    f.sw.w.size = 1;
    f.rtx[0] = 101;
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw, &f.conn);
    CHECK_EQUAL(100, t);
}

TEST(next_poll, returns_smallest_rtx_if_tinygram_timer_on_but_larger_than_rtx)
{
    Fixture f;
    f.sw.tiny_on = ARQ_TRUE;
    f.sw.tiny = 100;
    f.sw.w.size = 1;
    f.rtx[0] = 99;
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw, &f.conn);
    CHECK_EQUAL(99, t);
}

TEST(next_poll, returns_rtx_timer_if_smaller_than_ack_seg_timer)
{
    Fixture f;
    f.sw.w.size = 1;
    f.rtx[0] = 123;
    f.rw.inter_seg_ack_on = ARQ_TRUE;
    f.rw.inter_seg_ack = 200;
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw, &f.conn);
    CHECK_EQUAL(123, t);
}

TEST(next_poll, returns_ack_seg_timer_if_only_active_timer)
{
    Fixture f;
    f.rw.inter_seg_ack_on = ARQ_TRUE;
    f.rw.inter_seg_ack = 123;
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw, &f.conn);
    CHECK_EQUAL(123, t);
}

TEST(next_poll, returns_ack_seg_timer_if_smaller_than_tinygram_timer)
{
    Fixture f;
    f.rw.inter_seg_ack_on = ARQ_TRUE;
    f.rw.inter_seg_ack = 2;
    f.sw.tiny_on = ARQ_TRUE;
    f.sw.tiny = 333;
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw, &f.conn);
    CHECK_EQUAL(2, t);
}

TEST(next_poll, returns_ack_seg_timer_if_smaller_than_rtx_timer)
{
    Fixture f;
    f.rw.inter_seg_ack_on = ARQ_TRUE;
    f.rw.inter_seg_ack = 10;
    f.sw.w.size = 1;
    f.rtx[0] = 100;
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw, &f.conn);
    CHECK_EQUAL(10, t);
}

TEST(next_poll, calls_conn_next_poll)
{
    Fixture f;
    ARQ_MOCK_HOOK(arq__conn_next_poll, MockConnNextPoll);
    mock().expectOneCall("arq__conn_next_poll").withParameter("c", (void const *)&f.conn);
    arq__next_poll(&f.sw, &f.rw, &f.conn);
}

TEST(next_poll, returns_result_of_next_poll_if_smaller)
{
    Fixture f;
    ARQ_MOCK_HOOK(arq__conn_next_poll, MockConnNextPoll);
    mock().expectOneCall("arq__conn_next_poll").withParameter("c", (void const *)&f.conn)
                                               .andReturnValue(12345);
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw, &f.conn);
    CHECK_EQUAL(12345, t);
}

}

