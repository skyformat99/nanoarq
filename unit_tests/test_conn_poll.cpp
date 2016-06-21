#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(conn_poll) {};

namespace {

struct Fixture
{
    Fixture()
    {
        c.state = ARQ_CONN_STATE_CLOSED;
        cfg.connection_rst_period = 500;
        cfg.connection_rst_attempts = 8;
        arq__frame_hdr_init(&sh);
        arq__frame_hdr_init(&rh);
    }
    arq_conn_t c;
    arq_cfg_t cfg;
    arq__frame_hdr_t sh, rh;
};

struct RstSentFixture : Fixture
{
    RstSentFixture()
    {
        c.state = ARQ_CONN_STATE_RST_SENT;
        c.u.rst_sent.tmr = 0;
        c.u.rst_sent.cnt = 0;
    }
};

TEST(conn_poll, rst_sent_decrements_timer)
{
    RstSentFixture f;
    f.c.u.rst_sent.tmr = 123;
    arq__conn_poll(&f.c, &f.sh, &f.rh, 23, &f.cfg);
    CHECK_EQUAL(100, f.c.u.rst_sent.tmr);
}

TEST(conn_poll, rst_sent_returns_nothing_to_emit_if_timer_not_expired)
{
    RstSentFixture f;
    f.c.u.rst_sent.tmr = 1;
    arq_bool_t const emit = arq__conn_poll(&f.c, &f.sh, &f.rh, 0, &f.cfg);
    CHECK_EQUAL(ARQ_FALSE, emit);
}

TEST(conn_poll, rst_sent_sends_rst_if_timer_is_zero)
{
    RstSentFixture f;
    arq__conn_poll(&f.c, &f.sh, &f.rh, 23, &f.cfg);
    CHECK_EQUAL(ARQ_TRUE, f.sh.rst);
}

TEST(conn_poll, rst_sent_returns_emit_if_timer_expired_and_rst_written)
{
    RstSentFixture f;
    arq_bool_t const emit = arq__conn_poll(&f.c, &f.sh, &f.rh, 23, &f.cfg);
    CHECK_EQUAL(ARQ_TRUE, emit);
}

TEST(conn_poll, rst_sent_doesnt_emit_if_null_send_header)
{
    RstSentFixture f;
    arq_bool_t const emit = arq__conn_poll(&f.c, nullptr, &f.rh, 23, &f.cfg);
    CHECK_EQUAL(ARQ_FALSE, emit);
}

TEST(conn_poll, rst_sent_doesnt_reset_timer_if_expired_and_null_send_header)
{
    RstSentFixture f;
    arq__conn_poll(&f.c, nullptr, &f.rh, 23, &f.cfg);
    CHECK_EQUAL(0, f.c.u.rst_sent.tmr);
}

TEST(conn_poll, rst_sent_increments_count_when_timer_expires_and_can_send_rst)
{
    RstSentFixture f;
    f.c.u.rst_sent.cnt = 3;
    arq__conn_poll(&f.c, &f.sh, &f.rh, 23, &f.cfg);
    CHECK_EQUAL(4, f.c.u.rst_sent.cnt);
}

TEST(conn_poll, rst_sent_resets_timer_after_sending_rst)
{
    RstSentFixture f;
    arq__conn_poll(&f.c, &f.sh, &f.rh, 23, &f.cfg);
    CHECK_EQUAL(f.cfg.connection_rst_period, f.c.u.rst_sent.tmr);
}

}

