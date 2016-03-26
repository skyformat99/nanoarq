#include "functional_tests.h"

namespace {

TEST(functional, send_10mb_through_window)
{
    arq_cfg_t cfg;
    cfg.segment_length_in_bytes = 220;
    cfg.message_length_in_segments = 4;
    cfg.retransmission_timeout = 100;
    cfg.checksum_cb = &arq_crc32;

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
            int sent_this_turn;
            arq_err_t const e =
                arq_send(&ctx.arq, &input_data[input_idx], input_data.size() - input_idx, &sent_this_turn);
            CHECK_EQUAL(ARQ_OK_COMPLETED, e);
            input_idx += sent_this_turn;
        }

        while (ctx.arq.send_wnd.w.size) {
            for (auto i = 0; i < cfg.message_length_in_segments; ++i) {
                if (output_data.size() >= input_data.size()) {
                    break;
                }

                {
                    int unused;
                    arq_event_t event;
                    arq_time_t next_poll;
                    arq_err_t const e = arq_backend_poll(&ctx.arq, 0, &unused, &event, &next_poll);
                    CHECK_EQUAL(ARQ_OK_COMPLETED, e);
                }

                int frame_len;
                {
                    void const *p;
                    {
                        arq_err_t const e = arq_backend_send_ptr_get(&ctx.arq, &p, &frame_len);
                        CHECK_EQUAL(ARQ_OK_COMPLETED, e);
                        CHECK(p && frame_len > 0);
                    }
                    std::memcpy(frame.data(), p, frame_len);
                    if (frame_len) {
                        arq_err_t const e = arq_backend_send_ptr_release(&ctx.arq);
                        CHECK_EQUAL(ARQ_OK_COMPLETED, e);
                    }
                }

                {
                    int unused;
                    arq_event_t event;
                    arq_time_t next_poll;
                    arq_err_t const e = arq_backend_poll(&ctx.arq, 0, &unused, &event, &next_poll);
                    CHECK_EQUAL(ARQ_OK_COMPLETED, e);
                }

                if (frame_len) {
                    arq_uchar_t const *seg;
                    arq__frame_hdr_t h;
                    arq__frame_read_result_t const r = arq__frame_read(frame.data(),
                                                                       frame_len,
                                                                       cfg.checksum_cb,
                                                                       &h,
                                                                       (void const **)&seg);
                    CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, r);
                    std::copy(seg, seg + h.seg_len, std::back_inserter(output_data));
                }
            }
            arq__send_wnd_ack(&ctx.arq.send_wnd, seq, (1 << cfg.message_length_in_segments) - 1);
            seq = (seq + 1) % (ARQ__FRAME_MAX_SEQ_NUM + 1);
        }
    }
    CHECK_EQUAL(input_data.size(), output_data.size());
    MEMCMP_EQUAL(input_data.data(), output_data.data(), input_data.size());
}

}

