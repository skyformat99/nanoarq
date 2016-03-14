#include "nanoarq_unit_test.h"
#include "nanoarq_hook_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <cstring>

TEST_GROUP(recv) {};

namespace {

TEST(recv, invalid_params)
{
    void *p = nullptr;
    int size;
    arq_t arq;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_recv(nullptr, p, 1, &size));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_recv(&arq, nullptr, 1, &size));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_recv(&arq, p, 1, nullptr));
}

TEST(recv, fails_if_not_connected)
{
    // TODO
}

unsigned MockRecvWndRecv(arq__recv_wnd_t *rw, void *dst, unsigned dst_max)
{
    return mock().actualCall("arq__recv_wnd_recv").withParameter("rw", rw)
                                                  .withParameter("dst", dst)
                                                  .withParameter("dst_max", dst_max)
                                                  .returnUnsignedIntValue();
}

TEST(recv, calls_wnd_recv)
{
    arq_t arq;
    char recv[20];
    ARQ_MOCK_HOOK(arq__recv_wnd_recv, MockRecvWndRecv);
    mock().expectOneCall("arq__recv_wnd_recv").withParameter("rw", &arq.recv_wnd)
                                              .withParameter("dst", (void *)recv)
                                              .withParameter("dst_max", sizeof(recv));
    int recvd;
    arq_recv(&arq, recv, sizeof(recv), &recvd);
}

TEST(recv, wnd_recv_return_copies_into_output_size_param)
{
    arq_t arq;
    char recv[20];
    ARQ_MOCK_HOOK(arq__recv_wnd_recv, MockRecvWndRecv);
    mock().expectOneCall("arq__recv_wnd_recv").ignoreOtherParameters().andReturnValue(4321);
    int recvd;
    arq_recv(&arq, recv, sizeof(recv), &recvd);
    CHECK_EQUAL(4321, recvd);
}

TEST(recv, returns_success)
{
    arq_t arq;
    char recv[20];
    ARQ_MOCK_HOOK(arq__recv_wnd_recv, MockRecvWndRecv);
    mock().expectOneCall("arq__recv_wnd_recv").ignoreOtherParameters();
    int recvd;
    arq_err_t const rv = arq_recv(&arq, recv, sizeof(recv), &recvd);
    CHECK_EQUAL(ARQ_OK_COMPLETED, rv);
}

}

