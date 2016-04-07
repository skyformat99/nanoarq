#include "functional_tests.h"
#include "arq_context.h"

namespace {

TEST(functional, ack_full_window)
{
    arq_cfg_t cfg;
    cfg.segment_length_in_bytes = 128;
    cfg.message_length_in_segments = 4;
    cfg.send_window_size_in_messages = 16;
    cfg.recv_window_size_in_messages = 16;
    cfg.retransmission_timeout = 100;
    cfg.checksum = &arq_crc32;

    ArqContext sender(cfg), receiver(cfg);

    std::vector< arq_uchar_t > send_test_data(cfg.segment_length_in_bytes *
                                              cfg.message_length_in_segments *
                                              cfg.send_window_size_in_messages);

    for (auto i = 0u; i < send_test_data.size() / 2; ++i) {
        uint16_t const v = i;
        std::memcpy(&send_test_data[i * 2], &v, sizeof(v));
    }

    std::vector< unsigned char > recv_test_data;
    recv_test_data.reserve(send_test_data.size());

    std::array< arq_uchar_t, 256 > frame;
    auto frame_len = 0;

    // send a full message into the sender
    {
        int sent;
        arq_err_t const e = arq_send(sender.arq, send_test_data.data(), send_test_data.size(), &sent);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL((int)send_test_data.size(), sent);
    }

    for (auto i = 0u; i < cfg.message_length_in_segments * sender.arq->send_wnd.w.cap; ++i) {
        // poll the sender to move a frame from the send window into the send frame
        {
            arq_event_t event;
            arq_time_t next_poll;
            int bytes_to_drain;
            arq_err_t const e = arq_backend_poll(sender.arq, 0, &bytes_to_drain, &event, &next_poll);
            CHECK(ARQ_SUCCEEDED(e));
            CHECK(bytes_to_drain > 0);
        }

        // drain the send frame
        {
            void const *p;
            arq_err_t e = arq_backend_send_ptr_get(sender.arq, &p, &frame_len);
            CHECK(ARQ_SUCCEEDED(e));
            std::memcpy(frame.data(), p, frame_len);
            e = arq_backend_send_ptr_release(sender.arq);
            CHECK(ARQ_SUCCEEDED(e));
        }

        // load the send frame into the receiver
        {
            int bytes_filled;
            arq_err_t const e = arq_backend_recv_fill(receiver.arq, frame.data(), frame_len, &bytes_filled);
            CHECK(ARQ_SUCCEEDED(e));
            CHECK_EQUAL(frame_len, bytes_filled);
        }

        // poll the receiver to process the frame
        {
            arq_event_t event;
            arq_time_t next_poll;
            int bytes_to_drain;
            arq_err_t const e = arq_backend_poll(receiver.arq, 0, &bytes_to_drain, &event, &next_poll);
            CHECK(ARQ_SUCCEEDED(e));
        }
    }

    for (auto i = 0u; i < cfg.send_window_size_in_messages; ++i) {
        // drain the receiver's ACK-only frame
        {
            void const *p;
            arq_err_t e = arq_backend_send_ptr_get(receiver.arq, &p, &frame_len);
            CHECK(ARQ_SUCCEEDED(e));
            std::memcpy(frame.data(), p, frame_len);
            e = arq_backend_send_ptr_release(receiver.arq);
            CHECK(ARQ_SUCCEEDED(e));
        }

        // poll the receiver to process the frame
        {
            arq_event_t event;
            arq_time_t next_poll;
            int bytes_to_drain;
            arq_err_t const e = arq_backend_poll(receiver.arq, 0, &bytes_to_drain, &event, &next_poll);
            CHECK(ARQ_SUCCEEDED(e));
        }

        // load the ACK frame into the sender
        {
            int bytes_filled;
            arq_err_t const e = arq_backend_recv_fill(sender.arq, frame.data(), frame_len, &bytes_filled);
            CHECK(ARQ_SUCCEEDED(e));
            CHECK_EQUAL(frame_len, bytes_filled);
        }

        // poll the sender to slide the send window
        {
            arq_event_t event;
            arq_time_t next_poll;
            int bytes_to_drain;
            arq_err_t const e = arq_backend_poll(sender.arq, 0, &bytes_to_drain, &event, &next_poll);
            CHECK(ARQ_SUCCEEDED(e));
        }
    }

    // drain the receiver's receive window
    {
        std::array< arq_uchar_t, 128 > data;
        int bytes_read;
        do {
            arq_err_t const e = arq_recv(receiver.arq, data.data(), data.size(), &bytes_read);
            CHECK(ARQ_SUCCEEDED(e));
            std::copy(data.data(), data.data() + bytes_read, std::back_inserter(recv_test_data));
        } while (bytes_read);
    }

    CHECK_EQUAL(0, sender.arq->send_wnd.w.size);
    CHECK_EQUAL(send_test_data.size(), recv_test_data.size());
    MEMCMP_EQUAL(recv_test_data.data(), send_test_data.data(), send_test_data.size());
}

}

