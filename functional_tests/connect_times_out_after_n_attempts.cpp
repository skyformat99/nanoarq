#include "functional_tests.h"

namespace {

TEST(functional, connect_times_out_after_n_attempts)
{
    arq_cfg_t cfg;
    cfg.segment_length_in_bytes = 220;
    cfg.message_length_in_segments = 4;
    cfg.send_window_size_in_messages = 16;
    cfg.recv_window_size_in_messages = 16;
    cfg.retransmission_timeout = 1000;
    cfg.tinygram_send_delay = cfg.segment_length_in_bytes;
    cfg.checksum = &arq_crc32;
    cfg.connection_rst_attempts = 10;
    cfg.connection_rst_period = 100;
    ArqContext ctx(cfg);

    // connect to nothing
    {
        arq_err_t const e = arq_connect(ctx.arq);
        CHECK(ARQ_SUCCEEDED(e));
    }

    arq_time_t const dt = cfg.connection_rst_period;
    for (auto i = 0u; i < cfg.connection_rst_attempts; ++i) {
        // poll to stimulate sending the rst frame
        {
            arq_event_t event;
            arq_time_t next_poll;
            arq_bool_t sp, rp;
            arq_err_t const e = arq_backend_poll(ctx.arq, dt, &event, &sp, &rp, &next_poll);
            CHECK(ARQ_SUCCEEDED(e) && sp);
        }

        // drain and discard the rst frame
        {
            void const *unused;
            unsigned size;
            arq_err_t e = arq_backend_send_ptr_get(ctx.arq, &unused, &size);
            CHECK(ARQ_SUCCEEDED(e));
            CHECK_EQUAL(arq__frame_len(0), size);
            e = arq_backend_send_ptr_release(ctx.arq);
            CHECK(ARQ_SUCCEEDED(e));
        }
    }

    // next poll will exceed the number of rst attempts and connect will fail
    {
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t sp, rp;
        arq_err_t const e = arq_backend_poll(ctx.arq, dt, &event, &sp, &rp, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && sp);
        CHECK_EQUAL(ARQ_EVENT_CONN_FAILED_NO_RESPONSE, event);
        CHECK_EQUAL(ARQ_CONN_STATE_CLOSED, ctx.arq->conn.state);
    }
}

}

