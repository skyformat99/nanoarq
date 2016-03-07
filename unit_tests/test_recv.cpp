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

}

