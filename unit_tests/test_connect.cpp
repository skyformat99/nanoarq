#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(connect) {};

namespace {

TEST(connect, invalid_params)
{
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_connect(nullptr));
}

TEST(connect, returns_err_not_disconnected_if_state_is_not_disconnected)
{
    arq_t arq;
    arq.conn.state = ARQ_CONN_STATE_ESTABLISHED;
    CHECK_EQUAL(ARQ_ERR_NOT_DISCONNECTED, arq_connect(&arq));
}

TEST(connect, changes_state_to_rst_sent)
{
    arq_t arq;
    arq.conn.state = ARQ_CONN_STATE_CLOSED;
    arq_connect(&arq);
    CHECK_EQUAL(ARQ_CONN_STATE_RST_SENT, arq.conn.state);
}

TEST(connect, sets_cnt_to_zero)
{
    arq_t arq;
    arq.conn.state = ARQ_CONN_STATE_CLOSED;
    arq.conn.u.rst_sent.cnt = 123;
    arq_connect(&arq);
    CHECK_EQUAL(0, arq.conn.u.rst_sent.cnt);
}

TEST(connect, sets_tmr_to_zero)
{
    arq_t arq;
    arq.conn.state = ARQ_CONN_STATE_CLOSED;
    arq.conn.u.rst_sent.tmr = 123;
    arq_connect(&arq);
    CHECK_EQUAL(0, arq.conn.u.rst_sent.tmr);
}

TEST(connect, returns_success)
{
    arq_t arq;
    arq.conn.state = ARQ_CONN_STATE_CLOSED;
    arq_err_t const e = arq_connect(&arq);
    CHECK_EQUAL(ARQ_OK_POLL_REQUIRED, e);
}

}

