#include "functional_tests.h"

namespace {

TEST(functional, recv_full_window_one_byte_at_a_time)
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

    std::vector< arq_uchar_t > test_input, test_output;

    test_input.resize(cfg.recv_window_size_in_messages *
                      cfg.message_length_in_segments *
                      cfg.segment_length_in_bytes);
    for (auto i = 0u; i < test_input.size() / 2; ++i) {
        arq_uint16_t x = (arq_uint16_t)i;
        std::memcpy(&test_input[i * 2], &x, sizeof(x));
    }

    arq__frame_hdr_t h;
    h.msg_len = cfg.message_length_in_segments;
    h.seg_id = 0;
    h.seq_num = 0;
    h.cur_ack_vec = 0;

    std::vector< arq_uchar_t > frame;
    frame.reserve(arq__frame_len(cfg.segment_length_in_bytes));

    size_t test_input_offset = 0;
    while (test_input_offset < test_input.size()) {
        h.seg = 1;
        h.seg_len = arq__min(cfg.segment_length_in_bytes, test_input.size() - test_input_offset);
        frame.resize(arq__frame_len(h.seg_len));

        unsigned const frame_len =
            arq__frame_write(&h, &test_input[test_input_offset], arq_crc32, frame.data(), frame.size());

        test_input_offset += h.seg_len;
        h.seq_num += (h.seg_id == cfg.message_length_in_segments - 1);
        h.seg_id = (h.seg_id + 1) % cfg.message_length_in_segments;

        {
            unsigned recvd;
            arq_err_t e = arq_backend_recv_fill(ctx.arq, frame.data(), frame_len, &recvd);
            CHECK(ARQ_SUCCEEDED(e));
            CHECK_EQUAL(frame_len, recvd);
            arq_event_t event;
            arq_time_t next_poll;
            arq_bool_t s, r;
            e = arq_backend_poll(ctx.arq, 0, &event, &s, &r, &next_poll);
            CHECK(ARQ_SUCCEEDED(e));
        }
    }
    CHECK_EQUAL(test_input.size(), test_input_offset);

    while (test_output.size() < test_input.size()) {
        arq_uchar_t byte;
        unsigned recvd;
        arq_err_t const e = arq_recv(ctx.arq, &byte, 1, &recvd);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(1, recvd);
        test_output.emplace_back(byte);
    }
    CHECK_EQUAL(test_input.size(), test_output.size());
    MEMCMP_EQUAL(test_input.data(), test_output.data(), test_input.size());
}

}


