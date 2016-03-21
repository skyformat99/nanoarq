#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <cstring>

TEST_GROUP(send) {};

namespace {

TEST(send, invalid_params)
{
    void const *p = nullptr;
    int size;
    arq_t arq;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_send(nullptr, p, 1, &size));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_send(&arq, nullptr, 1, &size));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_send(&arq, p, 1, nullptr));
}

TEST(send, fails_if_not_connected)
{
    // TODO
}

int MockSendWndSend(arq__send_wnd_t *sw, void const *buf, int len)
{
    return mock().actualCall("arq__send_wnd_send")
                 .withParameter("sw", sw).withParameter("buf", buf).withParameter("len", len)
                 .returnIntValue();
}

TEST(send, calls_wnd_snd)
{
    arq_t arq;
    char buf[16];
    ARQ_MOCK_HOOK(arq__send_wnd_send, MockSendWndSend);
    mock().expectOneCall("arq__send_wnd_send")
          .withParameter("sw", &arq.send_wnd)
          .withParameter("buf", (void const *)buf)
          .withParameter("len", (int)sizeof(buf));
    int sent;
    arq_send(&arq, buf, sizeof(buf), &sent);
}

TEST(send, bytes_written_is_return_value_from_wnd_send)
{
    arq_t arq;
    char buf[16];
    ARQ_MOCK_HOOK(arq__send_wnd_send, MockSendWndSend);
    mock().expectOneCall("arq__send_wnd_send").ignoreOtherParameters().andReturnValue(1234);
    int sent;
    arq_send(&arq, buf, sizeof(buf), &sent);
    CHECK_EQUAL(sent, 1234);
}

TEST(send, returns_success)
{
    arq_t arq;
    char buf[16];
    ARQ_MOCK_HOOK(arq__send_wnd_send, MockSendWndSend);
    mock().ignoreOtherCalls();
    int sent;
    arq_err_t const e = arq_send(&arq, buf, sizeof(buf), &sent);
    CHECK_EQUAL(ARQ_OK_COMPLETED, e);
}

}


