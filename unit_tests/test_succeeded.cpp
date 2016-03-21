#include "arq_in_unit_tests.h"
#include <CppUTest/TestHarness.h>

TEST_GROUP(succeeded) {};

namespace {

TEST(succeeded, true_for_ok)
{
    CHECK(ARQ_SUCCEEDED(ARQ_OK_COMPLETED));
}

TEST(succeeded, true_for_enum_greater_than_zero)
{
    CHECK(ARQ_SUCCEEDED(ARQ_OK_POLL_REQUIRED));
}

TEST(succeeded, true_for_numbers_greater_than_or_equal_to_zero)
{
    CHECK(ARQ_SUCCEEDED(0));
    CHECK(ARQ_SUCCEEDED(1));
    CHECK(ARQ_SUCCEEDED(10));
}

TEST(succeeded, fails_for_negative_enum)
{
    CHECK_FALSE(ARQ_SUCCEEDED(ARQ_ERR_INVALID_PARAM));
}

TEST(succeeded, fails_for_negative_numbers)
{
    CHECK_FALSE(ARQ_SUCCEEDED(-1));
    CHECK_FALSE(ARQ_SUCCEEDED(-10));
}

}

