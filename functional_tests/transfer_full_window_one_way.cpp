#include "functional_tests.h"
#include "arq_context.h"

namespace {

TEST(functional, transfer_full_window_one_way_manual_acks)
{
    arq_cfg_t config;
    config.segment_length_in_bytes = 220;
    config.message_length_in_segments = 4;
    config.retransmission_timeout = 100;
    config.checksum_cb = &arq_crc32;

    ArqContext sender(config), receiver(config);

    std::vector< unsigned char > send_test_data(sender.send_wnd_buf.size());
    for (auto i = 0u; i < send_test_data.size() / 2; ++i) {
        uint16_t const v = i;
        std::memcpy(&send_test_data[i * 2], &v, sizeof(v));
    }

    std::vector< unsigned char > recv_test_data;
    recv_test_data.reserve(send_test_data.size());

    {
        int sent;
        arq_err_t const e = arq_send(&sender.arq, send_test_data.data(), send_test_data.size(), &sent);
        CHECK_EQUAL(ARQ_OK_COMPLETED, e);
        CHECK_EQUAL((int)send_test_data.size(), sent);
    }

    for (auto i = 0; i < config.message_length_in_segments * sender.arq.send_wnd.w.cap; ++i) {
        {
            arq_event_t event;
            arq_time_t next_poll;
            int bytes_to_drain;
            arq_err_t const e = arq_backend_poll(&sender.arq, 0, &bytes_to_drain, &event, &next_poll);
            CHECK(ARQ_SUCCEEDED(e));
            CHECK(bytes_to_drain > 0);
        }

        int size;
        unsigned char decode_buf[256];
        {
            void const *p;
            arq_err_t e = arq_backend_send_ptr_get(&sender.arq, &p, &size);
            CHECK(ARQ_SUCCEEDED(e));
            std::memcpy(decode_buf, p, size);
            e = arq_backend_send_ptr_release(&sender.arq);
            CHECK(ARQ_SUCCEEDED(e));
        }

        {
            int bytes_filled;
            arq_err_t const e = arq_backend_recv_fill(&receiver.arq, decode_buf, size, &bytes_filled);
            CHECK(ARQ_SUCCEEDED(e));
            CHECK_EQUAL(size, bytes_filled);
        }

        {
            arq_event_t event;
            arq_time_t next_poll;
            int bytes_to_drain;
            arq_err_t const e = arq_backend_poll(&receiver.arq, 0, &bytes_to_drain, &event, &next_poll);
            CHECK(ARQ_SUCCEEDED(e));
        }
    }

    while (recv_test_data.size() < send_test_data.size()) {
        {
            unsigned char recv_buf[256];
            int bytes_recvd;
            arq_err_t const e = arq_recv(&receiver.arq, recv_buf, sizeof(recv_buf), &bytes_recvd);
            CHECK(ARQ_SUCCEEDED(e));
            CHECK(bytes_recvd > 0);

            std::copy(std::begin(recv_buf), std::end(recv_buf), std::back_inserter(recv_test_data));
        }
    }
    CHECK_EQUAL(send_test_data.size(), recv_test_data.size());
    MEMCMP_EQUAL(send_test_data.data(), recv_test_data.data(), send_test_data.size());
}

}

