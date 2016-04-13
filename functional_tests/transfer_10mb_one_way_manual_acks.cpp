#include "functional_tests.h"
#include "arq_context.h"

namespace {

TEST(functional, transfer_10mb_one_way_manual_acks)
{
    arq_cfg_t cfg;
    cfg.segment_length_in_bytes = 220;
    cfg.message_length_in_segments = 4;
    cfg.retransmission_timeout = 100;
    cfg.send_window_size_in_messages = 16;
    cfg.recv_window_size_in_messages = 16;
    cfg.checksum = &arq_crc32;

    ArqContext sender(cfg), receiver(cfg);

    std::vector< unsigned char > send_test_data(1024 * 1024 * 10);
    for (auto i = 0u; i < send_test_data.size() / 2; ++i) {
        uint16_t const v = i;
        std::memcpy(&send_test_data[i * 2], &v, sizeof(v));
    }

    std::vector< unsigned char > recv_test_data;
    recv_test_data.reserve(send_test_data.size());

    std::array< arq_uchar_t, 256 > frame;
    auto send_idx = 0u;
    while (recv_test_data.size() < send_test_data.size()) {
        // fill the send window
        {
            arq_event_t event;
            arq_time_t next_poll;
            arq_bool_t s, r;
            arq_err_t e = arq_backend_poll(sender.arq, 0, &event, &s, &r, &next_poll);
            CHECK_EQUAL(ARQ_OK_COMPLETED, e);
            unsigned sent;
            e = arq_send(sender.arq, &send_test_data[send_idx], send_test_data.size() - send_idx, &sent);
            CHECK(ARQ_SUCCEEDED(e));
            e = arq_flush(sender.arq);
            CHECK(ARQ_SUCCEEDED(e));
            send_idx += sent;
        }

        while (sender.arq->send_wnd.w.size) {
            // prepare a segment from send window
            {
                arq_event_t event;
                arq_time_t next_poll;
                arq_bool_t send_pending, r;
                arq_err_t e = arq_backend_poll(sender.arq, 0, &event, &send_pending, &r, &next_poll);
                CHECK_EQUAL(ARQ_OK_COMPLETED, e);
                CHECK(send_pending);
            }

            // copy frame from send window into local frame array
            int frame_len;
            {
                void const *p;
                {
                    arq_err_t const e = arq_backend_send_ptr_get(sender.arq, &p, &frame_len);
                    CHECK_EQUAL(ARQ_OK_COMPLETED, e);
                    CHECK(p && frame_len > 0);
                }
                std::memcpy(frame.data(), p, frame_len);
                if (frame_len) {
                    arq_err_t const e = arq_backend_send_ptr_release(sender.arq);
                    CHECK_EQUAL(ARQ_OK_COMPLETED, e);
                }
            }

            // ack the message
            {
                std::array< arq_uchar_t, 256 > decode;
                std::memcpy(decode.data(), frame.data(), frame_len);
                void const *seg;
                arq__frame_hdr_t h;
                arq__frame_read_result_t const r =
                    arq__frame_read(decode.data(), frame_len, cfg.checksum, &h, &seg);
                CHECK(ARQ_SUCCEEDED(r));
                unsigned const ack_vec = (1 << (h.seg_id + 1)) - 1u;
                arq__send_wnd_ack(&sender.arq->send_wnd, h.seq_num, ack_vec);
            }

            // push frame into receiver
            {
                int filled;
                arq_err_t e = arq_backend_recv_fill(receiver.arq, frame.data(), frame_len, &filled);
                CHECK(ARQ_SUCCEEDED(e));
                CHECK_EQUAL(frame_len, filled);
                arq_event_t event;
                arq_time_t next_poll;
                arq_bool_t s, r;
                e = arq_backend_poll(receiver.arq, 0, &event, &s, &r, &next_poll);
                CHECK(ARQ_SUCCEEDED(e));
            }
        }
        CHECK_EQUAL(0, sender.arq->send_wnd.w.size);

        // drain the receive window
        while (receiver.arq->recv_wnd.w.size) {
            unsigned char recv_buf[1024];
            unsigned bytes_recvd;
            arq_err_t const e = arq_recv(receiver.arq, recv_buf, sizeof(recv_buf), &bytes_recvd);
            CHECK(ARQ_SUCCEEDED(e));
            CHECK(bytes_recvd > 0);
            std::copy(recv_buf, recv_buf + bytes_recvd, std::back_inserter(recv_test_data));
        }
    }

    CHECK_EQUAL(send_test_data.size(), recv_test_data.size());
    MEMCMP_EQUAL(send_test_data.data(), recv_test_data.data(), send_test_data.size());
}

}

