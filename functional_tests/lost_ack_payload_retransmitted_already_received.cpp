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
    cfg.inter_segment_timeout = 100;
    cfg.checksum = &arq_crc32;

    ArqContext sender(cfg), receiver(cfg);

    std::vector< arq_uchar_t > send_test_data(cfg.segment_length_in_bytes * cfg.message_length_in_segments);
    for (auto i = 0u; i < send_test_data.size() / 2; ++i) {
        uint16_t const v = i;
        std::memcpy(&send_test_data[i * 2], &v, sizeof(v));
    }

    std::vector< unsigned char > recv_test_data(send_test_data.size());
    std::array< arq_uchar_t, 256 > frame;
    auto frame_len = 0u;

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
        unsigned bytes_filled;
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
    }

    // lose the ACK from receiver to sender
    {
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, recv_pending;
        arq_err_t e = arq_backend_poll(receiver.arq, 0, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && send_pending);
        void const *p;
        e = arq_backend_send_ptr_get(receiver.arq, &p, &frame_len);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(frame.data(), p, frame_len);
        e = arq_backend_send_ptr_release(receiver.arq);
        CHECK(ARQ_SUCCEEDED(e));
        e = arq_backend_poll(receiver.arq, 0, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && !send_pending);
        arq__frame_hdr_t h;
        void const *seg;
        arq__frame_read_result_t const r = arq__frame_read(frame.data(), frame_len, cfg.checksum, &h, &seg);
        CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, r);
        CHECK_EQUAL(0, h.seg);
        CHECK_EQUAL(0, h.seg_len);
        CHECK(h.ack);
        CHECK_EQUAL(0, h.ack_num);
        CHECK_EQUAL(1, h.cur_ack_vec);
    }

    // poll the sender with a large dt to retransmit the message
    {
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, r;
        arq_err_t e =
            arq_backend_poll(sender.arq, cfg.retransmission_timeout, &event, &send_pending, &r, &next_poll);
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

    // receive the retransmitted message
    {
        unsigned bytes_filled;
        arq_err_t e = arq_backend_recv_fill(receiver.arq, frame.data(), frame_len, &bytes_filled);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(frame_len, bytes_filled);
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, recv_pending;
        e = arq_backend_poll(receiver.arq, 0, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && recv_pending && send_pending);
    }

    // examine and confirm the ACK
    {
        void const *p;
        arq_err_t e = arq_backend_send_ptr_get(receiver.arq, &p, &frame_len);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(arq__frame_len(0), frame_len);
        std::memcpy(frame.data(), p, frame_len);
        e = arq_backend_send_ptr_release(receiver.arq);
        CHECK(ARQ_SUCCEEDED(e));
        arq__frame_hdr_t h;
        void const *seg;
        arq__frame_read_result_t const r = arq__frame_read(frame.data(), frame_len, cfg.checksum, &h, &seg);
        CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, r);
        CHECK_EQUAL(0, h.seg);
        CHECK_EQUAL(0, h.seg_len);
        CHECK(h.ack);
        CHECK_EQUAL(0, h.ack_num);
        CHECK_EQUAL(1, h.cur_ack_vec);
    }
}

}

