#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(rst) {};

namespace {

void MockSendWndPtrRst(arq__send_wnd_ptr_t *p)
{
    mock().actualCall("arq__send_wnd_ptr_rst").withParameter("p", p);
}

void MockSendWndRst(arq__send_wnd_t *sw)
{
    mock().actualCall("arq__send_wnd_rst").withParameter("sw", sw);
}

void MockRecvWndRst(arq__recv_wnd_t *rw)
{
    mock().actualCall("arq__recv_wnd_rst").withParameter("rw", rw);
}

void MockSendFrameRst(arq__send_frame_t *f)
{
    mock().actualCall("arq__send_frame_rst").withParameter("f", f);
}

void MockRecvFrameRst(arq__recv_frame_t *f)
{
    mock().actualCall("arq__recv_frame_rst").withParameter("f", f);
}

struct Fixture
{
    Fixture()
    {
        ARQ_MOCK_HOOK(arq__send_wnd_ptr_rst, MockSendWndPtrRst);
        ARQ_MOCK_HOOK(arq__recv_wnd_rst, MockRecvWndRst);
        ARQ_MOCK_HOOK(arq__send_wnd_rst, MockSendWndRst);
        ARQ_MOCK_HOOK(arq__send_frame_rst, MockSendFrameRst);
        ARQ_MOCK_HOOK(arq__recv_frame_rst, MockRecvFrameRst);
    }
    arq_t arq;
};

TEST(rst, sets_need_poll_to_false)
{
    Fixture f;
    f.arq.need_poll = ARQ_TRUE;
    mock().ignoreOtherCalls();
    arq__rst(&f.arq);
    CHECK_EQUAL(ARQ_FALSE, f.arq.need_poll);
}

TEST(rst, resets_send_wnd_ptr)
{
    Fixture f;
    mock().expectOneCall("arq__send_wnd_ptr_rst").withParameter("p", &f.arq.send_wnd_ptr);
    mock().ignoreOtherCalls();
    arq__rst(&f.arq);
}

TEST(rst, resets_send_window)
{
    Fixture f;
    mock().expectOneCall("arq__send_wnd_rst").withParameter("sw", &f.arq.send_wnd);
    mock().ignoreOtherCalls();
    arq__rst(&f.arq);
}

TEST(rst, resets_recv_window)
{
    Fixture f;
    mock().expectOneCall("arq__recv_wnd_rst").withParameter("rw", &f.arq.recv_wnd);
    mock().ignoreOtherCalls();
    arq__rst(&f.arq);
}

TEST(rst, resets_send_frame)
{
    Fixture f;
    mock().expectOneCall("arq__send_frame_rst").withParameter("f", &f.arq.send_frame);
    mock().ignoreOtherCalls();
    arq__rst(&f.arq);
}

TEST(rst, resets_recv_frame)
{
    Fixture f;
    mock().expectOneCall("arq__recv_frame_rst").withParameter("f", &f.arq.recv_frame);
    mock().ignoreOtherCalls();
    arq__rst(&f.arq);
}

TEST(rst, resets_connection_state_to_closed)
{
    Fixture f;
    f.arq.conn.state = ARQ_CONN_STATE_ESTABLISHED;
    mock().ignoreOtherCalls();
    arq__rst(&f.arq);
    CHECK_EQUAL(ARQ_CONN_STATE_CLOSED, f.arq.conn.state);
}

}

