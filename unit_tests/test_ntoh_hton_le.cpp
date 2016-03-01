#include "nanoarq_unit_test.h"
#include <CppUTest/TestHarness.h>

TEST_GROUP(ntoh_hton_le) {};

namespace {

TEST(ntoh_hton_le, ntoh16_swapped)
{
    CHECK_EQUAL((uint16_t)0x3412u, ::arq__ntoh16(0x1234u));
}

TEST(ntoh_hton_le, hton16_swapped)
{
    CHECK_EQUAL((uint16_t)0x3412u, ::arq__hton16(0x1234u));
}

TEST(ntoh_hton_le, ntoh32_swapped)
{
    CHECK_EQUAL(0x78563412u, ::arq__ntoh32(0x12345678u));
}

TEST(ntoh_hton_le, hton32_swapped)
{
    CHECK_EQUAL(0x78563412u, ::arq__hton32(0x12345678u));
}

}

