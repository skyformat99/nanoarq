#include "functional_tests.h"

namespace {

TEST(functional, send_10mb_through_window)
{
    arq_cfg_t cfg;
    cfg.segment_length_in_bytes = 220;
    cfg.message_length_in_segments = 4;
    cfg.send_window_size_in_messages = 16;
    cfg.recv_window_size_in_messages = 16;
    cfg.retransmission_timeout = 100;
    cfg.tinygram_send_delay = 10;
    cfg.checksum = &arq_crc32;

    ArqContext ctx(cfg);

    std::vector< arq_uchar_t > input_data(1024 * 1024 * 10);
    for (auto i = 0u; i < input_data.size() / 2; ++i) {
        uint16_t const v = i;
        std::memcpy(&input_data[i * 2], &v, sizeof(v));
    }
    auto input_idx = 0u;

    std::vector< arq_uchar_t > output_data;
    output_data.reserve(input_data.size());

    std::array< arq_uchar_t, 256 > frame;
    auto seq = 0u;

    while (output_data.size() < input_data.size()) {
        {
            unsigned sent;
            arq_err_t e = arq_send(ctx.arq, &input_data[input_idx], input_data.size() - input_idx, &sent);
            CHECK_EQUAL(ARQ_OK_COMPLETED, e);
            e = arq_flush(ctx.arq);
            CHECK_EQUAL(ARQ_OK_COMPLETED, e);
            input_idx += sent;
        }

        while (ctx.arq->send_wnd.w.size) {
            arq_uint16_t ack_vec = 0u;
            for (auto i = 0u; i < cfg.message_length_in_segments; ++i) {
                if (output_data.size() >= input_data.size()) {
                    break;
                }

                {
                    arq_bool_t s, r;
                    arq_event_t e;
                    arq_time_t next_poll;
                    arq_err_t const err = arq_backend_poll(ctx.arq, 0, &e, &s, &r, &next_poll);
                    CHECK_EQUAL(ARQ_OK_COMPLETED, err);
                }

                unsigned frame_len;
                {
                    void const *p;
                    {
                        arq_err_t const e = arq_backend_send_ptr_get(ctx.arq, &p, &frame_len);
                        CHECK_EQUAL(ARQ_OK_COMPLETED, e);
                        CHECK(p && frame_len > 0);
                    }
                    std::memcpy(frame.data(), p, frame_len);
                    if (frame_len) {
                        arq_err_t const e = arq_backend_send_ptr_release(ctx.arq);
                        CHECK_EQUAL(ARQ_OK_COMPLETED, e);
                    }
                    arq_bool_t s, r;
                    arq_event_t e;
                    arq_time_t next_poll;
                    arq_err_t const err = arq_backend_poll(ctx.arq, 0, &e, &s, &r, &next_poll);
                    CHECK_EQUAL(ARQ_OK_COMPLETED, err);
                }

                if (frame_len) {
                    arq_uchar_t const *seg;
                    arq__frame_hdr_t h;
                    arq__frame_read_result_t const r = arq__frame_read(frame.data(),
                                                                       frame_len,
                                                                       cfg.checksum,
                                                                       &h,
                                                                       (void const **)&seg);
                    CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, r);
                    std::copy(seg, seg + h.seg_len, std::back_inserter(output_data));
                    ack_vec |= 1 << h.seg_id;
                }
            }
            arq__send_wnd_ack(&ctx.arq->send_wnd, seq, ack_vec);
            seq = (seq + 1) % (ARQ__FRAME_MAX_SEQ_NUM + 1);
        }
    }
    CHECK_EQUAL(input_data.size(), output_data.size());
    MEMCMP_EQUAL(input_data.data(), output_data.data(), input_data.size());
}

}

