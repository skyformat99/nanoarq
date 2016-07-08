#include "functional_tests.h"
#include "arq_context.h"

namespace {

struct frame_t
{
    std::array< arq_uchar_t, 256 > buf;
    unsigned len;
};

TEST(functional, connect_simultaneous)
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

    frame_t peer1_rst, peer1_ack, peer2_rst, peer2_ack;
    std::array< arq_uchar_t, 256 > tmp;
    unsigned recvd;
    arq_event_t event;
    arq_time_t next_poll;
    arq_bool_t sp, rp;
    void const *p;
    void const *seg;
    arq__frame_hdr_t hdr;
    arq_err_t e;

    // peer1 connect, confirm payload carries rst
    {
        e = arq_connect(peer1.arq);
        CHECK(ARQ_SUCCEEDED(e));
        e = arq_backend_poll(peer1.arq, 0, &event, &sp, &rp, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && (event == ARQ_EVENT_NONE) && sp && !rp);
        e = arq_backend_send_ptr_get(peer1.arq, &p, &peer1_rst.len);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(peer1_rst.buf.data(), p, peer1_rst.len);
        e = arq_backend_send_ptr_release(peer1.arq);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(tmp.data(), peer1_rst.buf.data(), peer1_rst.len);
        arq__frame_read_result_t const read_result =
            arq__frame_read(tmp.data(), peer1_rst.len, cfg.checksum, &hdr, &seg);
        CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, read_result);
        CHECK(hdr.rst && !hdr.ack && !hdr.seg);
    }

    // peer2 connect, confirm payload carries rst
    {
        e = arq_connect(peer2.arq);
        CHECK(ARQ_SUCCEEDED(e));
        e = arq_backend_poll(peer2.arq, 0, &event, &sp, &rp, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && (event == ARQ_EVENT_NONE) && sp && !rp);
        e = arq_backend_send_ptr_get(peer2.arq, &p, &peer2_rst.len);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(peer2_rst.buf.data(), p, peer2_rst.len);
        e = arq_backend_send_ptr_release(peer2.arq);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(tmp.data(), peer2_rst.buf.data(), peer2_rst.len);
        arq__frame_read_result_t const read_result =
            arq__frame_read(tmp.data(), peer2_rst.len, cfg.checksum, &hdr, &seg);
        CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, read_result);
        CHECK(hdr.rst && !hdr.ack && !hdr.seg);
    }

    // deliver peer1's rst to peer2, confirm peer2 ack + connected
    {
        e = arq_backend_recv_fill(peer2.arq, peer1_rst.buf.data(), peer1_rst.len, &recvd);
        CHECK(ARQ_SUCCEEDED(e) && (recvd == peer1_rst.len));
        e = arq_backend_poll(peer2.arq, 0, &event, &sp, &rp, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && (event == ARQ_EVENT_CONN_ESTABLISHED) && sp && !rp);
        e = arq_backend_send_ptr_get(peer2.arq, &p, &peer2_ack.len);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(peer2_ack.buf.data(), p, peer2_ack.len);
        e = arq_backend_send_ptr_release(peer2.arq);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(tmp.data(), peer2_ack.buf.data(), peer2_ack.len);
        arq__frame_read_result_t const read_result =
            arq__frame_read(tmp.data(), peer2_ack.len, cfg.checksum, &hdr, &seg);
        CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, read_result);
        CHECK(!hdr.rst && hdr.ack && !hdr.seg);
    }

    // deliver peer2's rst to peer1, confirm peer1 ack + connected
    {
        e = arq_backend_recv_fill(peer1.arq, peer2_rst.buf.data(), peer2_rst.len, &recvd);
        CHECK(ARQ_SUCCEEDED(e) && (recvd == peer2_rst.len));
        e = arq_backend_poll(peer1.arq, 0, &event, &sp, &rp, &next_poll);
        CHECK(ARQ_SUCCEEDED(e) && (event == ARQ_EVENT_CONN_ESTABLISHED) && sp && !rp);
        e = arq_backend_send_ptr_get(peer1.arq, &p, &peer1_ack.len);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(peer1_ack.buf.data(), p, peer1_ack.len);
        e = arq_backend_send_ptr_release(peer1.arq);
        CHECK(ARQ_SUCCEEDED(e));
        std::memcpy(tmp.data(), peer1_ack.buf.data(), peer1_ack.len);
        arq__frame_read_result_t const read_result =
            arq__frame_read(tmp.data(), peer1_ack.len, cfg.checksum, &hdr, &seg);
        CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, read_result);
        CHECK(!hdr.rst && hdr.ack && !hdr.seg);
    }
}

}

