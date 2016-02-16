#include "nanoarq_in_test_project.h"
#include <CppUTest/TestHarness.h>

TEST_GROUP(abs) {};

namespace
{

TEST(abs, x_is_positive)
{
    CHECK_EQUAL(arq__abs(3), 3);
}

TEST(abs, x_is_negative)
{
    CHECK_EQUAL(arq__abs(-3), 3);
}

TEST(abs, x_is_zero)
{
    CHECK_EQUAL(arq__abs(0), 0);
}

}

