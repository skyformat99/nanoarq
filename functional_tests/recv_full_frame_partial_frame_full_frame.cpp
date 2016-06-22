#include "functional_tests.h"

namespace {

TEST(functional, recv_full_frame_partial_frame_full_frame)
{
    arq_cfg_t cfg;
    cfg.segment_length_in_bytes = 64;
    cfg.message_length_in_segments = 1;
    cfg.send_window_size_in_messages = 16;
    cfg.recv_window_size_in_messages = 16;
    cfg.retransmission_timeout = 100;
    cfg.checksum = &arq_crc32;
    cfg.connection_rst_attempts = 10;
    cfg.connection_rst_period = 100;

    ArqContext ctx(cfg);

    std::vector< arq_uchar_t > test_input, test_output;

    test_input.resize(cfg.segment_length_in_bytes * 2 + 1);
    for (auto i = 0u; i < test_input.size() / 2; ++i) {
        arq_uint16_t x = (arq_uint16_t)i;
        std::memcpy(&test_input[i * 2], &x, sizeof(x));
    }

    arq__frame_hdr_t h;
    arq__frame_hdr_init(&h);

    std::vector< arq_uchar_t > frame;
    frame.resize(arq__frame_len(cfg.segment_length_in_bytes));

    // load full message
    {
        arq__frame_hdr_init(&h);
        h.seg = ARQ_TRUE;
        h.seg_len = cfg.segment_length_in_bytes;
        h.msg_len = 1;
        unsigned const frame_len =
            arq__frame_write(&h, test_input.data(), cfg.checksum, frame.data(), frame.size());
        CHECK_EQUAL(arq__frame_len(cfg.segment_length_in_bytes), frame_len);
        unsigned recvd;
        arq_err_t e = arq_backend_recv_fill(ctx.arq, frame.data(), frame_len, &recvd);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(frame_len, recvd);
        arq_bool_t sr, rr;
        arq_time_t np;
        arq_event_t event;
        e = arq_backend_poll(ctx.arq, 0, &event, &sr, &rr, &np);
        CHECK(ARQ_SUCCEEDED(e) && sr && rr);
    }

    // load one byte
    {
        arq__frame_hdr_init(&h);
        h.seg = ARQ_TRUE;
        h.seq_num = 1;
        h.seg_len = 1;
        h.msg_len = 1;
        unsigned const frame_len = arq__frame_write(&h,
                                                    &test_input[cfg.segment_length_in_bytes],
                                                    cfg.checksum,
                                                    frame.data(),
                                                    frame.size());
        CHECK_EQUAL(arq__frame_len(1), frame_len);
        unsigned recvd;
        arq_err_t e = arq_backend_recv_fill(ctx.arq, frame.data(), frame_len, &recvd);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(frame_len, recvd);
        arq_bool_t sr, rr;
        arq_time_t np;
        arq_event_t event;
        e = arq_backend_poll(ctx.arq, 0, &event, &sr, &rr, &np);
        CHECK(ARQ_SUCCEEDED(e) && sr && rr);
    }

    // load full message
    {
        arq__frame_hdr_init(&h);
        h.seg = ARQ_TRUE;
        h.seq_num = 2;
        h.seg_len = cfg.segment_length_in_bytes;
        h.msg_len = 1;
        unsigned const frame_len = arq__frame_write(&h,
                                                    &test_input[cfg.segment_length_in_bytes + 1],
                                                    cfg.checksum,
                                                    frame.data(),
                                                    frame.size());
        CHECK_EQUAL(arq__frame_len(cfg.segment_length_in_bytes), frame_len);
        unsigned recvd;
        arq_err_t e = arq_backend_recv_fill(ctx.arq, frame.data(), frame_len, &recvd);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(frame_len, recvd);
        arq_bool_t sr, rr;
        arq_time_t np;
        arq_event_t event;
        e = arq_backend_poll(ctx.arq, 0, &event, &sr, &rr, &np);
        CHECK(ARQ_SUCCEEDED(e) && sr && rr);
    }

    // receive the full bit/small/big message trio
    {
        test_output.resize(test_input.size());
        unsigned recvd;
        arq_err_t e = arq_recv(ctx.arq, test_output.data(), test_output.size(), &recvd);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(test_input.size(), recvd);
        MEMCMP_EQUAL(test_input.data(), test_output.data(), test_input.size());
    }
}

}

