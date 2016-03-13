#include "nanoarq_unit_test.h"
#include <CppUTest/TestHarness.h>
#include <array>
#include <vector>

TEST_GROUP(recv_frame) {};

namespace {

TEST(recv_frame, init_assigns_parameters)
{
    arq__recv_frame_t f;
    f.cap = f.len = 12345;
    f.buf = nullptr;
    arq__recv_frame_init(&f, &f, 100);
    CHECK_EQUAL(100, f.cap);
    CHECK_EQUAL((void *)&f, (void *)f.buf);
}

TEST(recv_frame, init_zeroes_len_and_state)
{
    arq__recv_frame_t f;
    void *buf = &f;
    arq__recv_frame_init(&f, buf, 100);
    CHECK_EQUAL(0, f.len);
    CHECK_EQUAL(ARQ__RECV_FRAME_STATE_ACCUMULATING_FRAME, f.state);
}

struct Fixture
{
    Fixture()
    {
        arq__recv_frame_init(&f, buf.data(), buf.size());
    }
    arq__recv_frame_t f;
    std::array< arq_uchar_t, 254 > buf;
    std::vector< arq_uchar_t > fill;
};

void PopulateFill(Fixture &f, int bytes, bool end_with_zero)
{
    f.fill.resize(bytes);
    auto i = 0u;
    for (auto const n = arq__sub_sat(bytes, 1); i < n; ++i) {
        f.fill[i] = (arq_uchar_t)i + 1;
    }
    f.fill[i] = end_with_zero ? 0x00 : 0xFF;
}

TEST(recv_frame, fill_buffer_without_zeroes_stays_in_accumulating_state)
{
    Fixture f;
    PopulateFill(f, 19, false);
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    CHECK_EQUAL(ARQ__RECV_FRAME_STATE_ACCUMULATING_FRAME, f.f.state);
}

TEST(recv_frame, fill_buffer_without_zeroes_sets_len)
{
    Fixture f;
    PopulateFill(f, 19, false);
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    CHECK_EQUAL(f.fill.size(), f.f.len);
}

TEST(recv_frame, fill_buffer_without_zeroes_copies_buffer_in)
{
    Fixture f;
    PopulateFill(f, 117, false);
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    MEMCMP_EQUAL(f.fill.data(), f.f.buf, f.fill.size());
}

TEST(recv_frame, fill_buffer_without_zeroes_doesnt_overflow_buffer)
{
    Fixture f;
    std::array< arq_uchar_t, f.buf.size() + 1 > sentinel_buf;
    sentinel_buf[sentinel_buf.size() - 1] = 0x25;
    f.f.buf = sentinel_buf.data();
    PopulateFill(f, f.buf.size() * 2, false);
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    CHECK_EQUAL(0x25, sentinel_buf[sentinel_buf.size() - 1]);
}

TEST(recv_frame, fill_multiple_buffers_without_zeroes_accumulates_data_into_frame)
{
    Fixture f;
    PopulateFill(f, 19, false);
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    MEMCMP_EQUAL(f.fill.data(), f.f.buf, f.fill.size());
    MEMCMP_EQUAL(f.fill.data(), &f.f.buf[f.fill.size()], f.fill.size());
    MEMCMP_EQUAL(f.fill.data(), &f.f.buf[f.fill.size() * 2], f.fill.size());
}

TEST(recv_frame, fill_multiple_buffers_without_zeroes_accumulates_into_frame_len)
{
    Fixture f;
    PopulateFill(f, 19, false);
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    CHECK_EQUAL(f.fill.size() * 3, f.f.len);
}

TEST(recv_frame, fill_multiple_buffers_without_zeroes_doesnt_overflow_buffer)
{
    Fixture f;
    std::array< arq_uchar_t, f.buf.size() + 1 > sentinel_buf;
    sentinel_buf[sentinel_buf.size() - 1] = 0x25;
    f.f.buf = sentinel_buf.data();
    PopulateFill(f, 100, false);
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    CHECK_EQUAL(0x25, sentinel_buf[sentinel_buf.size() - 1]);
}

}

