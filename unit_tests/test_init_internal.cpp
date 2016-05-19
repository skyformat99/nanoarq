#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(init_intl) {};

namespace {

void MockWndInit(arq__wnd_t *w, unsigned wnd_cap, unsigned msg_len, unsigned seg_len)
{
    mock().actualCall("arq__wnd_init").withParameter("w", w)
                                      .withParameter("wnd_cap", wnd_cap)
                                      .withParameter("msg_len", msg_len)
                                      .withParameter("seg_len", seg_len);
}

void MockSendFrameInit(arq__send_frame_t *f, unsigned cap)
{
    mock().actualCall("arq__send_frame_init").withParameter("f", f).withParameter("cap", cap);
}

void MockRecvFrameInit(arq__recv_frame_t *f, unsigned cap)
{
    mock().actualCall("arq__recv_frame_init").withParameter("f", f).withParameter("cap", cap);
}

struct Fixture
{
    Fixture()
    {
        ARQ_MOCK_HOOK(arq__wnd_init, MockWndInit);
        ARQ_MOCK_HOOK(arq__send_frame_init, MockSendFrameInit);
        ARQ_MOCK_HOOK(arq__recv_frame_init, MockRecvFrameInit);
        arq.cfg.message_length_in_segments = 16;
        arq.cfg.segment_length_in_bytes = 64;
        arq.cfg.send_window_size_in_messages = 32;
        arq.cfg.recv_window_size_in_messages = 19;
    }
    arq_t arq;
};

TEST(init_intl, initializes_send_window)
{
    Fixture f;
    mock().expectOneCall("arq__wnd_init").withParameter("w", &f.arq.send_wnd)
                                         .withParameter("wnd_cap", f.arq.cfg.send_window_size_in_messages)
                                         .withParameter("msg_len", f.arq.cfg.message_length_in_segments *
                                                                   f.arq.cfg.segment_length_in_bytes)
                                         .withParameter("seg_len", f.arq.cfg.segment_length_in_bytes);
    mock().expectOneCall("arq__wnd_init").ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq__init(&f.arq);
}

TEST(init_intl, initializes_recv_window)
{
    Fixture f;
    mock().expectOneCall("arq__wnd_init").ignoreOtherParameters();
    mock().expectOneCall("arq__wnd_init").withParameter("w", &f.arq.recv_wnd)
                                         .withParameter("wnd_cap", f.arq.cfg.recv_window_size_in_messages)
                                         .withParameter("msg_len", f.arq.cfg.message_length_in_segments *
                                                                   f.arq.cfg.segment_length_in_bytes)
                                         .withParameter("seg_len", f.arq.cfg.segment_length_in_bytes);
    mock().ignoreOtherCalls();
    arq__init(&f.arq);
}

TEST(init_intl, initializes_send_frame)
{
    Fixture f;
    unsigned const frame_len = arq__frame_len(f.arq.cfg.segment_length_in_bytes);
    mock().expectOneCall("arq__send_frame_init").withParameter("f", &f.arq.send_frame)
                                                .withParameter("cap", frame_len);
    mock().ignoreOtherCalls();
    arq__init(&f.arq);
}

TEST(init_intl, initializes_recv_frame)
{
    Fixture f;
    unsigned const frame_len = arq__frame_len(f.arq.cfg.segment_length_in_bytes);
    mock().expectOneCall("arq__recv_frame_init").withParameter("f", &f.arq.recv_frame)
                                                .withParameter("cap", frame_len);
    mock().ignoreOtherCalls();
    arq__init(&f.arq);
}

}

