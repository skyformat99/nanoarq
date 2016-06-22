#include "functional_tests.h"

namespace {

TEST(functional, send_full_window)
{
    arq_cfg_t cfg;
    cfg.segment_length_in_bytes = 220;
    cfg.message_length_in_segments = 4;
    cfg.send_window_size_in_messages = 16;
    cfg.recv_window_size_in_messages = 16;
    cfg.retransmission_timeout = 100;
    cfg.checksum = &arq_crc32;
    cfg.connection_rst_attempts = 10;
    cfg.connection_rst_period = 100;
    ArqContext ctx(cfg);

    std::vector< arq_uchar_t > send_test_data(cfg.send_window_size_in_messages *
                                              cfg.message_length_in_segments *
                                              cfg.segment_length_in_bytes);
    for (auto i = 0u; i < send_test_data.size() / 2; ++i) {
        uint16_t const v = i;
        std::memcpy(&send_test_data[i * 2], &v, sizeof(v));
    }

    std::vector< arq_uchar_t > recv_test_data;
    recv_test_data.reserve(send_test_data.size());

    {
        unsigned sent;
        arq_err_t const e = arq_send(ctx.arq, send_test_data.data(), send_test_data.size(), &sent);
        CHECK(ARQ_SUCCEEDED(e));
    }

    while (recv_test_data.size() < send_test_data.size()) {
        arq_uchar_t decode_buf[256];
        {
            arq_event_t event;
            arq_time_t next_poll;
            arq_bool_t send_pending, recv_pending;
            arq_err_t e = arq_backend_poll(ctx.arq, 0, &event, &send_pending, &recv_pending, &next_poll);
            CHECK(ARQ_SUCCEEDED(e));
            CHECK(send_pending);
        }

        unsigned size;
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
            recv_test_data.insert(std::end(recv_test_data),
                                  (arq_uchar_t const *)seg,
                                  (arq_uchar_t const *)seg + h.seg_len);
        }
    }
    CHECK_EQUAL(send_test_data.size(), recv_test_data.size());
    MEMCMP_EQUAL(send_test_data.data(), recv_test_data.data(), send_test_data.size());
}

}

