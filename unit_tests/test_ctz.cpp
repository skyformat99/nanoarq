#include "arq_in_unit_tests.h"
#include <CppUTest/TestHarness.h>
#include <limits>

TEST_GROUP(ctz) {};

namespace
{

TEST(ctz, all_1_bits_returns_0)
{
    CHECK_EQUAL(0, arq__ctz((unsigned)-1));
}

TEST(ctz, all_but_last_1_bit_returns_1)
{
    CHECK_EQUAL(1, arq__ctz((unsigned)-1 & ~1u));
}

TEST(ctz, only_middle_bit_returns_index_of_middle_bit)
{
    unsigned const bit_index = 5u;
    CHECK_EQUAL(bit_index, arq__ctz(1u << bit_index));
}

TEST(ctz, only_highest_bit_set_returns_unsigned_bit_len_minus_one)
{
    unsigned const max = std::numeric_limits< unsigned >::max();
    CHECK_EQUAL((sizeof(unsigned) * 8) - 1u, arq__ctz(max & ~(max >> 1u)));
}

TEST(ctz, test_all_single_bits)
{
    for (auto i = 0u; i < sizeof(unsigned) * 8; ++i) {
        CHECK_EQUAL(i, arq__ctz(1u << i));
    }
}

}

