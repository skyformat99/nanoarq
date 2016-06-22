#include "functional_tests.h"

namespace {

TEST(functional, poll_until_tinygram_sends)
{
    arq_cfg_t cfg;
    cfg.segment_length_in_bytes = 220;
    cfg.message_length_in_segments = 4;
    cfg.send_window_size_in_messages = 16;
    cfg.recv_window_size_in_messages = 16;
    cfg.retransmission_timeout = 100;
    cfg.tinygram_send_delay = 50;
    cfg.checksum = &arq_crc32;
    cfg.connection_rst_attempts = 10;
    cfg.connection_rst_period = 100;
    ArqContext ctx(cfg);

    arq_uchar_t const send_data = 0x5A;
    {
        unsigned sent;
        arq_err_t const e = arq_send(ctx.arq, &send_data, 1, &sent);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(1, sent);
    }

    for (auto i = 0u; i < cfg.tinygram_send_delay - 1; ++i) {
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, r;
        arq_err_t const e = arq_backend_poll(ctx.arq, 1, &event, &send_pending, &r, &next_poll);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(0, send_pending);
        CHECK_EQUAL(cfg.tinygram_send_delay - i - 1, next_poll);
    }

    {
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, r;
        arq_err_t const e = arq_backend_poll(ctx.arq, 1, &event, &send_pending, &r, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && send_pending);
    }

    unsigned size;
    arq_uchar_t decode_buf[256];
    {
        void const *p;
        arq_err_t e = arq_backend_send_ptr_get(ctx.arq, &p, &size);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(decode_buf, p, size);
        e = arq_backend_send_ptr_release(ctx.arq);
        CHECK(ARQ_SUCCEEDED(e));
    }

    {
        void const *seg;
        arq__frame_hdr_t h;
        arq__frame_read_result_t const r = arq__frame_read(decode_buf, size, cfg.checksum, &h, &seg);
        CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, r);
        CHECK_EQUAL(1, h.seg_len);
        arq_uchar_t actual;
        std::memcpy(&actual, seg, 1);
        CHECK_EQUAL(send_data, actual);
    }
}

}

