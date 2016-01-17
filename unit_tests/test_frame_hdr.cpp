#include "nanoarq_in_test_project.h"
#include "CppUTest/TestHarness.h"

TEST_GROUP(frame_hdr) {};

namespace
{

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
        arq__frame_hdr_read(&buf, &h);
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
    arq__frame_hdr_read(&f.buf, &f.h);
    CHECK_EQUAL(1, (int)f.h.rst);
    f.buf[2] = 0;
    arq__frame_hdr_read(&f.buf, &f.h);
    CHECK_EQUAL(0, (int)f.h.rst);
}

TEST(frame_hdr, read_fin_flag)
{
    ReadFixture f;
    f.buf[2] = 1;
    arq__frame_hdr_read(&f.buf, &f.h);
    CHECK_EQUAL(1, (int)f.h.fin);
    f.buf[2] = 0;
    arq__frame_hdr_read(&f.buf, &f.h);
    CHECK_EQUAL(0, (int)f.h.fin);
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
    CHECK_EQUAL(AckSegmentMask, f.h.ack_seg_mask);
}

TEST(frame_hdr, returns_header_size_in_bytes)
{
    ReadFixture f;
    int const n = arq__frame_hdr_read(&f.buf, &f.h);
    CHECK_EQUAL(12, n);
}

struct WriteFixture
{
    WriteFixture()
    {
        h.version = Version;
        h.seg_len = SegmentLength;
        h.rst = 0;
        h.fin = 0;
        h.win_size = WindowSize;
        h.seq_num = SequenceNumber;
        h.msg_len = MessageLength;
        h.seg_id = SegmentID;
        h.ack_num = AckNumber;
        h.ack_seg_mask = AckSegmentMask;
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
    CHECK_EQUAL(2, f.buf[2] & 2);
    f.h.rst = 0;
    arq__frame_hdr_write(&f.h, f.buf);
    CHECK_EQUAL(0, f.buf[2] & 2);
}

TEST(frame_hdr, write_fin_flag)
{
    WriteFixture f;
    f.h.fin = 1;
    arq__frame_hdr_write(&f.h, f.buf);
    CHECK_EQUAL(1, f.buf[2] & 1);
    f.h.fin = 0;
    arq__frame_hdr_write(&f.h, f.buf);
    CHECK_EQUAL(0, f.buf[2] & 1);
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
}

TEST(frame_hdr, write_ack_number)
{
    WriteFixture f;
}

TEST(frame_hdr, write_ack_segment_mask)
{
    WriteFixture f;
}

}

