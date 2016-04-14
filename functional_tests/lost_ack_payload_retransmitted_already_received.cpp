#include "functional_tests.h"
#include "arq_context.h"

namespace {

TEST(functional, lost_ack_payload_retransmitted_already_received)
{
    arq_cfg_t cfg;
    cfg.segment_length_in_bytes = 128;
    cfg.message_length_in_segments = 1;
    cfg.send_window_size_in_messages = 1;
    cfg.recv_window_size_in_messages = 1;
    cfg.retransmission_timeout = 100;
    cfg.checksum = &arq_crc32;

    ArqContext sender(cfg), receiver(cfg);

    std::vector< arq_uchar_t > send_test_data(cfg.segment_length_in_bytes * cfg.message_length_in_segments);
    for (auto i = 0u; i < send_test_data.size() / 2; ++i) {
        uint16_t const v = i;
        std::memcpy(&send_test_data[i * 2], &v, sizeof(v));
    }

    std::vector< unsigned char > recv_test_data(send_test_data.size());

    std::array< arq_uchar_t, 256 > frame;
    auto frame_len = 0;

    // send a full message
    {
        unsigned sent;
        arq_err_t e = arq_send(sender.arq, send_test_data.data(), send_test_data.size(), &sent);
        CHECK_EQUAL(ARQ_OK_COMPLETED, e);
        CHECK_EQUAL(send_test_data.size(), sent);
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, r;
        e = arq_backend_poll(sender.arq, 0, &event, &send_pending, &r, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && send_pending);
        void const *p;
        e = arq_backend_send_ptr_get(sender.arq, &p, &frame_len);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(frame.data(), p, frame_len);
        e = arq_backend_send_ptr_release(sender.arq);
        CHECK(ARQ_SUCCEEDED(e));
        e = arq_backend_poll(sender.arq, 0, &event, &send_pending, &r, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && !send_pending);
    }

    // receive the message
    {
        int bytes_filled;
        arq_err_t e = arq_backend_recv_fill(receiver.arq, frame.data(), frame_len, &bytes_filled);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(frame_len, bytes_filled);
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t s, recv_pending;
        e = arq_backend_poll(receiver.arq, 0, &event, &s, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && recv_pending);
        unsigned recvd;
        e = arq_recv(receiver.arq, recv_test_data.data(), recv_test_data.size(), &recvd);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(send_test_data.size(), recvd);
        MEMCMP_EQUAL(send_test_data.data(), recv_test_data.data(), send_test_data.size());
        CHECK_EQUAL(0, receiver.arq->recv_wnd.w.size);
    }

    // lose the ACK from receiver to sender
    {
    }
}

}

