#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(conn_next_poll) {};

namespace {

struct Fixture
{
    arq__conn_t conn;
};

TEST(conn_next_poll, returns_rst_sent_timer_if_in_rst_sent_state_and_smaller_than_other_timers)
{
    Fixture f;
    f.conn.state = ARQ_CONN_STATE_RST_SENT;
    f.conn.u.rst_sent.recvd_rst_ack = ARQ_FALSE;
    f.conn.u.rst_sent.simultaneous = ARQ_FALSE;
    f.conn.u.rst_sent.tmr = 7;
    arq_time_t const t = arq__conn_next_poll(&f.conn);
    CHECK_EQUAL(7, t);
}

TEST(conn_next_poll, ignores_rst_sent_timer_if_an_rst_ack_has_been_seen)
{
    Fixture f;
    f.conn.state = ARQ_CONN_STATE_RST_SENT;
    f.conn.u.rst_sent.recvd_rst_ack = ARQ_TRUE;
    f.conn.u.rst_sent.tmr = 7;
    arq_time_t const t = arq__conn_next_poll(&f.conn);
    CHECK_EQUAL(ARQ_TIME_INFINITY, t);
}

TEST(conn_next_poll, ignores_rst_sent_timer_if_an_rst_has_been_seen)
{
    Fixture f;
    f.conn.state = ARQ_CONN_STATE_RST_SENT;
    f.conn.u.rst_sent.recvd_rst_ack = ARQ_FALSE;
    f.conn.u.rst_sent.simultaneous = ARQ_TRUE;
    f.conn.u.rst_sent.tmr = 7;
    arq_time_t const t = arq__conn_next_poll(&f.conn);
    CHECK_EQUAL(ARQ_TIME_INFINITY, t);
}

TEST(conn_next_poll, ignores_rst_sent_timer_if_not_in_rst_sent_state)
{
    Fixture f;
    f.conn.u.rst_sent.tmr = 7;
    arq_time_t const t = arq__conn_next_poll(&f.conn);
    CHECK_EQUAL(ARQ_TIME_INFINITY, t);
}

TEST(conn_next_poll, returns_rst_recvd_timer_if_in_rst_recvd_state)
{
    Fixture f;
    f.conn.state = ARQ_CONN_STATE_RST_RECVD;
    f.conn.u.rst_recvd.tmr = 3;
    arq_time_t const t = arq__conn_next_poll(&f.conn);
    CHECK_EQUAL(3, t);
}

TEST(conn_next_poll, ignores_rst_recvd_timer_if_not_in_rst_recvd_state)
{
    Fixture f;
    f.conn.u.rst_recvd.tmr = 7;
    arq_time_t const t = arq__conn_next_poll(&f.conn);
    CHECK_EQUAL(ARQ_TIME_INFINITY, t);
}

}

