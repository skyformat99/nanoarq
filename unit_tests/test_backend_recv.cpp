#include "nanoarq_unit_test.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(backend_recv) {};

namespace {

TEST(backend_recv, invalid_params)
{
    arq_t arq;
    void *recv = nullptr;
    int recv_size = 0, bytes_recvd;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_recv_fill(nullptr, recv,    recv_size, &bytes_recvd));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_recv_fill(&arq,    nullptr, recv_size, &bytes_recvd));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_recv_fill(&arq,    recv,    recv_size, nullptr));
}

unsigned MockRecvFrameFill(arq__recv_frame_t *f, void const *src, unsigned len)
{
    return mock().actualCall("arq__recv_frame_fill").withParameter("f", f)
                                                    .withParameter("src", src)
                                                    .withParameter("len", len)
                                                    .returnUnsignedIntValue();
}

struct Fixture
{
    Fixture()
    {
        ARQ_MOCK_HOOK(arq__recv_frame_fill, MockRecvFrameFill);
    }
    arq_t arq;
    char recv[29];
    int recvd;
};

TEST(backend_recv, forwards_to_arq_recv_frame_fill)
{
    Fixture f;
    mock().expectOneCall("arq__recv_frame_fill").withParameter("f", &f.arq.recv_frame)
                                                .withParameter("src", (void const *)f.recv)
                                                .withParameter("len", sizeof(f.recv));
    arq_backend_recv_fill(&f.arq, f.recv, sizeof(f.recv), &f.recvd);
}

TEST(backend_recv, return_value_from_arq_recv_frame_fill_written_to_output_len_param)
{
    Fixture f;
    mock().expectOneCall("arq__recv_frame_fill").ignoreOtherParameters().andReturnValue(123);
    arq_backend_recv_fill(&f.arq, f.recv, sizeof(f.recv), &f.recvd);
    CHECK_EQUAL(123, f.recvd);
}

TEST(backend_recv, returns_ok_success_if_frame_state_is_accumulating)
{
    Fixture f;
    mock().ignoreOtherCalls();
    f.arq.recv_frame.state = ARQ__RECV_FRAME_STATE_ACCUMULATING;
    arq_err_t const rv = arq_backend_recv_fill(&f.arq, f.recv, sizeof(f.recv), &f.recvd);
    CHECK_EQUAL(ARQ_OK_COMPLETED, rv);
}

TEST(backend_recv, returns_ok_poll_if_frame_state_is_complete)
{
    Fixture f;
    mock().ignoreOtherCalls();
    f.arq.recv_frame.state = ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT;
    arq_err_t const rv = arq_backend_recv_fill(&f.arq, f.recv, sizeof(f.recv), &f.recvd);
    CHECK_EQUAL(ARQ_OK_POLL_REQUIRED, rv);
}

}

