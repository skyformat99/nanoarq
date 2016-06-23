#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(conn_poll_state_cb_get) {};

namespace {

TEST(conn_poll_state_cb_get, closed_returns_closed)
{
    CHECK_EQUAL(&arq__conn_poll_state_closed, arq__conn_poll_state_cb_get(ARQ_CONN_STATE_CLOSED));
}

TEST(conn_poll_state_cb_get, rst_sent_returns_rst_sent)
{
    CHECK_EQUAL(&arq__conn_poll_state_rst_sent, arq__conn_poll_state_cb_get(ARQ_CONN_STATE_RST_SENT));
}

TEST(conn_poll_state_cb_get, rst_recvd_returns_rst_recvd)
{
    CHECK_EQUAL(&arq__conn_poll_state_rst_recvd, arq__conn_poll_state_cb_get(ARQ_CONN_STATE_RST_RECVD));
}

TEST(conn_poll_state_cb_get, unknown_enums_get_null)
{
    CHECK_EQUAL(&arq__conn_poll_state_null, arq__conn_poll_state_cb_get((arq_conn_state_t)9000));
}

}

