#include "arq_in_unit_tests.h"
#include <CppUTest/TestHarness.h>
#include <array>
#include <cstring>

TEST_GROUP(next_poll) {};

namespace {

struct Fixture
{
    Fixture()
    {
        sw.rtx = rtx.data();
        std::memset(rtx.data(), 0, sizeof(arq_time_t) * rtx.size());
        sw.w.cap = rtx.size();
        sw.w.size = 0;
        sw.tiny_on = 0;
    };
    arq__send_wnd_t sw;
    std::array< arq_time_t, 16 > rtx;
    arq__recv_wnd_t rw;
};

TEST(next_poll, returns_infinity_if_send_window_empty_and_tinygram_timer_off)
{
    Fixture f;
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw);
    CHECK_EQUAL(ARQ_TIME_INFINITY, t);
}

TEST(next_poll, returns_only_rtx_timer)
{
    Fixture f;
    f.sw.w.size = 1;
    f.rtx[0] = 13;
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw);
    CHECK_EQUAL(13, t);
}

TEST(next_poll, returns_smallest_rtx_timer)
{
    Fixture f;
    f.sw.w.size = 14;
    for (auto i = 0u; i < f.sw.w.size; ++i) {
        f.rtx[i] = 14 - i;
    }
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw);
    CHECK_EQUAL(1, t);
}

TEST(next_poll, looks_at_rtx_timers_inside_of_active_send_window)
{
    Fixture f;
    f.sw.w.size = f.rtx.size();
    for (auto i = 0u; i < f.sw.w.size; ++i) {
        f.rtx[i] = 2;
    }
    f.sw.w.seq = 5;
    f.sw.w.size = 2;
    f.rtx[5] = 0;
    f.rtx[6] = 1;
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw);
    CHECK_EQUAL(1, t);
}

TEST(next_poll, doesnt_return_expired_timers)
{
    Fixture f;
    f.sw.w.size = 10;
    f.rtx[7] = 123;
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw);
    CHECK_EQUAL(123, t);
}

TEST(next_poll, returns_tinygram_expiry_time_if_tinygram_timer_on)
{
    Fixture f;
    f.sw.tiny_on = 1;
    f.sw.tiny = 93;
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw);
    CHECK_EQUAL(93, t);
}

TEST(next_poll, returns_smallest_rtx_if_tinygram_timer_on_but_larger_than_rtx)
{
    Fixture f;
    f.sw.tiny_on = 1;
    f.sw.tiny = 100;
    f.sw.w.size = 1;
    f.rtx[0] = 99;
    arq_time_t const t = arq__next_poll(&f.sw, &f.rw);
    CHECK_EQUAL(99, t);
}

}

