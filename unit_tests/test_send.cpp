#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <array>
#include <cstring>

TEST_GROUP(send) {};

namespace {

TEST(send, invalid_params)
{
    void const *p = nullptr;
    unsigned size;
    arq_t arq;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_send(nullptr, p, 1, &size));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_send(&arq, nullptr, 1, &size));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_send(&arq, p, 1, nullptr));
}

TEST(send, need_poll)
{
    arq_uchar_t buf;
    unsigned size;
    arq_t arq;
    arq.need_poll = ARQ_TRUE;
    CHECK_EQUAL(ARQ_ERR_POLL_REQUIRED, arq_send(&arq, &buf, 1, &size));
}

unsigned MockSendWndSend(arq__send_wnd_t *sw, void const *buf, unsigned len)
{
    return mock().actualCall("arq__send_wnd_send")
                 .withParameter("sw", sw).withParameter("buf", buf).withParameter("len", len)
                 .returnUnsignedIntValue();
}

struct Fixture
{
    Fixture()
    {
        arq.need_poll = ARQ_FALSE;
        ARQ_MOCK_HOOK(arq__send_wnd_send, MockSendWndSend);
    }
    arq_t arq;
    std::array< arq_uchar_t, 16 > buf;
    unsigned sent;
};

TEST(send, calls_wnd_snd)
{
    Fixture f;
    mock().expectOneCall("arq__send_wnd_send")
          .withParameter("sw", &f.arq.send_wnd)
          .withParameter("buf", (void const *)f.buf.data())
          .withParameter("len", f.buf.size());
    arq_send(&f.arq, f.buf.data(), f.buf.size(), &f.sent);
}

TEST(send, bytes_written_is_return_value_from_wnd_send)
{
    Fixture f;
    mock().expectOneCall("arq__send_wnd_send").ignoreOtherParameters().andReturnValue(1234);
    arq_send(&f.arq, f.buf.data(), f.buf.size(), &f.sent);
    CHECK_EQUAL(f.sent, 1234);
}

TEST(send, returns_success)
{
    Fixture f;
    mock().ignoreOtherCalls();
    arq_err_t const e = arq_send(&f.arq, f.buf.data(), f.buf.size(), &f.sent);
    CHECK_EQUAL(ARQ_OK_COMPLETED, e);
}

}


