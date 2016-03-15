#include "functional_tests.h"

namespace {

TEST(functional, send_full_window)
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
    std::vector< unsigned char > send_test_data(send_wnd_buf.size());
    for (auto i = 0u; i < send_test_data.size() / 2; ++i) {
        uint16_t const v = i;
        std::memcpy(&send_test_data[i * 2], &v, sizeof(v));
    }

    std::vector< unsigned char > recv_test_data;
    recv_test_data.reserve(send_test_data.size());

    {
        int sent;
        arq_err_t const e = arq_send(&arq, send_test_data.data(), send_test_data.size(), &sent);
        CHECK_EQUAL(ARQ_OK_COMPLETED, e);
    }

    for (;;) {
        unsigned char decode_buf[256];
        {
            arq_event_t event;
            arq_time_t next_poll;
            int bytes_to_drain;
            arq_err_t e = arq_backend_poll(&arq, 0, &bytes_to_drain, &event, &next_poll);
            CHECK_EQUAL(ARQ_OK_COMPLETED, e);
            if (bytes_to_drain == 0) {
                break;
            }
        }

        int size;
        {
            void const *p;
            arq_err_t e = arq_backend_send_ptr_get(&arq, &p, &size);
            CHECK_EQUAL(ARQ_OK_COMPLETED, e);
            std::memcpy(decode_buf, p, size);
            e = arq_backend_send_ptr_release(&arq);
            CHECK_EQUAL(ARQ_OK_COMPLETED, e);
        }

        {
            void const *seg;
            arq__frame_hdr_t h;
            arq__frame_read_result_t const r =
                arq__frame_read(decode_buf, size, arq.cfg.checksum_cb, &h, &seg);
            CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, r);
            recv_test_data.insert(std::end(recv_test_data),
                                  (unsigned char const *)seg,
                                  (unsigned char const *)seg + h.seg_len);
        }
    }

    MEMCMP_EQUAL(send_test_data.data(), recv_test_data.data(), send_test_data.size());
}

}

