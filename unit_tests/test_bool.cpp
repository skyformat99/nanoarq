#include "arq_in_unit_tests.h"
#include <CppUTest/TestHarness.h>

TEST_GROUP(bool) {};

namespace {

TEST(bool, true_does_not_equal_false)
{
    CHECK(ARQ_TRUE != ARQ_FALSE);
}

TEST(bool, false_is_zero)
{
    CHECK_EQUAL(0, ARQ_FALSE);
}

TEST(bool, true_is_one)
{
    CHECK_EQUAL(1, ARQ_TRUE);
}

TEST(bool, truthful_expression_is_equivalent_to_arq_true)
{
    CHECK_EQUAL(ARQ_TRUE, 1 == 1);
}

TEST(bool, false_expression_is_equivalent_to_arq_false)
{
    CHECK_EQUAL(ARQ_FALSE, 1 == 0);
}

TEST(bool, not_true_equals_false)
{
    CHECK_EQUAL(ARQ_FALSE, !ARQ_TRUE);
}

TEST(bool, not_false_equals_true)
{
    CHECK_EQUAL(ARQ_TRUE, !ARQ_FALSE);
}

TEST(bool, size_is_one_byte)
{
    CHECK_EQUAL(1, sizeof(arq_bool_t));
}

}

