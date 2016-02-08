#include "nanoarq_in_test_project.h"
#include "nanoarq_hook_plugin.h"

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <cstring>

TEST_GROUP(frame) {};

namespace
{

TEST(frame, size_is_header_size_plus_segment_length_plus_cobs_overhead)
{
    int seg_len = 123;
    CHECK_EQUAL(NANOARQ_FRAME_COBS_OVERHEAD + NANOARQ_FRAME_HEADER_SIZE + seg_len,
                arq__frame_size(seg_len));
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
        h.ack_seg_mask = 0x0AC3;
        h.rst = 1;
        h.fin = 1;
        CHECK((int)sizeof(frame) >= arq__frame_size(h.seg_len));
    }
    arq__frame_hdr_t h;
    int const frame_len = arq__frame_size(sizeof(seg));
    char const seg[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    char frame[64];
};

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

void MockArqFrameChecksumWrite(arq_checksum_cb_t checksum, void *checksum_seat, void *frame, int len)
{
    mock().actualCall("arq__frame_checksum_write")
        .withParameter("checksum", (void *)checksum)
        .withParameter("checksum_seat", checksum_seat)
        .withParameter("frame", frame)
        .withParameter("len", len);
}

void MockArqFrameEncode(void *frame, int len)
{
    mock().actualCall("arq__frame_encode").withParameter("frame", frame).withParameter("len", len);
}

void MockArqFrameDecode(void *frame, int len)
{
    mock().actualCall("arq__frame_decode").withParameter("frame", frame).withParameter("len", len);
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

void MockArqCobsDecode(void *p, int len)
{
    mock().actualCall("arq__cobs_decode").withParameter("p", p).withParameter("len", len);
}

uint32_t MockChecksum(void const *p, int len)
{
    return (uint32_t)mock().actualCall("checksum")
        .withParameter("p", p).withParameter("len", len)
        .returnIntValue();
}

uint32_t MockHtoN32(uint32_t x)
{
    return (uint32_t)mock().actualCall("arq__hton32").withParameter("x", x).returnIntValue();
}

uint32_t MockNtoH32(uint32_t x)
{
    return (uint32_t)mock().actualCall("arq__ntoh32").withParameter("x", x).returnIntValue();
}

struct MockFixture : Fixture
{
    MockFixture()
    {
        NANOARQ_MOCK_HOOK(arq__frame_hdr_write, MockArqFrameHdrWrite);
        NANOARQ_MOCK_HOOK(arq__frame_seg_write, MockArqFrameSegWrite);
        NANOARQ_MOCK_HOOK(arq__frame_checksum_write, MockArqFrameChecksumWrite);
        NANOARQ_MOCK_HOOK(arq__frame_encode, MockArqFrameEncode);
        NANOARQ_MOCK_HOOK(arq__frame_hdr_read, MockArqFrameHdrRead);
        NANOARQ_MOCK_HOOK(arq__frame_decode, MockArqFrameDecode);
        NANOARQ_MOCK_HOOK(arq__ntoh32, MockNtoH32);
        NANOARQ_MOCK_HOOK(arq__hton32, MockHtoN32);
    }
};

TEST(frame, write_writes_frame_header_at_offset_1)
{
    MockFixture f;
    mock().expectOneCall("arq__frame_hdr_write")
          .withParameter("frame_hdr", &f.h)
          .withParameter("out_buf", (void *)&f.frame[1])
          .andReturnValue(NANOARQ_FRAME_HEADER_SIZE);
    mock().ignoreOtherCalls();
    f.h.seg_len = 0;
    arq__frame_write(&f.h, nullptr, &MockChecksum, f.frame, sizeof(f.frame));
}

TEST(frame, write_writes_segment_after_header)
{
    MockFixture f;
    NANOARQ_MOCK_UNHOOK(arq__frame_hdr_write);
    mock().expectOneCall("arq__frame_seg_write")
          .withParameter("seg", (void const *)f.seg)
          .withParameter("out_buf", (void *)(f.frame + 1 + NANOARQ_FRAME_HEADER_SIZE))
          .withParameter("len", f.h.seg_len);
    mock().ignoreOtherCalls();
    arq__frame_write(&f.h, f.seg, &MockChecksum, f.frame, sizeof(f.frame));
}

TEST(frame, write_writes_checksum_after_segment)
{
    MockFixture f;
    mock().expectOneCall("arq__frame_hdr_write").ignoreOtherParameters().andReturnValue(NANOARQ_FRAME_HEADER_SIZE);
    mock().expectOneCall("arq__frame_seg_write").ignoreOtherParameters().andReturnValue(f.h.seg_len);
    mock().expectOneCall("arq__frame_checksum_write")
          .withParameter("checksum", (void *)&MockChecksum)
          .withParameter("checksum_seat", (void *)(f.frame + 1 + NANOARQ_FRAME_HEADER_SIZE + f.h.seg_len))
          .withParameter("frame", (void *)f.frame)
          .withParameter("len", f.frame_len);
    mock().ignoreOtherCalls();
    arq__frame_write(&f.h, f.seg, &MockChecksum, f.frame, sizeof(f.frame));
}

TEST(frame, write_encodes_frame_at_offset_zero)
{
    MockFixture f;
    mock().expectOneCall("arq__frame_encode")
        .withParameter("frame", (void *)f.frame)
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
    NANOARQ_MOCK_UNHOOK(arq__frame_checksum_write);
    mock().expectOneCall("checksum")
          .withParameter("p", (void const *)&f.frame[1])
          .withParameter("len", f.frame_len - 1 - 4);
    mock().ignoreOtherCalls();
    uint32_t dummy;
    arq__frame_checksum_write(MockChecksum, &dummy, f.frame, f.frame_len);
}

TEST(frame, checksum_write_converts_checksum_to_network_order)
{
    MockFixture f;
    NANOARQ_MOCK_UNHOOK(arq__frame_checksum_write);
    uint32_t const checksum = 0x12345678;
    mock().expectOneCall("checksum").ignoreOtherParameters().andReturnValue((int)checksum);
    mock().expectOneCall("arq__hton32").withParameter("x", checksum).andReturnValue(0);
    uint32_t dummy;
    arq__frame_checksum_write(MockChecksum, &dummy, f.frame, f.frame_len);
}

TEST(frame, checksum_write_writes_network_order_checksum_to_checksum_seat)
{
    MockFixture f;
    NANOARQ_MOCK_UNHOOK(arq__frame_checksum_write);
    uint32_t const checksum = 0x12345678;
    uint32_t const checksum_n = 0x78563412;
    mock().expectOneCall("checksum").ignoreOtherParameters().andReturnValue((int)checksum);
    mock().expectOneCall("arq__hton32").withParameter("x", checksum).andReturnValue((int)checksum_n);
    uint32_t checksum_actual;
    arq__frame_checksum_write(MockChecksum, &checksum_actual, f.frame, f.frame_len);
    CHECK_EQUAL(checksum_n, checksum_actual);
}

TEST(frame, frame_encode_forwards_to_cobs_encode)
{
    void *p = (void *)0x12345678;
    int const len = 54321;
    NANOARQ_MOCK_HOOK(arq__cobs_encode, MockArqCobsEncode);
    mock().expectOneCall("arq__cobs_encode").withParameter("p", p).withParameter("len", len);
    arq__frame_encode(p, len);
}

TEST(frame, frame_decode_forwards_to_cobs_decode)
{
    void *p = (void *)0x12345678;
    int const len = 54321;
    NANOARQ_MOCK_HOOK(arq__cobs_decode, MockArqCobsDecode);
    mock().expectOneCall("arq__cobs_decode").withParameter("p", p).withParameter("len", len);
    arq__frame_decode(p, len);
}

TEST(frame, read_decodes_frame)
{
    MockFixture f;
    mock().expectOneCall("arq__frame_decode").withParameter("frame", (void *)f.frame).withParameter("len", f.frame_len);
    mock().ignoreOtherCalls();
    void const *seg;
    arq__frame_read(f.frame, f.frame_len, &f.h, &seg);
}

TEST(frame, read_reads_frame_header_at_offset_1)
{
    MockFixture f;
    mock().expectOneCall("arq__frame_hdr_read")
          .withParameter("buf", (void const *)&f.frame[1])
          .ignoreOtherParameters();
    mock().ignoreOtherCalls();
    void const *seg;
    arq__frame_read(f.frame, f.frame_len, &f.h, &seg);
}

TEST(frame, read_points_out_seg_to_segment_in_frame)
{
    Fixture f;
    arq__frame_hdr_write(&f.h, &f.frame[1]);
    std::memcpy(&f.frame[1 + NANOARQ_FRAME_HEADER_SIZE], f.seg, sizeof(f.seg));
    void const *rseg;
    arq__frame_hdr_t rh;
    arq__frame_read(f.frame, f.frame_len, &rh, &rseg);
    CHECK_EQUAL(rseg, (void const *)&f.frame[1 + NANOARQ_FRAME_HEADER_SIZE]);
}

}

