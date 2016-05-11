#include "functional_tests.h"

namespace {

TEST(functional, flush_tinygram)
{
    arq_cfg_t cfg;
    cfg.segment_length_in_bytes = 220;
    cfg.message_length_in_segments = 4;
    cfg.send_window_size_in_messages = 16;
    cfg.recv_window_size_in_messages = 16;
    cfg.retransmission_timeout = 1000;
    cfg.tinygram_send_delay = cfg.segment_length_in_bytes;
    cfg.checksum = &arq_crc32;
    ArqContext ctx(cfg);

    arq_uchar_t const send_test_data = 0xA5;
    arq_uchar_t recv_test_data = 0;

    // send the test byte, confirm there's nothing to send
    {
        unsigned sent;
        arq_err_t e = arq_send(ctx.arq, &send_test_data, 1, &sent);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(1, sent);
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, recv_pending;
        e = arq_backend_poll(ctx.arq, 0, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && !send_pending);
    }

    // flush the test byte, confirm there's data to send
    {
        arq_err_t e = arq_flush(ctx.arq);
        CHECK(ARQ_SUCCEEDED(e));
        arq_event_t event;
        arq_time_t next_poll;
        arq_bool_t send_pending, recv_pending;
        e = arq_backend_poll(ctx.arq, 0, &event, &send_pending, &recv_pending, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && send_pending);
    }

    // drain the frame, confirm the test data is sent
    {
        void const *p;
        unsigned size;
        arq_uchar_t frame[256];
        arq__frame_hdr_t hdr;
        arq_err_t e = arq_backend_send_ptr_get(ctx.arq, &p, &size);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(arq__frame_len(1), size);
        std::memcpy(frame, p, size);
        e = arq_backend_send_ptr_release(ctx.arq);
        CHECK(ARQ_SUCCEEDED(e));
        arq__frame_read_result_t const r = arq__frame_read(frame, size, &arq_crc32, &hdr, &p);
        CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, r);
        std::memcpy(&recv_test_data, p, 1);
    }

    CHECK_EQUAL(send_test_data, recv_test_data);
}

}

