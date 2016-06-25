#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(conn_poll_state_rst_established) {};

namespace {

struct Fixture
{
    Fixture()
    {
        c.state = ARQ_CONN_STATE_ESTABLISHED;
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

TEST(conn_poll_state_rst_established, doesnt_emit_if_no_arriving_conn_flags)
{
    Fixture f;
    arq__conn_poll_state_established(&f.ctx, &f.emit, &f.e);
    CHECK_EQUAL(ARQ_FALSE, f.emit);
}

}

