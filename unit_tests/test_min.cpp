#include "nanoarq_unit_test.h"
#include <CppUTest/TestHarness.h>

TEST_GROUP(min) {};

namespace {

TEST(min, x_less_than_y)
{
    CHECK_EQUAL(1, arq__min(1, 10));
}

TEST(min, x_greater_than_y)
{
    CHECK_EQUAL(1, arq__min(10, 1));
}

TEST(min, x_equal_to_y)
{
    CHECK_EQUAL(1, arq__min(1, 1));
}

}

