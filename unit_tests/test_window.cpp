#include "nanoarq_unit_test.h"
#include "nanoarq_hook_plugin.h"
#include <CppUTestExt/MockSupport.h>
#include <CppUTest/TestHarness.h>
#include <cstring>
#include <vector>
#include <array>

TEST_GROUP(wnd) {};

namespace {

struct UninitializedWindowFixture
{
    UninitializedWindowFixture()
    {
        w.buf = nullptr;
        w.msg = msg.data();
    }
    arq__wnd_t w;
    std::array< arq__msg_t, 64 > msg;
};

TEST(wnd, init_writes_params_to_struct)
{
    UninitializedWindowFixture f;
    arq__wnd_init(&f.w, f.msg.size(), 16, 8);
    CHECK_EQUAL(f.msg.size(), f.w.cap);
    CHECK_EQUAL(16, f.w.msg_len);
    CHECK_EQUAL(8, f.w.seg_len);
}

TEST(wnd, init_full_ack_vec_is_one_if_seg_len_equals_msg_len)
{
    UninitializedWindowFixture f;
    arq__wnd_init(&f.w, f.msg.size(), 16, 16);
    CHECK_EQUAL(0b1, f.w.full_ack_vec);
}

TEST(wnd, init_calculates_full_ack_vector_from_msg_len_and_seg_len)
{
    UninitializedWindowFixture f;
    arq__wnd_init(&f.w, f.msg.size(), 32, 8);
    CHECK_EQUAL(0b1111, f.w.full_ack_vec);
}

void MockWndRst(arq__wnd_t *w)
{
    mock().actualCall("arq__wnd_rst").withParameter("w", w);
}

TEST(wnd, init_calls_rst)
{
    UninitializedWindowFixture f;
    ARQ_MOCK_HOOK(arq__wnd_rst, MockWndRst);
    mock().expectOneCall("arq__wnd_rst").withParameter("w", &f.w);
    arq__wnd_init(&f.w, f.msg.size(), 16, 8);
}

struct Fixture : UninitializedWindowFixture
{
    Fixture()
    {
        buf.resize(w.msg_len * w.cap);
        w.buf = buf.data();
        arq__wnd_init(&w, msg.size(), 128, 16);
    }

    std::vector< unsigned char > buf;
    std::vector< unsigned char > snd;
    int n = 0;
    void const *p = nullptr;
};

TEST(wnd, seg_with_base_idx_zero_returns_buf)
{
    Fixture f;
    arq__wnd_seg(&f.w, 0, 0, &f.p, &f.n);
    CHECK_EQUAL((void const *)f.buf.data(), f.p);
}

TEST(wnd, seg_message_less_than_one_segment_returns_message_size)
{
    Fixture f;
    f.w.msg_len = f.w.msg[0].len = 13;
    arq__wnd_seg(&f.w, 0, 0, &f.p, &f.n);
    CHECK_EQUAL(f.w.msg[0].len, f.n);
}

TEST(wnd, seg_message_greater_than_one_segment_returns_seg_len_for_non_final_segment_index)
{
    Fixture f;
    f.w.msg[0].len = f.w.msg_len;
    arq__wnd_seg(&f.w, 0, 0, &f.p, &f.n);
    CHECK_EQUAL(f.w.seg_len, f.n);
}

TEST(wnd, seg_points_at_segment_len_times_segment_index)
{
    Fixture f;
    f.w.msg[0].len = f.w.msg_len;
    arq__wnd_seg(&f.w, 0, 3, &f.p, &f.n);
    CHECK_EQUAL((void const *)&f.buf[3 * f.w.seg_len], f.p);
}

TEST(wnd, seg_returns_msg_len_remainder_for_final_segment)
{
    Fixture f;
    f.w.msg[0].len = f.w.msg_len - (f.w.seg_len / 2);
    arq__wnd_seg(&f.w, 0, (f.w.msg_len / f.w.seg_len) - 1, &f.p, &f.n);
    CHECK_EQUAL(f.w.seg_len / 2, f.n);
}

TEST(wnd, seg_size_looks_up_message_by_message_index)
{
    Fixture f;
    f.w.size = 2;
    f.w.msg[1].len = 5;
    arq__wnd_seg(&f.w, 1, 0, &f.p, &f.n);
    CHECK_EQUAL(5, f.n);
}

TEST(wnd, seg_buf_looks_up_message_by_message_index)
{
    Fixture f;
    f.w.size = 2;
    f.w.msg[1].len = 5;
    arq__wnd_seg(&f.w, 1, 0, &f.p, &f.n);
    CHECK_EQUAL((void const *)&f.buf[f.w.msg_len], f.p);
}

TEST(wnd, seg_buf_pointer_is_message_offset_plus_segment_offset)
{
    Fixture f;
    f.w.size = 3;
    f.w.msg[2].len = f.w.seg_len + 1;
    arq__wnd_seg(&f.w, 2, 1, &f.p, &f.n);
    CHECK_EQUAL((void const *)&f.buf[(f.w.msg_len * 2) + f.w.seg_len], f.p);
}

}

