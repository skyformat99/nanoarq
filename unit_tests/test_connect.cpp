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

TEST(connect, calls_arq__connect)
{
    struct Local
    {
        static void MockArqConnect(arq_conn_t *conn)
        {
            mock().actualCall("arq__connect").withParameter("conn", conn);
        }
    };

    ARQ_MOCK_HOOK(arq__connect, Local::MockArqConnect);

    arq_t arq;
    arq.conn.state = ARQ_CONN_STATE_CLOSED;
    mock().expectOneCall("arq__connect").withParameter("conn", &arq.conn);
    arq_connect(&arq);
}

TEST(connect, returns_success)
{
    arq_t arq;
    arq.conn.state = ARQ_CONN_STATE_CLOSED;
    arq_err_t const e = arq_connect(&arq);
    CHECK_EQUAL(ARQ_OK_POLL_REQUIRED, e);
}

}

