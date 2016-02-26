#include "nanoarq_in_test_project.h"
#include <CppUTest/TestHarness.h>

#include <cstring>

TEST_GROUP(poll) {};

namespace
{

TEST(poll, invalid_params)
{
    arq_t arq;
    int send_size;
    arq_event_t event;
    arq_time_t time;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_poll(nullptr, 0, &send_size, &event,  &time));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_poll(&arq,    0, nullptr,    &event,  &time));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_poll(&arq,    0, &send_size, nullptr, &time));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_poll(&arq,    0, &send_size, &event,  nullptr));
}

TEST(poll, nothing_to_send_returns_zero_in_backend_send_size)
{

}

}

