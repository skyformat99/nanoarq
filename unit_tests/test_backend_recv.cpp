#include "nanoarq_unit_test.h"
#include <CppUTest/TestHarness.h>

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

}

