#include "arq_in_unit_tests.h"
#include <CppUTest/TestHarness.h>
#include <cstring>

TEST_GROUP(frame_hdr) {};

namespace {

enum
{
    Version = 123,
    SegmentLength = 44,
    WindowSize = 19,
    SequenceNumber = 0x5AF, // 0101 1010 1111
    MessageLength = 0xC3,   // 1100 0011
    SegmentID = 0xA71,      // 1010 0111 0001
    AckNumber = 0x814,      // 1000 0001 0100
    AckSegmentMask = 0x235  // 0010 0011 0101
};

TEST(frame_hdr, init_sets_all_fields_to_zero)
{
    arq__frame_hdr_t h;
    std::memset(&h, 0xFF, sizeof(h));
    arq__frame_hdr_init(&h);
    CHECK_EQUAL(0, h.version);
    CHECK_EQUAL(0, h.seg_len);
    CHECK_EQUAL(0, h.win_size);
    CHECK_EQUAL(0, h.seq_num);
    CHECK_EQUAL(0, h.msg_len);
    CHECK_EQUAL(0, h.seg_id);
    CHECK_EQUAL(0, h.ack_num);
    CHECK_EQUAL(0, h.cur_ack_vec);
    CHECK_EQUAL(0, h.rst);
    CHECK_EQUAL(0, h.fin);
    CHECK_EQUAL(0, h.ack);
    CHECK_EQUAL(0, h.seg);
}

struct ReadFixture
{
    ReadFixture()
    {
        uint8_t *src = buf;
        *src++ = (uint8_t)Version;
        *src++ = (uint8_t)SegmentLength;
        *src++ = 0;
        *src++ = (uint8_t)WindowSize;
        *src++ = 0x5A; *src++ = 0xFC; *src++ = 0x3A; *src++ = 0x71;
        *src++ = 0x81; *src++ = 0x40; *src++ = 0x02; *src++ = 0x35;
        arq__frame_hdr_read(buf, &h);
    }

    arq__frame_hdr_t h;
    uint8_t buf[32];
};

TEST(frame_hdr, read_version)
{
    ReadFixture f;
    CHECK_EQUAL(Version, f.h.version);
}

TEST(frame_hdr, read_segment_length)
{
    ReadFixture f;
    CHECK_EQUAL(SegmentLength, f.h.seg_len);
}

TEST(frame_hdr, read_rst_flag)
{
    ReadFixture f;
    f.buf[2] = 1 << 1;
    arq__frame_hdr_read(f.buf, &f.h);
    CHECK_EQUAL(1, (int)f.h.rst);
    f.buf[2] = 0;
    arq__frame_hdr_read(f.buf, &f.h);
    CHECK_EQUAL(0, (int)f.h.rst);
}

TEST(frame_hdr, read_fin_flag)
{
    ReadFixture f;
    f.buf[2] = 1;
    arq__frame_hdr_read(f.buf, &f.h);
    CHECK_EQUAL(1, (int)f.h.fin);
    f.buf[2] = 0;
    arq__frame_hdr_read(f.buf, &f.h);
    CHECK_EQUAL(0, (int)f.h.fin);
}

TEST(frame_hdr, read_ack_flag)
{
    ReadFixture f;
    f.buf[2] = 1 << 2;
    arq__frame_hdr_read(f.buf, &f.h);
    CHECK_EQUAL(1, (int)f.h.ack);
    f.buf[2] = 0;
    arq__frame_hdr_read(f.buf, &f.h);
    CHECK_EQUAL(0, (int)f.h.ack);
}

TEST(frame_hdr, read_seg_flag)
{
    ReadFixture f;
    f.buf[2] = 1 << 3;
    arq__frame_hdr_read(f.buf, &f.h);
    CHECK_EQUAL(1, (int)f.h.seg);
    f.buf[2] = 0;
    arq__frame_hdr_read(f.buf, &f.h);
    CHECK_EQUAL(0, (int)f.h.seg);
}

TEST(frame_hdr, read_window_size)
{
    ReadFixture f;
    CHECK_EQUAL(WindowSize, f.h.win_size);
}

TEST(frame_hdr, read_sequence_number)
{
    ReadFixture f;
    CHECK_EQUAL(SequenceNumber, f.h.seq_num);
}

TEST(frame_hdr, read_message_length)
{
    ReadFixture f;
    CHECK_EQUAL(MessageLength, f.h.msg_len);
}

TEST(frame_hdr, read_segment_id)
{
    ReadFixture f;
    CHECK_EQUAL(SegmentID, f.h.seg_id);
}

TEST(frame_hdr, read_ack_number)
{
    ReadFixture f;
    CHECK_EQUAL(AckNumber, f.h.ack_num);
}

TEST(frame_hdr, read_ack_segment_mask)
{
    ReadFixture f;
    CHECK_EQUAL(AckSegmentMask, f.h.cur_ack_vec);
}

struct WriteFixture
{
    WriteFixture()
    {
        h.version = Version;
        h.seg_len = SegmentLength;
        h.rst = 0;
        h.fin = 0;
        h.seg = 0;
        h.ack = 0;
        h.win_size = WindowSize;
        h.seq_num = SequenceNumber;
        h.msg_len = MessageLength;
        h.seg_id = SegmentID;
        h.ack_num = AckNumber;
        h.cur_ack_vec = AckSegmentMask;
        arq__frame_hdr_write(&h, buf);
    }

