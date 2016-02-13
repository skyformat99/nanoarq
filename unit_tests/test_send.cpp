#include "nanoarq_in_test_project.h"
#include <CppUTest/TestHarness.h>

#include <cstring>

TEST_GROUP(send) {};

namespace
{

TEST(send, invalid_params)
{
    void const *p = nullptr;
    int size;
    arq_t arq;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_send(nullptr, p, 1, &size));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_send(&arq, nullptr, 1, &size));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_send(&arq, p, 1, nullptr));
}


}


