#include "nanoarq_in_test_project.h"
#include <CppUTest/TestHarness.h>

#include <cstring>

TEST_GROUP(backend_send) {};

namespace
{

TEST(backend_send, send_ptr_get_invalid_params)
{
    arq_t arq;
    void *send;
    int size;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_send_ptr_get(nullptr, &send, &size));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_send_ptr_get(&arq, nullptr, &size));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_send_ptr_get(&arq, &send, nullptr));
}

TEST(backend_send, send_ptr_release_invalid_params)
{
    arq_t arq;
    void *send;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_send_ptr_release(nullptr, &send));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_send_ptr_release(&arq, nullptr));
}

}

