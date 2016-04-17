#include "functional_tests.h"

namespace {

TEST(functional, tiny_sends_accumulate_into_message)
{
    arq_cfg_t cfg;
    cfg.segment_length_in_bytes = 220;
    cfg.message_length_in_segments = 4;
    cfg.send_window_size_in_messages = 16;
    cfg.recv_window_size_in_messages = 16;
    cfg.retransmission_timeout = 1000;
    cfg.tinygram_send_delay = cfg.segment_length_in_bytes;
    cfg.checksum = &arq_crc32;
    ArqContext ctx(cfg);

    std::vector< arq_uchar_t > send_test_data(cfg.segment_length_in_bytes - 1);
    for (auto i = 0u; i < send_test_data.size() / 2; ++i) {
        arq_uint16_t x = (arq_uint16_t)i * 2;
        std::memcpy(&send_test_data[i * 2], &x, sizeof(x));
    }

    for (auto i = 0u; i < cfg.tinygram_send_delay - 1; ++i) {
        {
            unsigned sent;
            arq_err_t e = arq_send(ctx.arq, &send_test_data[i], 1, &sent);
            CHECK(ARQ_SUCCEEDED(e));
            CHECK_EQUAL(1, sent);
            arq_event_t event;
            arq_time_t next_poll;
            arq_bool_t send_pending, recv_pending;
            e = arq_backend_poll(ctx.arq, 1, &event, &send_pending, &recv_pending, &next_poll);
            CHECK(ARQ_SUCCEEDED(e) && !send_pending);
        }
    }

    {
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, recv_pending;
        arq_err_t const e = arq_backend_poll(ctx.arq, 1, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && send_pending);
    }

    unsigned size;
    arq_uchar_t decode_buf[256];
    {
        void const *p;
        arq_err_t e = arq_backend_send_ptr_get(ctx.arq, &p, &size);
        CHECK_EQUAL(ARQ_OK_COMPLETED, e);
        std::memcpy(decode_buf, p, size);
        e = arq_backend_send_ptr_release(ctx.arq);
        CHECK_EQUAL(ARQ_OK_COMPLETED, e);
    }

    {
        void const *seg;
        arq__frame_hdr_t h;
        arq__frame_read_result_t const r = arq__frame_read(decode_buf, size, cfg.checksum, &h, &seg);
        CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, r);
        MEMCMP_EQUAL(send_test_data.data(), seg, send_test_data.size());
    }
}

}

