#include "functional_tests.h"

namespace {

TEST(functional, retransmission_timers)
{
    arq_cfg_t cfg;
    {
        arq_err_t const e = arq_seg_len_from_frame_len(128, &cfg.segment_length_in_bytes);
        CHECK(ARQ_SUCCEEDED(e));
    }
    cfg.message_length_in_segments = 1;
    cfg.send_window_size_in_messages = 16;
    cfg.recv_window_size_in_messages = 16;
    cfg.retransmission_timeout = 100;
    cfg.tinygram_send_delay = 50;
    cfg.checksum = &arq_crc32;
    ArqContext ctx(cfg);

    std::vector< arq_uchar_t > send_test_data(cfg.segment_length_in_bytes *
                                              cfg.message_length_in_segments *
                                              3);
    for (auto i = 0u; i < send_test_data.size() / 2; ++i) {
        uint16_t const v = i;
        std::memcpy(&send_test_data[i * 2], &v, sizeof(v));
    }

    {
        unsigned sent;
        arq_err_t const e = arq_send(ctx.arq, send_test_data.data(), send_test_data.size(), &sent);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(send_test_data.size(), sent);
    }

    std::vector< arq_uchar_t > recv_test_data(send_test_data.size());
    arq__frame_hdr_t hdr;
    arq_uchar_t frame[256];
    void const *seg;

    // drain message 0
    {
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, recv_pending;
        void const *p;
        int size;
        arq_err_t e = arq_backend_poll(ctx.arq, 0, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && send_pending);
        e = arq_backend_send_ptr_get(ctx.arq, &p, &size);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(arq__frame_len(cfg.segment_length_in_bytes), (unsigned)size);
        std::memcpy(frame, p, size);
        e = arq_backend_send_ptr_release(ctx.arq);
        CHECK(ARQ_SUCCEEDED(e));
        arq__frame_read_result_t const r = arq__frame_read(frame, size, &arq_crc32, &hdr, &seg);
        CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, r);
        CHECK_EQUAL(0, hdr.seq_num);
        std::memcpy(recv_test_data.data(), seg, hdr.seg_len);
    }

    // lose message 1
    {
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, recv_pending;
        void const *p;
        int size;
        arq_err_t e = arq_backend_poll(ctx.arq, 0, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && send_pending);
        e = arq_backend_send_ptr_get(ctx.arq, &p, &size);
        CHECK(ARQ_SUCCEEDED(e));
        e = arq_backend_send_ptr_release(ctx.arq);
        CHECK(ARQ_SUCCEEDED(e));
    }

    // drain message 2
    {
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, recv_pending;
        void const *p;
        int size;
        arq_err_t e = arq_backend_poll(ctx.arq, 0, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && send_pending);
        e = arq_backend_send_ptr_get(ctx.arq, &p, &size);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(arq__frame_len(cfg.segment_length_in_bytes), (unsigned)size);
        std::memcpy(frame, p, size);
        e = arq_backend_send_ptr_release(ctx.arq);
        CHECK(ARQ_SUCCEEDED(e));
        arq__frame_read_result_t const r = arq__frame_read(frame, size, &arq_crc32, &hdr, &seg);
        CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, r);
        CHECK_EQUAL(2, hdr.seq_num);
        std::memcpy(&recv_test_data[cfg.segment_length_in_bytes * 2], seg, hdr.seg_len);
        e = arq_backend_poll(ctx.arq, 0, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && send_pending);
    }

    // ack message 0
    {
        arq__frame_hdr_t h;
        arq__frame_hdr_init(&h);
        h.ack = 0;
        h.ack_num = 0;
        h.cur_ack_vec = 1;
        unsigned const len = arq__frame_write(&h, nullptr, &arq_crc32, frame, sizeof(frame));
        int recvd;
        arq_err_t e = arq_backend_recv_fill(ctx.arq, frame, sizeof(frame), &recvd);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(len, (unsigned)recvd);
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, recv_pending;
        e = arq_backend_poll(ctx.arq, 0, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && !send_pending);
    }

    // ack message 2
    {
        arq__frame_hdr_t h;
        arq__frame_hdr_init(&h);
        h.ack = 2;
        h.ack_num = 0;
        h.cur_ack_vec = 1;
        unsigned const len = arq__frame_write(&h, nullptr, &arq_crc32, frame, sizeof(frame));
        int recvd;
        arq_err_t e = arq_backend_recv_fill(ctx.arq, frame, sizeof(frame), &recvd);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(len, (unsigned)recvd);
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, recv_pending;
        e = arq_backend_poll(ctx.arq, 0, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && !send_pending);
    }

    // confirm that message 1's retransmission timer is active
    {
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, recv_pending;
        arq_err_t const e = arq_backend_poll(ctx.arq, 0, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && !send_pending);
        CHECK_EQUAL(cfg.retransmission_timeout, next_poll);
    }

    // step slowly towards retransmission
    for (auto i = 0u; i < cfg.retransmission_timeout - 1; ++i) {
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, recv_pending;
        arq_err_t const e = arq_backend_poll(ctx.arq, 0, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && !send_pending);
        CHECK_EQUAL(cfg.retransmission_timeout - i - 1, next_poll);
    }

    // drain message 1
    {
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, recv_pending;
        void const *p;
        int size;
        arq_err_t e = arq_backend_poll(ctx.arq, 1, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && send_pending);
        e = arq_backend_send_ptr_get(ctx.arq, &p, &size);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(arq__frame_len(cfg.segment_length_in_bytes), (unsigned)size);
        std::memcpy(frame, p, size);
        e = arq_backend_send_ptr_release(ctx.arq);
        CHECK(ARQ_SUCCEEDED(e));
        arq__frame_read_result_t const r = arq__frame_read(frame, size, &arq_crc32, &hdr, &seg);
        CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, r);
        CHECK_EQUAL(1, hdr.seq_num);
        std::memcpy(&recv_test_data[cfg.segment_length_in_bytes], seg, hdr.seg_len);
        e = arq_backend_poll(ctx.arq, 0, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e));
    }
    MEMCMP_EQUAL(recv_test_data.data(), send_test_data.data(), send_test_data.size());
}

}

