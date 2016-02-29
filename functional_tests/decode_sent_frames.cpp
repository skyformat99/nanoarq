#include "functional_tests.h"

namespace {

TEST(functional, decode_sent_frames)
{
    arq_t arq;
    std::array< arq__msg_t, 64 > send_wnd_msgs;
    std::array< unsigned char, 256 > send_frame;

    int const segment_length_in_bytes = 230;
    int const message_length_in_segments = 10;
    arq_time_t const retransmission_timeout = 100;

    std::vector< unsigned char > send_wnd_buf;
    send_wnd_buf.resize((send_wnd_msgs.size() * segment_length_in_bytes * message_length_in_segments));

    arq.cfg.checksum_cb = &arq_crc32;
    arq.send_wnd.msg = send_wnd_msgs.data();
    arq.send_wnd.buf = send_wnd_buf.data();
    arq.send_frame.buf = send_frame.data();

    arq__send_wnd_init(&arq.send_wnd,
                       send_wnd_msgs.size(),
                       message_length_in_segments * segment_length_in_bytes,
                       segment_length_in_bytes,
                       retransmission_timeout);
    arq__send_frame_init(&arq.send_frame, send_frame.size());
    arq__send_wnd_ptr_init(&arq.send_wnd_ptr);

    for (auto i = 0u; i < send_wnd_buf.size() / 2; ++i) {
        uint16_t const v = i;
        std::memcpy(&send_wnd_buf[i * 2], &v, sizeof(v));
    }

    {
        int sent;
        arq_err_t const e = arq_send(&arq, send_wnd_buf.data(), send_wnd_buf.size(), &sent);
        CHECK_EQUAL(ARQ_OK_COMPLETED, e);
    }

    std::vector< unsigned char > recv;
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
            recv.insert(std::end(recv), (unsigned char const *)seg, (unsigned char const *)seg + h.seg_len);
        }
    }
    MEMCMP_EQUAL(send_wnd_buf.data(), recv.data(), send_wnd_buf.size());
}

}

