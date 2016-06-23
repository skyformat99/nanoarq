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
        cfg.connection_rst_period = 500;
        cfg.connection_rst_attempts = 8;
        arq__frame_hdr_init(&sh);
        arq__frame_hdr_init(&rh);
    }
    arq__conn_t c;
    arq_cfg_t cfg;
    arq__frame_hdr_t sh, rh;
    arq_event_t e = (arq_event_t)-1;
};

TEST(conn_poll_state_closed, returns_no_event_if_nothing_happens)
{
    Fixture f;
    arq__conn_poll(&f.c, &f.sh, &f.rh, 100, &f.cfg, &f.e);
    CHECK_EQUAL(ARQ_EVENT_NONE, f.e);
}

TEST(conn_poll_state_closed, transitions_to_rst_recvd_if_rst_arrives)
{
    Fixture f;
    f.rh.rst = ARQ_TRUE;
    arq__conn_poll(&f.c, &f.sh, &f.rh, 1, &f.cfg, &f.e);
    CHECK_EQUAL(ARQ_CONN_STATE_RST_RECVD, f.c.state);
}

}

