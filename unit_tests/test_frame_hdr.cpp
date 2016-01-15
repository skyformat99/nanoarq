#include "nanoarq_in_test_project.h"
#include "CppUTest/TestHarness.h"

TEST_GROUP(frame_hdr) {};

namespace
{

struct Fixture
{
    enum
    {
        Version = 123,
        SegmentLen = 44,
        WindowSize = 19
    };

    Fixture()
    {
        uint8_t *src = buf;
        *src++ = (uint8_t)Version;
        *src++ = (uint8_t)SegmentLen;
        src++; // flags
        *src++ = (uint8_t)WindowSize;
    }

    arq__frame_hdr_t h;
    uint8_t buf[32];
};

TEST(frame_hdr, read_version)
{
    Fixture f;
    arq__frame_hdr_read(&f.buf, &f.h);
    CHECK_EQUAL(Fixture::Version, f.h.version);
}

TEST(frame_hdr, read_segment_length)
{
    Fixture f;
    arq__frame_hdr_read(&f.buf, &f.h);
    CHECK_EQUAL(Fixture::SegmentLen, f.h.seg_len);
}

TEST(frame_hdr, read_rst_flag)
{
    Fixture f;
    f.buf[2] = 1 << 1;
    arq__frame_hdr_read(&f.buf, &f.h);
    CHECK_EQUAL(1, (int)f.h.rst);

    f.buf[2] = 0;
    arq__frame_hdr_read(&f.buf, &f.h);
    CHECK_EQUAL(0, (int)f.h.rst);
}

TEST(frame_hdr, read_fin_flag)
{
    Fixture f;
    f.buf[2] = 1;
    arq__frame_hdr_read(&f.buf, &f.h);
    CHECK_EQUAL(1, (int)f.h.fin);

    f.buf[2] = 0;
    arq__frame_hdr_read(&f.buf, &f.h);
    CHECK_EQUAL(0, (int)f.h.fin);
}

TEST(frame_hdr, read_window_size)
{
    Fixture f;
    arq__frame_hdr_read(&f.buf, &f.h);
    CHECK_EQUAL(Fixture::WindowSize, f.h.win_size);
}

}

