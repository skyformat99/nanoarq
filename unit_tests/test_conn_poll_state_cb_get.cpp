#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(conn_poll_state_cb_get) {};

namespace {

TEST(conn_poll_state_cb_get, closed_returns_closed)
{
    CHECK_EQUAL((void *)&arq__conn_poll_state_closed,
                (void *)arq__conn_poll_state_cb_get(ARQ_CONN_STATE_CLOSED));
}

TEST(conn_poll_state_cb_get, rst_sent_returns_rst_sent)
{
    CHECK_EQUAL((void *)&arq__conn_poll_state_rst_sent,
                (void *)arq__conn_poll_state_cb_get(ARQ_CONN_STATE_RST_SENT));
}

TEST(conn_poll_state_cb_get, rst_recvd_returns_rst_recvd)
{
    CHECK_EQUAL((void *)&arq__conn_poll_state_rst_recvd,
                (void *)arq__conn_poll_state_cb_get(ARQ_CONN_STATE_RST_RECVD));
}

TEST(conn_poll_state_cb_get, established_returns_established)
{
    CHECK_EQUAL((void *)&arq__conn_poll_state_established,
                (void *)arq__conn_poll_state_cb_get(ARQ_CONN_STATE_ESTABLISHED));
}

TEST(conn_poll_state_cb_get, unknown_enums_get_null)
{
    CHECK_EQUAL((void *)&arq__conn_poll_state_null,
                (void *)arq__conn_poll_state_cb_get((arq_conn_state_t)9000));
}

}

