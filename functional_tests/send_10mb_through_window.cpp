#include "functional_tests.h"

namespace {

TEST(functional, send_10mb_through_window)
{
    arq_t arq;
    std::array< arq__msg_t, 4 > send_wnd_msgs;
    std::array< arq_time_t, 64 > rtx_timers;
    std::array< unsigned char, 256 > send_frame;
    std::vector< unsigned char > send_wnd_buf;
    std::vector< unsigned char > recv_wnd_buf;
    std::array< unsigned char, 256 > recv_frame;
    std::array< arq__msg_t, 4 > recv_wnd_msgs;

    arq.cfg.segment_length_in_bytes = 220;
    arq.cfg.message_length_in_segments = 4;
    arq.cfg.retransmission_timeout = 100;

    send_wnd_buf.resize(send_wnd_msgs.size() *
                        arq.cfg.segment_length_in_bytes *
                        arq.cfg.message_length_in_segments);

    recv_wnd_buf.resize(recv_wnd_msgs.size() *
                        arq.cfg.segment_length_in_bytes *
                        arq.cfg.message_length_in_segments);

    arq.cfg.checksum_cb = &arq_crc32;
    arq.send_wnd.w.msg = send_wnd_msgs.data();
    arq.send_wnd.w.buf = send_wnd_buf.data();
    arq.send_frame.buf = send_frame.data();
    arq.send_wnd.rtx = rtx_timers.data();

    arq.recv_wnd.w.msg = recv_wnd_msgs.data();
    arq.recv_wnd.w.buf = recv_wnd_buf.data();

    arq__wnd_init(&arq.send_wnd.w,
                  send_wnd_msgs.size(),
                  arq.cfg.message_length_in_segments * arq.cfg.segment_length_in_bytes,
                  arq.cfg.segment_length_in_bytes);
    arq__send_frame_init(&arq.send_frame, send_frame.size());
    arq__send_wnd_ptr_init(&arq.send_wnd_ptr);
    arq__wnd_init(&arq.recv_wnd.w,
                  recv_wnd_msgs.size(),
                  arq.cfg.message_length_in_segments * arq.cfg.segment_length_in_bytes,
                  arq.cfg.segment_length_in_bytes);
    arq__recv_frame_init(&arq.recv_frame, recv_frame.data(), recv_frame.size());
    arq__recv_wnd_rst(&arq.recv_wnd);

    std::vector< arq_uchar_t > input_data(7 * 1024);//1024 * 1024 * 10);
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
                arq_send(&arq, &input_data[input_idx], input_data.size() - input_idx, &sent_this_turn);
            CHECK_EQUAL(ARQ_OK_COMPLETED, e);
            input_idx += sent_this_turn;
        }

        while (arq.send_wnd.w.size) {
            for (auto i = 0; i < arq.cfg.message_length_in_segments; ++i) {
                {
                    int unused;
                    arq_event_t event;
                    arq_time_t next_poll;
                    arq_err_t const e = arq_backend_poll(&arq, 0, &unused, &event, &next_poll);
                    CHECK_EQUAL(ARQ_OK_COMPLETED, e);
                }

                int frame_len;
                {
                    void const *p;
                    {
                        arq_err_t const e = arq_backend_send_ptr_get(&arq, &p, &frame_len);
                        CHECK_EQUAL(ARQ_OK_COMPLETED, e);
                    }
                    std::memcpy(frame.data(), p, frame_len);
                    if (frame_len) {
                        arq_err_t const e = arq_backend_send_ptr_release(&arq);
                        CHECK_EQUAL(ARQ_OK_COMPLETED, e);
                    }
                }

                if (frame_len) {
                    arq_uchar_t const *seg;
                    arq__frame_hdr_t h;
                    arq__frame_read_result_t const r = arq__frame_read(frame.data(),
                                                                       frame_len,
                                                                       arq.cfg.checksum_cb,
                                                                       &h,
                                                                       (void const **)&seg);
                    CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, r);
                    std::copy(seg, seg + h.seg_len, std::back_inserter(output_data));
                }
            }
            arq__send_wnd_ack(&arq.send_wnd, seq, (1 << arq.cfg.message_length_in_segments) - 1);
            seq = (seq + 1) % (ARQ__FRAME_MAX_SEQ_NUM + 1);
        }
    }
    CHECK_EQUAL(input_data.size(), output_data.size());
    MEMCMP_EQUAL(input_data.data(), output_data.data(), input_data.size());
}

}

