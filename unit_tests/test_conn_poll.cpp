#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(conn_poll) {};

namespace {

static arq__conn_poll_state_cb_t MockConnPollStateCbGet(arq_conn_state_t state)
{
    return (arq__conn_poll_state_cb_t)mock().actualCall("arq__conn_poll_state_cb_get")
                                            .withParameter("state", state)
                                            .returnPointerValue();
}

struct Fixture
{
    Fixture()
    {
        c.state = ARQ_CONN_STATE_ESTABLISHED;
    }
    arq__conn_t c;
    arq__frame_hdr_t sh, rh;
    arq_cfg_t cfg;
    arq_event_t e;
};

TEST(conn_poll, calls_callback_with_parameters_in_context_struct)
{
    Fixture f;
    struct Local {
        static arq__conn_state_next_t UnpackCtx(arq__conn_state_ctx_t *ctx, arq_bool_t *, arq_event_t *)
        {
            return (arq__conn_state_next_t)mock().actualCall("MockStateCallback")
                                                 .withParameter("conn", ctx->conn)
                                                 .withParameter("sh", ctx->sh)
                                                 .withParameter("rh", (void const *)ctx->rh)
                                                 .withParameter("dt", ctx->dt)
                                                 .withParameter("cfg", (void const *)ctx->cfg)
                                                 .returnUnsignedIntValue();
        }
    };

    ARQ_MOCK_HOOK(arq__conn_poll_state_cb_get, MockConnPollStateCbGet);
    mock().expectOneCall("arq__conn_poll_state_cb_get").ignoreOtherParameters()
                                                       .andReturnValue((void *)Local::UnpackCtx);
    mock().expectOneCall("MockStateCallback").withParameter("conn", &f.c)
                                             .withParameter("sh", &f.sh)
                                             .withParameter("rh", (void const *)&f.rh)
                                             .withParameter("dt", 12345)
                                             .withParameter("cfg", (void const *)&f.cfg);
    arq__conn_poll(&f.c, &f.sh, &f.rh, 12345, &f.cfg, &f.e);
}

TEST(conn_poll, callback_out_event_parameter_is_written_to_conn_poll_event_parameter)
{
    Fixture f;
    struct Local {
        static arq__conn_state_next_t cb(arq__conn_state_ctx_t *, arq_bool_t *, arq_event_t *out_event)
        {
            *out_event = (arq_event_t)12345;
            return ARQ__CONN_STATE_STOP;
        }
    };

    ARQ_MOCK_HOOK(arq__conn_poll_state_cb_get, MockConnPollStateCbGet);
    mock().expectOneCall("arq__conn_poll_state_cb_get").ignoreOtherParameters()
                                                       .andReturnValue((void *)Local::cb);
    arq__conn_poll(&f.c, &f.sh, &f.rh, 0, &f.cfg, &f.e);
    CHECK_EQUAL(12345, (int)f.e);
}

TEST(conn_poll, returns_false_if_send_header_is_null)
{
    Fixture f;
    struct Local {
        static arq__conn_state_next_t cb(arq__conn_state_ctx_t *, arq_bool_t *out_emit, arq_event_t *)
        {
            *out_emit = ARQ_TRUE;
            return ARQ__CONN_STATE_STOP;
        }
    };
    arq_bool_t const emit = arq__conn_poll(&f.c, nullptr, &f.rh, 0, &f.cfg, &f.e);
    CHECK_EQUAL(ARQ_FALSE, emit);
}

TEST(conn_poll, returns_false_if_send_header_is_non_null_and_callback_doesnt_emit)
{
    Fixture f;
    struct Local {
        static arq__conn_state_next_t cb(arq__conn_state_ctx_t *, arq_bool_t *out_emit, arq_event_t *)
        {
            *out_emit = ARQ_FALSE;
            return ARQ__CONN_STATE_STOP;
        }
    };

    ARQ_MOCK_HOOK(arq__conn_poll_state_cb_get, MockConnPollStateCbGet);
    mock().expectOneCall("arq__conn_poll_state_cb_get").ignoreOtherParameters()
                                                       .andReturnValue((void *)Local::cb);
    arq_bool_t const emit = arq__conn_poll(&f.c, &f.sh, &f.rh, 0, &f.cfg, &f.e);
    CHECK_EQUAL(ARQ_FALSE, emit);
}

TEST(conn_poll, returns_true_if_send_header_is_non_null_and_callback_emits)
{
    Fixture f;
    struct Local {
        static arq__conn_state_next_t cb(arq__conn_state_ctx_t *, arq_bool_t *out_emit, arq_event_t *)
        {
            *out_emit = ARQ_TRUE;
            return ARQ__CONN_STATE_STOP;
        }
    };

    ARQ_MOCK_HOOK(arq__conn_poll_state_cb_get, MockConnPollStateCbGet);
    mock().expectOneCall("arq__conn_poll_state_cb_get").ignoreOtherParameters()
                                                       .andReturnValue((void *)Local::cb);
    arq_bool_t const emit = arq__conn_poll(&f.c, &f.sh, &f.rh, 0, &f.cfg, &f.e);
    CHECK_EQUAL(ARQ_TRUE, emit);
}

TEST(conn_poll, passes_current_state_to_cb_get)
{
    Fixture f;
    struct Local {
        static arq__conn_state_next_t cb(arq__conn_state_ctx_t *, arq_bool_t *, arq_event_t *)
        {
            return ARQ__CONN_STATE_STOP;
        }
    };

    ARQ_MOCK_HOOK(arq__conn_poll_state_cb_get, MockConnPollStateCbGet);
    f.c.state = ARQ_CONN_STATE_RST_RECVD;
    mock().expectOneCall("arq__conn_poll_state_cb_get").withParameter("state", f.c.state)
                                                       .andReturnValue((void *)Local::cb);
    arq__conn_poll(&f.c, &f.sh, &f.rh, 0, &f.cfg, &f.e);
}

TEST(conn_poll, only_calls_cb_once_if_cb_returns_STOP)
{
    Fixture f;
    struct Local {
        static arq__conn_state_next_t cb(arq__conn_state_ctx_t *, arq_bool_t *, arq_event_t *)
        {
            ++count();
            return ARQ__CONN_STATE_STOP;
        }

        static int& count()
        {
            static int s_count = 0;
            return s_count;
        }
    };

    ARQ_MOCK_HOOK(arq__conn_poll_state_cb_get, MockConnPollStateCbGet);
    mock().expectOneCall("arq__conn_poll_state_cb_get").ignoreOtherParameters()
                                                       .andReturnValue((void *)Local::cb);
    Local::count() = 0;
    arq__conn_poll(&f.c, &f.sh, &f.rh, 0, &f.cfg, &f.e);
    CHECK_EQUAL(1, Local::count());
}

TEST(conn_poll, calls_cb_repeatedly_until_cb_returns_STOP)
{
    Fixture f;
    struct Local {
        static arq__conn_state_next_t cb(arq__conn_state_ctx_t *, arq_bool_t *, arq_event_t *)
        {
            return (++count() < 10) ? ARQ__CONN_STATE_CONTINUE : ARQ__CONN_STATE_STOP;
        }

        static int& count()
        {
            static int s_count = 0;
            return s_count;
        }
    };

    ARQ_MOCK_HOOK(arq__conn_poll_state_cb_get, MockConnPollStateCbGet);
    mock().expectNCalls(10, "arq__conn_poll_state_cb_get").ignoreOtherParameters()
                                                          .andReturnValue((void *)Local::cb);
    Local::count() = 0;
    arq__conn_poll(&f.c, &f.sh, &f.rh, 0, &f.cfg, &f.e);
}

TEST(conn_poll, can_change_states_between_callbacks)
{
    Fixture f;
    struct Local {
        static arq__conn_state_next_t cb1(arq__conn_state_ctx_t *c, arq_bool_t *, arq_event_t *)
        {
            c->conn->state = (arq_conn_state_t)9000;
            return ARQ__CONN_STATE_CONTINUE;
        }

        static arq__conn_state_next_t cb2(arq__conn_state_ctx_t *, arq_bool_t *, arq_event_t *)
        {
            flag() = true;
            return ARQ__CONN_STATE_STOP;
        }

        static bool& flag() { static bool s_flag = false; return s_flag; }
    };

    ARQ_MOCK_HOOK(arq__conn_poll_state_cb_get, MockConnPollStateCbGet);
    f.c.state = ARQ_CONN_STATE_RST_RECVD;
    mock().expectOneCall("arq__conn_poll_state_cb_get").ignoreOtherParameters()
                                                       .andReturnValue((void *)Local::cb1);
    mock().expectOneCall("arq__conn_poll_state_cb_get").withParameter("state", 9000)
                                                       .andReturnValue((void *)Local::cb2);
    Local::flag() = false;
    arq__conn_poll(&f.c, &f.sh, &f.rh, 0, &f.cfg, &f.e);
    CHECK(Local::flag());
}

}

