#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <array>
#include <vector>
#include <algorithm>

TEST_GROUP(recv_frame) {};

namespace {

TEST(recv_frame, reset_zeroes_len_and_state)
{
    arq__recv_frame_t f;
    f.len = 1;
    f.state = (arq__recv_frame_state_t)(ARQ__RECV_FRAME_STATE_ACCUMULATING + 1);
    arq__recv_frame_rst(&f);
    CHECK_EQUAL(0, f.len);
    CHECK_EQUAL(ARQ__RECV_FRAME_STATE_ACCUMULATING, f.state);
}

TEST(recv_frame, init_assigns_parameters)
{
    arq__recv_frame_t f;
    f.cap = f.len = 12345;
    arq__recv_frame_init(&f, 100);
    CHECK_EQUAL(100, f.cap);
}

void MockFrameReset(arq__recv_frame_t *f)
{
    mock().actualCall("arq__recv_frame_rst").withParameter("f", f);
}

TEST(recv_frame, init_resets_frame)
{
    ARQ_MOCK_HOOK(arq__recv_frame_rst, MockFrameReset);
    arq__recv_frame_t f;
    mock().expectOneCall("arq__recv_frame_rst").withParameter("f", &f);
    arq__recv_frame_rst(&f);
}

struct Fixture
{
	enum { BUFSIZE = 254 };
    Fixture()
    {
        f.buf = buf.data();
        arq__recv_frame_init(&f, buf.size());
        arq__recv_frame_rst(&f);
        std::fill(std::begin(buf), std::end(buf), 0xFF);
    }
    arq__recv_frame_t f;
    std::array< arq_uchar_t, BUFSIZE > buf;
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
    CHECK_EQUAL(ARQ__RECV_FRAME_STATE_ACCUMULATING, f.f.state);
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
    std::array< arq_uchar_t, Fixture::BUFSIZE + 1 > sentinel_buf;
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
    std::array< arq_uchar_t, Fixture::BUFSIZE + 1 > sentinel_buf;
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

TEST(recv_frame, fill_returns_len_when_entire_buffer_fits_in_frame)
{
    Fixture f;
    PopulateFill(f, 19, false);
    unsigned const written = arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    CHECK_EQUAL(19, written);
}

TEST(recv_frame, fill_returns_bytes_written_when_buffer_partially_fits_in_frame)
{
    Fixture f;
    PopulateFill(f, f.f.cap - 13, false);
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    unsigned const written = arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    CHECK_EQUAL(13, written);
}

TEST(recv_frame, fill_returns_zero_when_frame_is_full_before_call_occurs)
{
    Fixture f;
    PopulateFill(f, 127, false);
    f.f.len = f.f.cap;
    unsigned const written = arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    CHECK_EQUAL(0, written);
}

TEST(recv_frame, fill_returns_zero_when_full_frame_is_already_accumulated)
{
    Fixture f;
    PopulateFill(f, 1, false);
    f.f.len = 24;
    f.f.state = ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT;
    unsigned const written = arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    CHECK_EQUAL(0, written);
}

TEST(recv_frame, fill_changes_state_to_full_frame_present_if_payload_ends_with_zero)
{
    Fixture f;
    PopulateFill(f, 32, true);
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    CHECK_EQUAL(ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT, f.f.state);
}

TEST(recv_frame, fill_len_is_accumulated_to_index_of_first_zero_in_payload)
{
    Fixture f;
    PopulateFill(f, 32, true);
    f.fill.emplace_back(0xFF);
    f.fill.emplace_back(0xFF);
    f.fill.emplace_back(0xFF);
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    CHECK_EQUAL(32, f.f.len);
}

TEST(recv_frame, fill_copies_terminating_zero_into_frame_buf)
{
    Fixture f;
    PopulateFill(f, 32, true);
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    CHECK_EQUAL(0x00, f.f.buf[31]);
}

TEST(recv_frame, fill_changes_state_to_full_frame_present_if_payload_contains_zero)
{
    Fixture f;
    PopulateFill(f, 32, true);
    f.fill.emplace_back(0xFF);
    f.fill.emplace_back(0xFF);
    f.fill.emplace_back(0xFF);
    arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    CHECK_EQUAL(ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT, f.f.state);
}

TEST(recv_frame, fill_returns_index_of_first_zero_found_when_payload_ends_with_zero)
{
    Fixture f;
    PopulateFill(f, 32, true);
    unsigned const written = arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    CHECK_EQUAL(32, written);
}

TEST(recv_frame, fill_returns_index_of_first_zero_found_when_payload_contains_zero)
{
    Fixture f;
    PopulateFill(f, 32, true);
    for (auto i = 0; i < 32; ++i) {
        f.fill.emplace_back(0xFE);
    }
    unsigned const written = arq__recv_frame_fill(&f.f, f.fill.data(), f.fill.size());
    CHECK_EQUAL(32, written);
}

}

