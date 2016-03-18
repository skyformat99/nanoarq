#include "functional_tests.h"

namespace {

TEST(functional, recv_full_window)
{
    arq_t arq;
    std::array< arq__msg_t, 4 > send_wnd_msgs;
    std::array< arq_time_t, 64 > rtx_timers;
    std::array< arq_uchar_t, 256 > send_frame;
    std::vector< arq_uchar_t > send_wnd_buf;
    std::vector< arq_uchar_t > recv_wnd_buf;
    std::array< arq_uchar_t, 256 > recv_frame;
    std::array< arq__msg_t, 4 > recv_wnd_msgs;
    std::array< arq_uchar_t, 4 > recv_wnd_ack;

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

    arq.recv_wnd.ack = recv_wnd_ack.data();
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

    std::vector< arq_uchar_t > test_input, test_output;

    test_input.resize(recv_wnd_buf.size());
    for (auto i = 0u; i < test_input.size() / 2; ++i) {
        arq_uint16_t x = (arq_uint16_t)i;
        std::memcpy(&test_input[i * 2], &x, sizeof(x));
    }

    arq__frame_hdr_t h;
    h.msg_len = arq.cfg.message_length_in_segments;
    h.seg_id = 0;
    h.seq_num = 0;
    h.cur_ack_vec = 0;

    std::vector< arq_uchar_t > frame;
    frame.reserve(arq__frame_len(arq.cfg.segment_length_in_bytes));

    size_t test_input_offset = 0;
    while (test_input_offset < test_input.size()) {
        h.seg_len = arq__min(arq.cfg.segment_length_in_bytes, test_input.size() - test_input_offset);
        frame.resize(arq__frame_len(h.seg_len));

        int const frame_len =
            arq__frame_write(&h, &test_input[test_input_offset], arq_crc32, frame.data(), frame.size());

        test_input_offset += h.seg_len;
        h.seq_num += (h.seg_id == arq.cfg.message_length_in_segments - 1);
        h.seg_id = (h.seg_id + 1) % arq.cfg.message_length_in_segments;

        {
            int recvd;
            arq_err_t const e = arq_backend_recv_fill(&arq, frame.data(), frame_len, &recvd);
            CHECK(ARQ_SUCCEEDED(e));
            CHECK_EQUAL(frame_len, recvd);
        }

        {
            int send_len;
            arq_event_t event;
            arq_time_t next_poll;
            arq_err_t const e = arq_backend_poll(&arq, 0, &send_len, &event, &next_poll);
            CHECK(ARQ_SUCCEEDED(e));
        }
    }
    CHECK_EQUAL(test_input.size(), test_input_offset);

    while (test_output.size() < test_input.size()) {
        std::array< arq_uchar_t, 256 > recv;
        int recvd;
        arq_err_t const e = arq_recv(&arq, recv.data(), recv.size(), &recvd);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK(recvd > 0);
        std::copy(&recv[0], &recv[recvd], std::back_inserter(test_output));
    }
    CHECK_EQUAL(test_input.size(), test_output.size());
    MEMCMP_EQUAL(test_input.data(), test_output.data(), test_input.size());
}

}

