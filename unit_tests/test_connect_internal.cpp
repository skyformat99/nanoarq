#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(connect_intl) {};

namespace {

TEST(connect_intl, changes_state_to_rst_sent)
{
    arq_conn_t c;
    c.state = ARQ_CONN_STATE_CLOSED;
    arq__connect(&c);
    CHECK_EQUAL(ARQ_CONN_STATE_RST_SENT, c.state);
}

TEST(connect_intl, sets_cnt_to_zero)
{
    arq_conn_t c;
    c.state = ARQ_CONN_STATE_CLOSED;
    c.ctx.rst_sent.cnt = 123;
    arq__connect(&c);
    CHECK_EQUAL(0, c.ctx.rst_sent.cnt);
}

}

