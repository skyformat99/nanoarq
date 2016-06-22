#include "functional_tests.h"
#include "arq_context.h"

namespace {

TEST(functional, losing_final_segment_in_message_triggers_nak)
{
    arq_cfg_t cfg;
    cfg.segment_length_in_bytes = 128;
    cfg.message_length_in_segments = 2;
    cfg.send_window_size_in_messages = 1;
    cfg.recv_window_size_in_messages = 1;
    cfg.retransmission_timeout = 100;
    cfg.inter_segment_timeout = 50;
    cfg.checksum = &arq_crc32;
    cfg.connection_rst_period = 100;
    cfg.connection_rst_attempts = 10;

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
    }

    // drain the first segment from the sender
    {
        void const *p;
        arq_err_t e = arq_backend_send_ptr_get(sender.arq, &p, &frame_len);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(frame.data(), p, frame_len);
        e = arq_backend_send_ptr_release(sender.arq);
        CHECK(ARQ_SUCCEEDED(e));
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, r;
        e = arq_backend_poll(sender.arq, 0, &event, &send_pending, &r, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && send_pending);
    }

    // lose the second segment from the sender
    {
        void const *p;
        arq_err_t e = arq_backend_send_ptr_get(sender.arq, &p, &frame_len);
        CHECK(ARQ_SUCCEEDED(e));
        e = arq_backend_send_ptr_release(sender.arq);
        CHECK(ARQ_SUCCEEDED(e));
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, r;
        e = arq_backend_poll(sender.arq, 0, &event, &send_pending, &r, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && !send_pending);
    }

    // receive the first segment, 'wait' until inter-segment timeout expires, drain the NAK frame
    {
        unsigned bytes_filled;
        arq_err_t e = arq_backend_recv_fill(receiver.arq, frame.data(), frame_len, &bytes_filled);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(frame_len, bytes_filled);
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, recv_pending;
        e = arq_backend_poll(receiver.arq, 0, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && !recv_pending && !send_pending);
        CHECK_EQUAL(cfg.inter_segment_timeout, next_poll);
        e = arq_backend_poll(receiver.arq, next_poll, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && !recv_pending && send_pending);
        void const *p;
        e = arq_backend_send_ptr_get(receiver.arq, &p, &frame_len);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(frame.data(), p, frame_len);
        e = arq_backend_send_ptr_release(receiver.arq);
        CHECK(ARQ_SUCCEEDED(e));
        e = arq_backend_poll(receiver.arq, 0, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && !send_pending && !recv_pending);
    }

    // confirm the NAK frame contents
    {
        std::array< arq_uchar_t, 256 > nak;
        std::memcpy(nak.data(), frame.data(), frame_len);
        arq__frame_hdr_t h;
        void const *seg;
        arq__frame_read_result_t const r = arq__frame_read(nak.data(), frame_len, cfg.checksum, &h, &seg);
        CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, r);
        CHECK_EQUAL(ARQ_FALSE, h.seg);
        CHECK_EQUAL(0, h.seg_len);
        CHECK_EQUAL(0, h.msg_len);
        CHECK_EQUAL(ARQ_TRUE, h.ack);
        CHECK_EQUAL(0, h.ack_num);
        CHECK_EQUAL(1, h.cur_ack_vec);
    }

    // push the NAK frame into the sender
    {
        unsigned bytes_filled;
        arq_err_t e = arq_backend_recv_fill(sender.arq, frame.data(), frame_len, &bytes_filled);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(frame_len, bytes_filled);
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, recv_pending;
        e = arq_backend_poll(sender.arq, 0, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && !recv_pending && send_pending);
    }

    // drain the retransmission from the sender
    {
        void const *p;
        arq_err_t e = arq_backend_send_ptr_get(sender.arq, &p, &frame_len);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(frame.data(), p, frame_len);
        e = arq_backend_send_ptr_release(sender.arq);
        CHECK(ARQ_SUCCEEDED(e));
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, r;
        e = arq_backend_poll(sender.arq, 0, &event, &send_pending, &r, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && !send_pending);
    }

    // confirm the retransmission
    {
        std::array< arq_uchar_t, 256 > rtx;
        std::memcpy(rtx.data(), frame.data(), frame_len);
        arq__frame_hdr_t h;
        void const *seg;
        arq__frame_read_result_t const r = arq__frame_read(rtx.data(), frame_len, cfg.checksum, &h, &seg);
        CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, r);
        CHECK_EQUAL(ARQ_TRUE, h.seg);
        CHECK_EQUAL(0, h.seq_num);
        CHECK_EQUAL(1, h.seg_id);
        CHECK_EQUAL(2, h.msg_len);
        CHECK_EQUAL(ARQ_FALSE, h.ack);
    }

    // load into receiver, receive into test vector
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
        unsigned recvd;
        e = arq_recv(receiver.arq, recv_test_data.data(), recv_test_data.size(), &recvd);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(send_test_data.size(), recvd);
        MEMCMP_EQUAL(send_test_data.data(), recv_test_data.data(), send_test_data.size());
    }
}

}

