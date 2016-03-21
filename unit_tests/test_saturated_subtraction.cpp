#include "arq_in_unit_tests.h"
#include <CppUTest/TestHarness.h>

TEST_GROUP(sub_sat) {};

namespace {

TEST(sub_sat, x_greater_than_y_yields_natural_difference)
{
    CHECK_EQUAL(6, arq__sub_sat(10, 4));
}

TEST(sub_sat, x_equals_y_yields_zero)
{
    CHECK_EQUAL(0, arq__sub_sat(10, 10));
}

TEST(sub_sat, x_less_than_y_saturates)
{
    CHECK_EQUAL(0, arq__sub_sat(1, 100000));
}

TEST(sub_sat, x_and_y_are_zero_yields_zero)
{
    CHECK_EQUAL(0, arq__sub_sat(0, 0));
}

TEST(sub_sat, x_is_zero_yields_zero)
{
    CHECK_EQUAL(0, arq__sub_sat(0, 12345));
}

TEST(sub_sat, y_is_zero_yields_x)
{
    CHECK_EQUAL(54321, arq__sub_sat(54321, 0));
}

}

