#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(conn_poll_state_rst_sent) {};

namespace {

struct Fixture
{
    Fixture()
    {
        c.state = ARQ_CONN_STATE_RST_SENT;
        c.u.rst_sent.tmr = 0;
        c.u.rst_sent.cnt = 0;
        c.u.rst_sent.recvd_rst_ack = ARQ_FALSE;
        cfg.connection_rst_period = 500;
        cfg.connection_rst_attempts = 8;
        arq__frame_hdr_init(&sh);
        arq__frame_hdr_init(&rh);
        ctx.conn = &c;
        ctx.sh = &sh;
        ctx.rh = &rh;
        ctx.dt = 100;
        ctx.cfg = &cfg;
    }
    arq__conn_t c;
    arq_cfg_t cfg;
    arq__frame_hdr_t sh, rh;
    arq__conn_state_ctx_t ctx;
    arq_bool_t emit = ARQ_FALSE;
    arq_event_t e = (arq_event_t)-1;
};

TEST(conn_poll_state_rst_sent, decrements_timer)
{
    Fixture f;
    f.c.u.rst_sent.tmr = 123;
    f.ctx.dt = 23;
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(100, f.c.u.rst_sent.tmr);
}

TEST(conn_poll_state_rst_sent, no_event_if_nothing_happens)
{
    Fixture f;
    f.c.u.rst_sent.tmr = 100;
    f.e = (arq_event_t)1234;
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(1234, (int)f.e);
}

TEST(conn_poll_state_rst_sent, returns_nothing_to_emit_if_timer_not_expired)
{
    Fixture f;
    f.c.u.rst_sent.tmr = 1;
    f.ctx.dt = 0;
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ_FALSE, f.emit);
}

TEST(conn_poll_state_rst_sent, sends_rst_if_timer_is_zero)
{
    Fixture f;
    f.c.u.rst_sent.tmr = f.ctx.dt;
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ_TRUE, f.sh.rst);
}

TEST(conn_poll_state_rst_sent, returns_emit_if_timer_expired_and_rst_written)
{
    Fixture f;
    f.c.u.rst_sent.tmr = f.ctx.dt;
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ_TRUE, f.emit);
}

TEST(conn_poll_state_rst_sent, doesnt_emit_if_null_send_header)
{
    Fixture f;
    f.ctx.sh = nullptr;
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ_FALSE, f.emit);
}

TEST(conn_poll_state_rst_sent, doesnt_reset_timer_if_expired_and_null_send_header)
{
    Fixture f;
    f.c.u.rst_sent.tmr = f.ctx.dt;
    f.ctx.sh = nullptr;
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(0, f.c.u.rst_sent.tmr);
}

TEST(conn_poll_state_rst_sent, increments_count_when_timer_expires_and_can_send_rst)
{
    Fixture f;
    f.c.u.rst_sent.tmr = f.ctx.dt;
    f.c.u.rst_sent.cnt = 3;
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(4, f.c.u.rst_sent.cnt);
}

TEST(conn_poll_state_rst_sent, resets_timer_after_sending_rst)
{
    Fixture f;
    f.c.u.rst_sent.tmr = f.ctx.dt;
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(f.cfg.connection_rst_period, f.c.u.rst_sent.tmr);
}

TEST(conn_poll_state_rst_sent, event_connect_failed_if_max_attempts_timed_out)
{
    Fixture f;
    f.c.u.rst_sent.cnt = f.cfg.connection_rst_attempts;
    f.c.u.rst_sent.tmr = f.ctx.dt;
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ_EVENT_CONN_FAILED_NO_RESPONSE, f.e);
}

TEST(conn_poll_state_rst_sent, transitions_to_closed_if_connect_fails)
{
    Fixture f;
    f.c.u.rst_sent.cnt = f.cfg.connection_rst_attempts;
    f.c.u.rst_sent.tmr = f.ctx.dt;
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ_CONN_STATE_CLOSED, f.c.state);
}

TEST(conn_poll_state_rst_sent, returns_stop_when_not_transitioning)
{
    Fixture f;
    f.ctx.sh = nullptr;
    arq__conn_state_next_t const go = arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ__CONN_STATE_STOP, go);
}

TEST(conn_poll_state_rst_sent, returns_stop_when_transitioning_to_closed)
{
    Fixture f;
    f.c.u.rst_sent.cnt = f.cfg.connection_rst_attempts;
    f.c.u.rst_sent.tmr = f.ctx.dt;
    arq__conn_state_next_t const go = arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ__CONN_STATE_STOP, go);
}

TEST(conn_poll_state_rst_sent, transitions_to_close_and_raises_desync_if_ack_without_rst_arrives)
{
    Fixture f;
    f.rh.ack = ARQ_TRUE;
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ_CONN_STATE_CLOSED, f.c.state);
    CHECK_EQUAL(ARQ_EVENT_CONN_FAILED_DESYNC, f.e);
}

TEST(conn_poll_state_rst_sent, transitions_to_close_and_raises_desync_if_seg_arrives)
{
    Fixture f;
    f.rh.seg = ARQ_TRUE;
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ_CONN_STATE_CLOSED, f.c.state);
    CHECK_EQUAL(ARQ_EVENT_CONN_FAILED_DESYNC, f.e);
}

TEST(conn_poll_state_rst_sent, receiving_rst_ack_sets_recvd_to_true)
{
    Fixture f;
    f.rh.rst = ARQ_TRUE;
    f.rh.ack = ARQ_TRUE;
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK(f.c.u.rst_sent.recvd_rst_ack);
}

TEST(conn_poll_state_rst_sent, writes_ack_to_send_header_if_recvd_rst_ack_is_true)
{
    Fixture f;
    f.c.u.rst_sent.recvd_rst_ack = ARQ_TRUE;
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK(f.sh.ack);
}

TEST(conn_poll_state_rst_sent, sets_emit_flag_when_writing_ack)
{
    Fixture f;
    f.c.u.rst_sent.recvd_rst_ack = ARQ_TRUE;
    f.emit = ARQ_FALSE;
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ_TRUE, f.emit);
}

TEST(conn_poll_state_rst_sent, transitions_to_established_after_sending_ack)
{
    Fixture f;
    f.c.u.rst_sent.recvd_rst_ack = ARQ_TRUE;
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ_CONN_STATE_ESTABLISHED, f.c.state);
}

TEST(conn_poll_state_rst_sent, waits_for_non_null_send_header_to_send_ack_and_transition)
{
    Fixture f;
    f.ctx.sh = nullptr;
    f.c.u.rst_sent.recvd_rst_ack = ARQ_TRUE;
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ_CONN_STATE_RST_SENT, f.c.state);
    f.ctx.sh = &f.sh;
    arq__conn_poll_state_rst_sent(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ_CONN_STATE_ESTABLISHED, f.c.state);
}

}

