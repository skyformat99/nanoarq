#include "functional_tests.h"

namespace {

TEST(functional, recv_10mb_through_window)
{
    arq_cfg_t cfg;
    cfg.segment_length_in_bytes = 220;
    cfg.message_length_in_segments = 4;
    cfg.send_window_size_in_messages = 16;
    cfg.recv_window_size_in_messages = 16;
    cfg.retransmission_timeout = 100;
    cfg.checksum = &arq_crc32;

    ArqContext ctx(cfg);

    std::vector< arq_uchar_t > test_input, test_output;

    test_input.resize(10 * 1024 * 1024);
    for (auto i = 0u; i < test_input.size() / 2; ++i) {
        arq_uint16_t x = (arq_uint16_t)i;
        std::memcpy(&test_input[i * 2], &x, sizeof(x));
    }

    arq__frame_hdr_t h;
    arq__frame_hdr_init(&h);
    h.msg_len = cfg.message_length_in_segments;

    std::vector< arq_uchar_t > frame;
    frame.reserve(arq__frame_len(cfg.segment_length_in_bytes));
    size_t test_input_offset = 0;
    while (test_output.size() < test_input.size()) {
        while (ctx.arq->recv_wnd.w.size < ctx.arq->recv_wnd.w.cap) {
            for (auto seg = 0u; seg < cfg.message_length_in_segments; ++seg) {
                h.seg = 1;
                h.seg_len = arq__min(cfg.segment_length_in_bytes, test_input.size() - test_input_offset);
                h.seg_id = seg;

                frame.resize(arq__frame_len(h.seg_len));

                int const frame_len = arq__frame_write(&h,
                                                       &test_input[test_input_offset],
                                                       arq_crc32,
                                                       frame.data(),
                                                       frame.size());
                CHECK(frame_len > 0);
                test_input_offset += h.seg_len;

                int bytes_pushed_into_window;
                {
                    arq_err_t const e =
                        arq_backend_recv_fill(ctx.arq, frame.data(), frame_len, &bytes_pushed_into_window);
                    CHECK(ARQ_SUCCEEDED(e));
                    CHECK_EQUAL(frame_len, bytes_pushed_into_window);
                }

                {
                    int send_len;
                    arq_event_t event;
                    arq_time_t next_poll;
                    arq_err_t const e = arq_backend_poll(ctx.arq, 0, &send_len, &event, &next_poll);
                    CHECK(ARQ_SUCCEEDED(e));
                }
            }
            h.seq_num = (h.seq_num + 1) % (ARQ__FRAME_MAX_SEQ_NUM + 1);
        }

        int bytes_recvd_from_window;
        while (ctx.arq->recv_wnd.w.size) {
            std::array< arq_uchar_t, 256 > recv;
            arq_err_t const e = arq_recv(ctx.arq, recv.data(), recv.size(), &bytes_recvd_from_window);
            CHECK(ARQ_SUCCEEDED(e));
            std::copy(&recv[0], &recv[bytes_recvd_from_window], std::back_inserter(test_output));
        }
    }
    CHECK_EQUAL(test_input.size(), test_output.size());
    MEMCMP_EQUAL(test_input.data(), test_output.data(), test_input.size());
}

}

