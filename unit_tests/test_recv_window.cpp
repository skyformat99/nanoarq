#include "nanoarq_unit_test.h"
#include "nanoarq_hook_plugin.h"
#include <CppUTestExt/MockSupport.h>
#include <CppUTest/TestHarness.h>
#include <array>
#include <vector>

TEST_GROUP(recv_wnd) {};

namespace {

struct Fixture
{
    Fixture()
    {
        w.msg = msg.data();
        arq__wnd_init(&w, msg.size(), 32, 64);
        buf.resize(w.msg_len * w.cap);
        w.buf = buf.data();
    }

    arq__wnd_t w;
    std::array< arq__msg_t, 64 > msg;
    std::vector< unsigned char > buf;
    std::vector< unsigned char > seg;
};

TEST(recv_wnd, frame_writes_len_to_msg)
{
    Fixture f;
    f.seg.resize(34);
    arq__recv_wnd_frame(&f.w, 0, 0, 1, f.seg.data(), f.seg.size());
    CHECK_EQUAL(f.seg.size(), f.msg[0].len);
}

TEST(recv_wnd, frame_accumulates_seg_len_to_partially_full_msg)
{
    Fixture f;
    f.seg.resize(29);
    f.msg[0].len = 17;
    arq__recv_wnd_frame(&f.w, 0, 1, 2, f.seg.data(), f.seg.size());
    CHECK_EQUAL(17 + 29, f.msg[0].len);
}

TEST(recv_wnd, frame_indexes_msg_by_seq)
{
    Fixture f;
    f.seg.resize(1);
    arq__recv_wnd_frame(&f.w, 12345, 0, 1, f.seg.data(), f.seg.size());
    CHECK_EQUAL(1, f.msg[12345 % f.w.cap].len);
}

void MockWndSeg(arq__wnd_t *w, int seq, int seg, void const **out_seg, int *out_seg_len)
{
    mock().actualCall("arq__wnd_seg")
          .withParameter("w", w)
          .withParameter("seq", seq)
          .withParameter("seg", seg)
          .withOutputParameter("out_seg", (void *)out_seg)
          .withOutputParameter("out_seg_len", out_seg_len);
}

TEST(recv_wnd, frame_copies_data_into_seat_provided_by_seg_call)
{
    Fixture f;
    int const seq = 123;
    int const seg = 456;
    for (auto i = 0; i < 7; ++i) {
        f.seg.emplace_back((unsigned char)i);
    }
    void *seat = &f.buf[9];
    ARQ_MOCK_HOOK(arq__wnd_seg, MockWndSeg);
    mock().expectOneCall("arq__wnd_seg")
          .withParameter("w", &f.w)
          .withParameter("seq", seq)
          .withParameter("seg", seg)
          .withOutputParameterReturning("out_seg", &seat, sizeof(void *))
          .ignoreOtherParameters();
    arq__recv_wnd_frame(&f.w, seq, seg, 1, f.seg.data(), f.seg.size());
    MEMCMP_EQUAL(f.seg.data(), seat, f.seg.size());
}

}

