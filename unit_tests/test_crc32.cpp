#include "nanoarq_unit_test.h"
#include <CppUTest/TestHarness.h>
#include <cstring>

TEST_GROUP(crc32) {};

namespace {

// http://www.lammertbies.nl/comm/info/crc-calculation.html

TEST(crc32, hello)
{
    char const *hello = "hello";
    uint32_t const helloCrc32 = arq_crc32(hello, std::strlen(hello));
    CHECK_EQUAL(0x3610A686, helloCrc32);
}

TEST(crc32, world)
{
    char const *world = "world";
    uint32_t const worldCrc32 = arq_crc32(world, std::strlen(world));
    CHECK_EQUAL(0x3A771143, worldCrc32);
}

}