    arq__frame_hdr_t h;
    char buf[32];
};

TEST(frame_hdr, write_version)
{
    WriteFixture f;
    CHECK_EQUAL((uint8_t)Version, (uint8_t)f.buf[0]);
}

TEST(frame_hdr, write_segment_length)
{
    WriteFixture f;
    CHECK_EQUAL((uint8_t)SegmentLength, (uint8_t)f.buf[1]);
}

TEST(frame_hdr, write_rst_flag)
{
    WriteFixture f;
    f.h.rst = 1;
    arq__frame_hdr_write(&f.h, f.buf);
    CHECK(f.buf[2] & 2);
    f.h.rst = 0;
    arq__frame_hdr_write(&f.h, f.buf);
    CHECK_EQUAL(0, f.buf[2] & 2);
}

TEST(frame_hdr, write_fin_flag)
{
    WriteFixture f;
    f.h.fin = 1;
    arq__frame_hdr_write(&f.h, f.buf);
    CHECK(f.buf[2] & 1);
    f.h.fin = 0;
    arq__frame_hdr_write(&f.h, f.buf);
    CHECK_EQUAL(0, f.buf[2] & 1);
}

TEST(frame_hdr, write_ack_flag)
{
    WriteFixture f;
    f.h.ack = 1;
    arq__frame_hdr_write(&f.h, f.buf);
    CHECK(f.buf[2] & 4);
    f.h.ack = 0;
    arq__frame_hdr_write(&f.h, f.buf);
    CHECK_EQUAL(0, f.buf[2] & 4);
}

TEST(frame_hdr, write_seg_flag)
{
    WriteFixture f;
    f.h.seg = 1;
    arq__frame_hdr_write(&f.h, f.buf);
    CHECK(f.buf[2] & 8);
    f.h.seg = 0;
    arq__frame_hdr_write(&f.h, f.buf);
    CHECK_EQUAL(0, f.buf[2] & 8);
}

TEST(frame_hdr, write_window_size)
{
    WriteFixture f;
    CHECK_EQUAL((uint8_t)WindowSize, (uint8_t)f.buf[3]);
}

TEST(frame_hdr, write_sequence_number)
{
    WriteFixture f;
    CHECK_EQUAL((uint8_t)0x5A, (uint8_t)f.buf[4]);
    CHECK_EQUAL((uint8_t)0xF0, (uint8_t)f.buf[5] & 0xF0);
}

TEST(frame_hdr, write_message_length)
{
    WriteFixture f;
    CHECK_EQUAL((uint8_t)0x0C, (uint8_t)f.buf[5] & 0x0F);
    CHECK_EQUAL((uint8_t)0x30, (uint8_t)f.buf[6] & 0xF0);
}

TEST(frame_hdr, write_segment_id)
{
    WriteFixture f;
    CHECK_EQUAL((uint8_t)0x0A, (uint8_t)f.buf[6] & 0x0F);
    CHECK_EQUAL((uint8_t)0x71, (uint8_t)f.buf[7]);
}

TEST(frame_hdr, write_ack_number)
{
    WriteFixture f;
    CHECK_EQUAL((uint8_t)0x81, (uint8_t)f.buf[8]);
    CHECK_EQUAL((uint8_t)0x40, (uint8_t)f.buf[9] & 0xF0);
}

TEST(frame_hdr, write_ack_segment_mask)
{
    WriteFixture f;
    CHECK_EQUAL((uint8_t)0x02, (uint8_t)f.buf[10] & 0x0F);
    CHECK_EQUAL((uint8_t)0x35, (uint8_t)f.buf[11]);
}

TEST(frame_hdr, write_returns_bytes_written)
{
    WriteFixture f;
    int const n = arq__frame_hdr_write(&f.h, f.buf);
    CHECK_EQUAL(ARQ__FRAME_HEADER_SIZE, n);
}

TEST(frame_hdr, headers_identical)
{
    arq__frame_hdr_t orig;
    orig.version = Version;
    orig.seg_len = SegmentLength;
    orig.rst = 1;
    orig.fin = 0;
    orig.win_size = WindowSize;
    orig.seq_num = SequenceNumber;
    orig.msg_len = MessageLength;
    orig.seg_id = SegmentID;
    orig.ack_num = AckNumber;
    orig.cur_ack_vec = AckSegmentMask;

    char buf[ARQ__FRAME_HEADER_SIZE];
    arq__frame_hdr_write(&orig, buf);
    arq__frame_hdr_t actual;
    arq__frame_hdr_read(buf, &actual);

    CHECK_EQUAL(orig.version, actual.version);
    CHECK_EQUAL(orig.seg_len, actual.seg_len);
    CHECK_EQUAL(orig.rst, actual.rst);
    CHECK_EQUAL(orig.fin, actual.fin);
    CHECK_EQUAL(orig.win_size, actual.win_size);
    CHECK_EQUAL(orig.seq_num, actual.seq_num);
    CHECK_EQUAL(orig.msg_len, actual.msg_len);
    CHECK_EQUAL(orig.seg_id, actual.seg_id);
    CHECK_EQUAL(orig.ack_num, actual.ack_num);
    CHECK_EQUAL(orig.cur_ack_vec, actual.cur_ack_vec);
}

}

