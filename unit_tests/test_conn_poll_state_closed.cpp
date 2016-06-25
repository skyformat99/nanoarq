#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(conn_poll_state_closed) {};

namespace {

struct Fixture
{
    Fixture()
    {
        c.state = ARQ_CONN_STATE_CLOSED;
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
    arq_bool_t emit;
    arq__conn_state_ctx_t ctx;
    arq_event_t e = (arq_event_t)-1;
};

TEST(conn_poll_state_closed, doesnt_change_event)
{
    Fixture f;
    f.e = (arq_event_t)12345;
    arq__conn_poll_state_closed(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(12345, (int)f.e);
}

TEST(conn_poll_state_closed, transitions_to_rst_recvd_if_rst_arrives)
{
    Fixture f;
    f.rh.rst = ARQ_TRUE;
    arq__conn_poll_state_closed(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ_CONN_STATE_RST_RECVD, f.c.state);
}

TEST(conn_poll_state_closed, returns_stop_if_not_transitioning)
{
    Fixture f;
    arq__conn_state_next_t const next = arq__conn_poll_state_closed(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ__CONN_STATE_STOP, next);
}

TEST(conn_poll_state_closed, returns_continue_if_transitioning)
{
    Fixture f;
    f.rh.rst = ARQ_TRUE;
    arq__conn_state_next_t const next = arq__conn_poll_state_closed(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ__CONN_STATE_CONTINUE, next);
}

TEST(conn_poll_state_closed, raises_desync_event_if_ack_arrives)
{
    Fixture f;
    f.rh.ack = ARQ_TRUE;
    arq__conn_poll_state_closed(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ_CONN_STATE_CLOSED, f.c.state);
    CHECK_EQUAL(ARQ_EVENT_CONN_FAILED_DESYNC, f.e);
}

TEST(conn_poll_state_closed, raises_desync_event_if_seg_arrives)
{
    Fixture f;
    f.rh.seg = ARQ_TRUE;
    arq__conn_poll_state_closed(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ_CONN_STATE_CLOSED, f.c.state);
    CHECK_EQUAL(ARQ_EVENT_CONN_FAILED_DESYNC, f.e);
}

}

