#include "nanoarq_in_test_project.h"
#include "CppUTest/TestHarness.h"

TEST_GROUP(cobs) {};

namespace
{

TEST(cobs, encode_one_nonzero_byte)
{
    unsigned char buf[] = { 255, 3, 255 };
    arq__cobs_encode(buf, sizeof(buf));
    unsigned char const encoded[] = { 2, 3, 0 };
    MEMCMP_EQUAL(encoded, buf, sizeof(buf));
}

TEST(cobs, encode_one_zero_byte)
{
    unsigned char buf[] = { 255, 0, 255 };
    arq__cobs_encode(buf, sizeof(buf));
    unsigned char const encoded[] = { 1, 1, 0 };
    MEMCMP_EQUAL(encoded, buf, sizeof(buf));
}

TEST(cobs, encode_two_nonzero_bytes)
{
    unsigned char buf[] = { 255, 3, 15, 255 };
    arq__cobs_encode(buf, sizeof(buf));
    unsigned char const encoded[] = { 3, 3, 15, 0 };
    MEMCMP_EQUAL(encoded, buf, sizeof(buf));
}

TEST(cobs, encode_two_zero_bytes)
{
    unsigned char buf[] = { 255, 0, 0, 255 };
    arq__cobs_encode(buf, sizeof(buf));
    unsigned char const encoded[] = { 1, 1, 1, 0 };
    MEMCMP_EQUAL(encoded, buf, sizeof(buf));
}

TEST(cobs, encode_every_other_byte_zero)
{
    unsigned char buf[] = { 255, 0, 10, 0, 10, 0, 10, 255 };
    arq__cobs_encode(buf, sizeof(buf));
    unsigned char const encoded[] = { 1, 2, 10, 2, 10, 2, 10, 0 };
    MEMCMP_EQUAL(encoded, buf, sizeof(buf));
}

TEST(cobs, encode_run_of_nonzero)
{
    unsigned char buf[] = { 255, 10, 0, 10, 10, 10, 255 };
    arq__cobs_encode(buf, sizeof(buf));
    unsigned char const encoded[] = { 2, 10, 4, 10, 10, 10, 0 };
    MEMCMP_EQUAL(encoded, buf, sizeof(buf));
}

TEST(cobs, encode_run_of_zeroes)
{
    unsigned char buf[] = { 255, 10, 0, 0, 0, 0, 255 };
    arq__cobs_encode(buf, sizeof(buf));
    unsigned char const encoded[] = { 2, 10, 1, 1, 1, 1, 0 };
    MEMCMP_EQUAL(encoded, buf, sizeof(buf));
}

TEST(cobs, decode_one_zero_byte)
{
    unsigned char buf[] = { 1, 1, 0 };
    arq__cobs_decode(buf, sizeof(buf));
    unsigned char const decoded[] = { 0, 0, 0 };
    MEMCMP_EQUAL(decoded, buf, sizeof(buf));
}

TEST(cobs, decode_one_nonzero_byte)
{
    unsigned char buf[] = { 2, 123, 0 };
    arq__cobs_decode(buf, sizeof(buf));
    unsigned char const decoded[] = { 0, 123, 0 };
    MEMCMP_EQUAL(decoded, buf, sizeof(buf));
}

TEST(cobs, decode_two_zero_bytes)
{
    unsigned char buf[] = { 1, 1, 1, 0 };
    arq__cobs_decode(buf, sizeof(buf));
    unsigned char const decoded[] = { 0, 0, 0, 0 };
    MEMCMP_EQUAL(decoded, buf, sizeof(buf));
}

TEST(cobs, decode_two_nonzero_bytes)
{
    unsigned char buf[] = { 3, 123, 77, 0 };
    arq__cobs_decode(buf, sizeof(buf));
    unsigned char const decoded[] = { 0, 123, 77, 0 };
    MEMCMP_EQUAL(decoded, buf, sizeof(buf));
}

TEST(cobs, decode_every_other_byte_zero)
{
    unsigned char buf[] = { 1, 2, 10, 2, 10, 2, 10, 0 };
    arq__cobs_decode(buf, sizeof(buf));
    unsigned char const decoded[] = { 0, 0, 10, 0, 10, 0, 10, 0 };
    MEMCMP_EQUAL(decoded, buf, sizeof(buf));
}

TEST(cobs, decode_run_of_nonzero)
{
    unsigned char buf[] = { 2, 10, 4, 10, 10, 10, 0 };
    arq__cobs_decode(buf, sizeof(buf));
    unsigned char const decoded[] = { 0, 10, 0, 10, 10, 10, 0 };
    MEMCMP_EQUAL(decoded, buf, sizeof(buf));
}

TEST(cobs, decode_run_of_zeroes)
{
    unsigned char buf[] = { 2, 10, 1, 1, 1, 1, 0 };
    arq__cobs_decode(buf, sizeof(buf));
    unsigned char const decoded[] = { 0, 10, 0, 0, 0, 0, 0 };
    MEMCMP_EQUAL(decoded, buf, sizeof(buf));
}

}

