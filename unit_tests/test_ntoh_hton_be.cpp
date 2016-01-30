#include <CppUTest/TestHarness.h>
#include <cstdint>
#include <cstddef>

TEST_GROUP(ntoh_hton_be) {};

namespace { namespace be
{
#define NANOARQ_UINT16_TYPE uint16_t
#define NANOARQ_UINT32_TYPE uint32_t
#define NANOARQ_UINTPTR_TYPE uintptr_t
#define NANOARQ_NULL_PTR nullptr
#define NANOARQ_LITTLE_ENDIAN_CPU 0
#define NANOARQ_COMPILE_CRC32 0

#undef NANOARQ_COMPILE_AS_CPP
#define NANOARQ_COMPILE_AS_CPP 1

#include "nanoarq.h"
#define NANOARQ_IMPLEMENTATION
#include "nanoarq.h"

TEST(ntoh_hton_be, ntoh16_identity)
{
    CHECK_EQUAL((uint16_t)0x1234u, be::arq__ntoh16(0x1234u));
}

TEST(ntoh_hton_be, hton16_identity)
{
    CHECK_EQUAL((uint16_t)0x1234u, be::arq__hton16(0x1234u));
}

TEST(ntoh_hton_be, ntoh32_identity)
{
    CHECK_EQUAL(0x12345678u, be::arq__ntoh32(0x12345678u));
}

TEST(ntoh_hton_be, hton32_identity)
{
    CHECK_EQUAL(0x12345678u, be::arq__hton32(0x12345678u));
}

}}

