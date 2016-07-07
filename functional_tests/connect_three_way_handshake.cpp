#include "functional_tests.h"
#include "arq_context.h"

namespace {

TEST(functional, connect_three_way_handshake)
{
    arq_cfg_t cfg;
    cfg.segment_length_in_bytes = 128;
    cfg.message_length_in_segments = 4;
    cfg.send_window_size_in_messages = 16;
    cfg.recv_window_size_in_messages = 16;
    cfg.retransmission_timeout = 100;
    cfg.inter_segment_timeout = 100;
    cfg.checksum = &arq_crc32;
    cfg.connection_rst_period = 100;
    cfg.connection_rst_attempts = 10;
    ArqContext peer1(cfg), peer2(cfg);

    std::array< arq_uchar_t, 256 > frame, tmp;
    unsigned frame_len, recvd;
    arq_event_t event;
    arq_time_t next_poll;
    arq_bool_t sp, rp;
    void const *p;
    void const *seg;
    arq__frame_hdr_t hdr;
    arq_err_t e;

    // initiate connect, confirm payload carries rst
    {
        e = arq_connect(peer1.arq);
        CHECK(ARQ_SUCCEEDED(e));
        e = arq_backend_poll(peer1.arq, 0, &event, &sp, &rp, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && (event == ARQ_EVENT_NONE) && sp && !rp);
        e = arq_backend_send_ptr_get(peer1.arq, &p, &frame_len);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(frame.data(), p, frame_len);
        e = arq_backend_send_ptr_release(peer1.arq);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(tmp.data(), frame.data(), frame_len);
        arq__frame_read_result_t const read_result =
            arq__frame_read(tmp.data(), frame_len, cfg.checksum, &hdr, &seg);
        CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, read_result);
        CHECK(hdr.rst && !hdr.ack && !hdr.seg);
    }

    // receive rst, confirm response carries rst/ack
    {
        e = arq_backend_recv_fill(peer2.arq, frame.data(), frame_len, &recvd);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(frame_len, recvd);
        e = arq_backend_poll(peer2.arq, 0, &event, &sp, &rp, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && (event == ARQ_EVENT_NONE) && sp && !rp);
        e = arq_backend_send_ptr_get(peer2.arq, &p, &frame_len);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(frame.data(), p, frame_len);
        e = arq_backend_send_ptr_release(peer2.arq);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(tmp.data(), frame.data(), frame_len);
        arq__frame_read_result_t const read_result =
            arq__frame_read(tmp.data(), frame_len, cfg.checksum, &hdr, &seg);
        CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, read_result);
        CHECK(hdr.rst && hdr.ack && !hdr.seg);
    }

    // receive rst/ack, confirm peer1 connected and response carries ack
    {
        e = arq_backend_recv_fill(peer1.arq, frame.data(), frame_len, &recvd);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(frame_len, recvd);
        e = arq_backend_poll(peer1.arq, 0, &event, &sp, &rp, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && (event == ARQ_EVENT_CONN_ESTABLISHED) && sp && !rp);
        CHECK_EQUAL(ARQ_CONN_STATE_ESTABLISHED, peer1.arq->conn.state);
        e = arq_backend_send_ptr_get(peer1.arq, &p, &frame_len);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(frame.data(), p, frame_len);
        e = arq_backend_send_ptr_release(peer1.arq);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(tmp.data(), frame.data(), frame_len);
        arq__frame_read_result_t const read_result =
            arq__frame_read(tmp.data(), frame_len, cfg.checksum, &hdr, &seg);
        CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, read_result);
        CHECK(!hdr.rst && hdr.ack && !hdr.seg);
    }

    // receive ack, confirm peer2 connected
    {
        e = arq_backend_recv_fill(peer2.arq, frame.data(), frame_len, &recvd);
        CHECK(ARQ_SUCCEEDED(e));
        CHECK_EQUAL(frame_len, recvd);
        e = arq_backend_poll(peer2.arq, 0, &event, &sp, &rp, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && (event == ARQ_EVENT_CONN_ESTABLISHED) && !sp && !rp);
        CHECK_EQUAL(ARQ_CONN_STATE_ESTABLISHED, peer2.arq->conn.state);
    }
}

}

