#include "functional_tests.h"

namespace {

struct ArqContext
{
    explicit ArqContext(arq_cfg_t const &config)
    {
        arq.cfg = config;

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
        arq__recv_wnd_ptr_init(&arq.recv_wnd_ptr);
        arq__recv_wnd_rst(&arq.recv_wnd);
    }

    ~ArqContext() = default;

    arq_t arq;
    std::array< arq__msg_t, 16 > send_wnd_msgs;
    std::array< arq_time_t, 16 > rtx_timers;
    std::array< unsigned char, 256 > send_frame;
    std::vector< unsigned char > send_wnd_buf;
    std::vector< unsigned char > recv_wnd_buf;
    std::array< unsigned char, 256 > recv_frame;
    std::array< arq__msg_t, 16 > recv_wnd_msgs;
    std::array< arq_uchar_t, 16 > recv_wnd_ack;

    ArqContext() = delete;
    ArqContext(ArqContext const &) = delete;
    ArqContext &operator =(ArqContext const &) = delete;
};

TEST(functional, transfer_full_window_one_way)
{
    arq_cfg_t config;
    config.segment_length_in_bytes = 220;
    config.message_length_in_segments = 4;
    config.retransmission_timeout = 100;

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

