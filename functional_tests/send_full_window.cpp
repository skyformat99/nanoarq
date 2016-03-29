#include "functional_tests.h"

namespace {

TEST(functional, send_full_window)
{
    arq_cfg_t cfg;
    cfg.segment_length_in_bytes = 220;
    cfg.message_length_in_segments = 4;
    cfg.retransmission_timeout = 100;
    cfg.checksum = &arq_crc32;

    ArqContext ctx(cfg);

    std::vector< unsigned char > send_test_data(ctx.send_wnd_buf.size());
    for (auto i = 0u; i < send_test_data.size() / 2; ++i) {
        uint16_t const v = i;
        std::memcpy(&send_test_data[i * 2], &v, sizeof(v));
    }

    std::vector< unsigned char > recv_test_data;
    recv_test_data.reserve(send_test_data.size());

    {
        int sent;
        arq_err_t const e = arq_send(&ctx.arq, send_test_data.data(), send_test_data.size(), &sent);
        CHECK_EQUAL(ARQ_OK_COMPLETED, e);
    }

    while (recv_test_data.size() < send_test_data.size()) {
        unsigned char decode_buf[256];
        {
            arq_event_t event;
            arq_time_t next_poll;
            int bytes_to_drain;
            arq_err_t e = arq_backend_poll(&ctx.arq, 0, &bytes_to_drain, &event, &next_poll);
            CHECK_EQUAL(ARQ_OK_COMPLETED, e);
            CHECK(bytes_to_drain > 0);
        }

        int size;
        {
            void const *p;
            arq_err_t e = arq_backend_send_ptr_get(&ctx.arq, &p, &size);
            CHECK_EQUAL(ARQ_OK_COMPLETED, e);
            std::memcpy(decode_buf, p, size);
            e = arq_backend_send_ptr_release(&ctx.arq);
            CHECK_EQUAL(ARQ_OK_COMPLETED, e);
        }

        {
            void const *seg;
            arq__frame_hdr_t h;
            arq__frame_read_result_t const r =
                arq__frame_read(decode_buf, size, cfg.checksum, &h, &seg);
            CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, r);
            recv_test_data.insert(std::end(recv_test_data),
                                  (unsigned char const *)seg,
                                  (unsigned char const *)seg + h.seg_len);
        }
    }
    CHECK_EQUAL(send_test_data.size(), recv_test_data.size());
    MEMCMP_EQUAL(send_test_data.data(), recv_test_data.data(), send_test_data.size());
}

}

