#include "nanoarq_unit_test.h"
#include "nanoarq_hook_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <cstring>

TEST_GROUP(frame) {};

namespace {

TEST(frame, len_is_header_size_plus_segment_length_plus_cobs_overhead_plus_checksum)
{
    int const seg_len = 123;
    CHECK_EQUAL(ARQ__FRAME_COBS_OVERHEAD + ARQ__FRAME_HEADER_SIZE + seg_len + 4,
                arq__frame_len(seg_len));
}

struct Fixture
{
    Fixture()
    {
        h.version = 12;
        h.seg_len = sizeof(seg);
        h.win_size = 9;
        h.seq_num = 123;
        h.msg_len = 5;
        h.seg_id = 3;
        h.ack_num = 23;
        h.cur_ack_vec = 0x0AC3;
        h.rst = 1;
        h.fin = 1;
        CHECK((int)sizeof(frame) >= arq__frame_len(h.seg_len));
        std::memset(frame, 0, sizeof(frame));
    }
    arq__frame_hdr_t h;
    int const frame_len = arq__frame_len(sizeof(seg));
    char const seg[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    char frame[64];
};

TEST(frame, write_returns_frame_len)
{
    struct Local
    {
        static arq_uint32_t StubChecksum(void const *, int) { return 0; }
    };

    Fixture f;
    int const written =
        arq__frame_write(&f.h, f.seg, &Local::StubChecksum, &f.frame, sizeof(f.frame));
    CHECK_EQUAL(arq__frame_len(sizeof(f.seg)), written);
}

int MockArqFrameHdrWrite(arq__frame_hdr_t *frame_hdr, void *out_buf)
{
    return mock().actualCall("arq__frame_hdr_write")
        .withParameter("frame_hdr", frame_hdr).withParameter("out_buf", out_buf)
        .returnIntValue();
}

int MockArqFrameSegWrite(void const *seg, void *out_buf, int len)
{
    return mock().actualCall("arq__frame_seg_write")
        .withParameter("seg", seg).withParameter("out_buf", out_buf).withParameter("len", len)
        .returnIntValue();
}

void MockArqFrameChecksumWrite(arq_checksum_cb_t checksum, void *checksum_seat, void *frame, int seg_len)
{
    mock().actualCall("arq__frame_checksum_write")
        .withParameter("checksum", (void *)checksum)
        .withParameter("checksum_seat", checksum_seat)
        .withParameter("frame", frame)
        .withParameter("seg_len", seg_len);
}

void MockArqCobsEncode(void *p, int len)
{
    mock().actualCall("arq__cobs_encode").withParameter("p", p).withParameter("len", len);
}

void MockArqFrameHdrRead(void const *buf, arq__frame_hdr_t *out_frame_hdr)
{
    mock().actualCall("arq__frame_hdr_read")
        .withParameter("buf", buf).withParameter("out_frame_hdr", out_frame_hdr);
}

arq__frame_read_result_t MockArqFrameChecksumRead(void const *frame,
                                                  int frame_len,
                                                  int seg_len,
                                                  arq_checksum_cb_t checksum)
{
    return (arq__frame_read_result_t)mock().actualCall("arq__frame_checksum_read")
                                           .withParameter("frame", frame)
                                           .withParameter("frame_len", frame_len)
                                           .withParameter("seg_len", seg_len)
                                           .withParameter("checksum", (void *)checksum)
                                           .returnIntValue();
}

void MockArqCobsDecode(void *p, int len)
{
    mock().actualCall("arq__cobs_decode").withParameter("p", p).withParameter("len", len);
}

uint32_t MockChecksum(void const *p, int len)
{
    return (uint32_t)mock().actualCall("checksum")
        .withParameter("p", p).withParameter("len", len).returnUnsignedIntValue();
}

uint32_t MockHtoN32(uint32_t x)
{
    return (uint32_t)mock().actualCall("arq__hton32").withParameter("x", x).returnUnsignedIntValue();
}

uint32_t MockNtoH32(uint32_t x)
{
    return (uint32_t)mock().actualCall("arq__ntoh32").withParameter("x", x).returnUnsignedIntValue();
}

struct MockFixture : Fixture
{
    MockFixture()
    {
        ARQ_MOCK_HOOK(arq__frame_hdr_write, MockArqFrameHdrWrite);
        ARQ_MOCK_HOOK(arq__frame_seg_write, MockArqFrameSegWrite);
        ARQ_MOCK_HOOK(arq__frame_checksum_write, MockArqFrameChecksumWrite);
        ARQ_MOCK_HOOK(arq__frame_hdr_read, MockArqFrameHdrRead);
        ARQ_MOCK_HOOK(arq__frame_checksum_read, MockArqFrameChecksumRead);
        ARQ_MOCK_HOOK(arq__cobs_encode, MockArqCobsEncode);
        ARQ_MOCK_HOOK(arq__cobs_decode, MockArqCobsDecode);
        ARQ_MOCK_HOOK(arq__ntoh32, MockNtoH32);
        ARQ_MOCK_HOOK(arq__hton32, MockHtoN32);
    }
};

TEST(frame, write_writes_frame_header_at_offset_1)
{
    MockFixture f;
    mock().expectOneCall("arq__frame_hdr_write")
          .withParameter("frame_hdr", &f.h)
          .withParameter("out_buf", (void *)&f.frame[1])
          .andReturnValue(ARQ__FRAME_HEADER_SIZE);
    mock().ignoreOtherCalls();
    f.h.seg_len = 0;
    arq__frame_write(&f.h, nullptr, &MockChecksum, f.frame, sizeof(f.frame));
}

TEST(frame, write_writes_segment_after_header)
{
    MockFixture f;
    ARQ_MOCK_UNHOOK(arq__frame_hdr_write);
    mock().expectOneCall("arq__frame_seg_write")
          .withParameter("seg", (void const *)f.seg)
          .withParameter("out_buf", (void *)(f.frame + 1 + ARQ__FRAME_HEADER_SIZE))
          .withParameter("len", f.h.seg_len);
    mock().ignoreOtherCalls();
    arq__frame_write(&f.h, f.seg, &MockChecksum, f.frame, sizeof(f.frame));
}

TEST(frame, write_writes_checksum_after_segment)
{
    MockFixture f;
    mock().expectOneCall("arq__frame_hdr_write").ignoreOtherParameters().andReturnValue(ARQ__FRAME_HEADER_SIZE);
    mock().expectOneCall("arq__frame_seg_write").ignoreOtherParameters().andReturnValue(f.h.seg_len);
    mock().expectOneCall("arq__frame_checksum_write")
          .withParameter("checksum", (void *)&MockChecksum)
          .withParameter("checksum_seat", (void *)(f.frame + 1 + ARQ__FRAME_HEADER_SIZE + f.h.seg_len))
          .withParameter("frame", (void *)f.frame)
          .withParameter("seg_len", f.h.seg_len);
    mock().ignoreOtherCalls();
    arq__frame_write(&f.h, f.seg, &MockChecksum, f.frame, sizeof(f.frame));
}

TEST(frame, write_encodes_frame_at_offset_zero)
{
    MockFixture f;
    mock().expectOneCall("arq__cobs_encode")
        .withParameter("p", (void *)f.frame)
        .withParameter("len", f.frame_len);
    mock().ignoreOtherCalls();
    arq__frame_write(&f.h, f.seg, &MockChecksum, f.frame, sizeof(f.frame));
}

TEST(frame, write_seg_copies_segment_into_buffer)
{
    Fixture f;
    arq__frame_seg_write(f.seg, f.frame, sizeof(f.seg));
    MEMCMP_EQUAL(f.seg, f.frame, sizeof(f.seg));
}

TEST(frame, checksum_write_computes_checksum_over_range_starting_at_offset_1)
{
    MockFixture f;
    ARQ_MOCK_UNHOOK(arq__frame_checksum_write);
    mock().expectOneCall("checksum")
          .withParameter("p", (void const *)&f.frame[1])
          .withParameter("len", ARQ__FRAME_HEADER_SIZE + f.h.seg_len);
    mock().ignoreOtherCalls();
    uint32_t dummy;
    arq__frame_checksum_write(MockChecksum, &dummy, f.frame, f.h.seg_len);
}

TEST(frame, checksum_write_converts_checksum_to_network_order)
{
    MockFixture f;
    ARQ_MOCK_UNHOOK(arq__frame_checksum_write);
    uint32_t const checksum = 0x12345678;
    mock().expectOneCall("checksum").ignoreOtherParameters().andReturnValue((int)checksum);
    mock().expectOneCall("arq__hton32").withParameter("x", checksum).andReturnValue(0);
    uint32_t dummy;
    arq__frame_checksum_write(MockChecksum, &dummy, f.frame, f.frame_len);
}

TEST(frame, checksum_write_writes_network_order_checksum_to_checksum_seat)
{
    MockFixture f;
    ARQ_MOCK_UNHOOK(arq__frame_checksum_write);
    uint32_t const checksum = 0x12345678;
    uint32_t const checksum_n = 0x78563412;
    mock().expectOneCall("checksum").ignoreOtherParameters().andReturnValue(checksum);
    mock().expectOneCall("arq__hton32").withParameter("x", checksum).andReturnValue(checksum_n);
    uint32_t checksum_actual;
    arq__frame_checksum_write(MockChecksum, &checksum_actual, f.frame, f.frame_len);
    CHECK_EQUAL(checksum_n, checksum_actual);
}

TEST(frame, read_decodes_frame)
{
    MockFixture f;
    mock().expectOneCall("arq__cobs_decode")
        .withParameter("p", (void *)f.frame).withParameter("len", f.frame_len);
    mock().ignoreOtherCalls();
    void const *seg;
    arq__frame_read(f.frame, f.frame_len, MockChecksum, &f.h, &seg);
}

TEST(frame, read_reads_checksum)
{
    MockFixture f;
    mock().expectOneCall("arq__frame_checksum_read")
        .withParameter("frame", (void const *)f.frame)
        .withParameter("frame_len", f.frame_len)
        .withParameter("seg_len", f.h.seg_len)
        .withParameter("checksum", (void *)MockChecksum)
        .andReturnValue(ARQ__FRAME_READ_RESULT_SUCCESS);
    mock().ignoreOtherCalls();
    void const *seg;
    arq__frame_read(f.frame, f.frame_len, MockChecksum, &f.h, &seg);
}

TEST(frame, checksum_read_calculates_checksum_over_header_and_segment)
{
    MockFixture f;
    ARQ_MOCK_UNHOOK(arq__frame_checksum_read);
    mock().expectOneCall("checksum")
        .withParameter("p", (void const *)&f.frame[1])
        .withParameter("len", ARQ__FRAME_HEADER_SIZE + f.h.seg_len)
        .andReturnValue(0);
    mock().ignoreOtherCalls();
    arq__frame_checksum_read(f.frame, f.frame_len, f.h.seg_len, MockChecksum);
}

TEST(frame, checksum_read_returns_malformed_if_checksum_doesnt_fit_in_frame)
{
    MockFixture f;
    ARQ_MOCK_UNHOOK(arq__frame_checksum_read);
    mock().ignoreOtherCalls();
    arq__frame_read_result_t const r =
        arq__frame_checksum_read(f.frame, f.frame_len, f.h.seg_len + 1, MockChecksum);
    CHECK_EQUAL(ARQ__FRAME_READ_RESULT_ERR_MALFORMED, r);
}

TEST(frame, checksum_read_returns_bad_checksum_if_computed_checksum_doesnt_match_frame_checksum)
{
    MockFixture f;
    ARQ_MOCK_UNHOOK(arq__frame_checksum_read);
    uint32_t const payload_checksum = 0xAAAAAAAA;
    uint32_t const computed_checksum = 0x22222222;
    mock().expectOneCall("checksum").ignoreOtherParameters().andReturnValue(computed_checksum);
    mock().ignoreOtherCalls();
    std::memcpy(&f.frame[1 + ARQ__FRAME_HEADER_SIZE + f.h.seg_len], &payload_checksum, 4);
    arq__frame_read_result_t const r =
        arq__frame_checksum_read(f.frame, f.frame_len, f.h.seg_len, MockChecksum);
    CHECK_EQUAL(ARQ__FRAME_READ_RESULT_ERR_CHECKSUM, r);
}

TEST(frame, checksum_read_returns_success_if_computed_checksum_matches_frame_checksum)
{
    MockFixture f;
    ARQ_MOCK_UNHOOK(arq__frame_checksum_read);
    uint32_t const checksum = 0x11223344;
    mock().expectOneCall("checksum").ignoreOtherParameters().andReturnValue(checksum);
    mock().expectOneCall("arq__ntoh32").ignoreOtherParameters().andReturnValue(checksum);
    mock().ignoreOtherCalls();
    arq__frame_read_result_t const r =
        arq__frame_checksum_read(f.frame, f.frame_len, f.h.seg_len, MockChecksum);
    CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, r);
}

TEST(frame, read_reads_frame_header_at_offset_1)
{
    MockFixture f;
    mock().expectOneCall("arq__frame_hdr_read")
          .withParameter("buf", (void const *)&f.frame[1])
          .ignoreOtherParameters();
    mock().ignoreOtherCalls();
    void const *seg;
    arq__frame_read(f.frame, f.frame_len, MockChecksum, &f.h, &seg);
}

TEST(frame, read_points_out_seg_to_segment_in_frame)
{
    MockFixture f;
    mock().ignoreOtherCalls();
    void const *seg;
    arq__frame_read(f.frame, f.frame_len, MockChecksum, &f.h, &seg);
    CHECK_EQUAL((void *)&f.frame[1 + ARQ__FRAME_HEADER_SIZE], seg);
}

}

