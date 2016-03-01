#include "nanoarq_unit_test.h"
#include <CppUTest/TestHarness.h>

#include <cstring>

TEST_GROUP(cobs) {};

namespace {

TEST(cobs, encode_one_nonzero_byte)
{
    unsigned char buf[] = { 255, 3, 255 };
    arq__cobs_encode(buf, sizeof(buf));
    unsigned char const encoded[] = { 2, 3, 0 };
    MEMCMP_EQUAL(buf, encoded, sizeof(buf));
}

TEST(cobs, encode_one_zero_byte)
{
    unsigned char buf[] = { 255, 0, 255 };
    arq__cobs_encode(buf, sizeof(buf));
    unsigned char const encoded[] = { 1, 1, 0 };
    MEMCMP_EQUAL(buf, encoded, sizeof(buf));
}

TEST(cobs, encode_two_nonzero_bytes)
{
    unsigned char buf[] = { 255, 3, 15, 255 };
    arq__cobs_encode(buf, sizeof(buf));
    unsigned char const encoded[] = { 3, 3, 15, 0 };
    MEMCMP_EQUAL(buf, encoded, sizeof(buf));
}

TEST(cobs, encode_two_zero_bytes)
{
    unsigned char buf[] = { 255, 0, 0, 255 };
    arq__cobs_encode(buf, sizeof(buf));
    unsigned char const encoded[] = { 1, 1, 1, 0 };
    MEMCMP_EQUAL(buf, encoded, sizeof(buf));
}

TEST(cobs, encode_every_other_byte_zero)
{
    unsigned char buf[] = { 255, 0, 10, 0, 10, 0, 10, 255 };
    arq__cobs_encode(buf, sizeof(buf));
    unsigned char const encoded[] = { 1, 2, 10, 2, 10, 2, 10, 0 };
    MEMCMP_EQUAL(buf, encoded, sizeof(buf));
}

TEST(cobs, encode_run_of_nonzero)
{
    unsigned char buf[] = { 255, 10, 0, 10, 10, 10, 255 };
    arq__cobs_encode(buf, sizeof(buf));
    unsigned char const encoded[] = { 2, 10, 4, 10, 10, 10, 0 };
    MEMCMP_EQUAL(buf, encoded, sizeof(buf));
}

TEST(cobs, encode_run_of_zeroes)
{
    unsigned char buf[] = { 255, 10, 0, 0, 0, 0, 255 };
    arq__cobs_encode(buf, sizeof(buf));
    unsigned char const encoded[] = { 2, 10, 1, 1, 1, 1, 0 };
    MEMCMP_EQUAL(buf, encoded, sizeof(buf));
}

TEST(cobs, encode_maximum_payload_size_no_zeroes)
{
    unsigned char buf[256];
    std::memset(buf, 0x55, sizeof(buf));
    arq__cobs_encode(buf, sizeof(buf));
    unsigned char encoded[256];
    std::memset(encoded, 0x55, sizeof(encoded));
    encoded[0] = 0xFF;
    encoded[255] = 0;
    MEMCMP_EQUAL(buf, encoded, sizeof(buf));
}

TEST(cobs, encode_maximum_payload_size_all_zeroes)
{
    unsigned char buf[256] = { 0 };
    arq__cobs_encode(buf, sizeof(buf));
    unsigned char encoded[256];
    std::memset(encoded, 0x01, sizeof(encoded));
    encoded[255] = 0;
    MEMCMP_EQUAL(buf, encoded, sizeof(buf));
}

TEST(cobs, encode_maximum_payload_size_every_other_byte_zero)
{
    unsigned char buf[256];
    for (int i = 0; i < 256; i += 2) {
        buf[i] = 0;
        buf[i + 1] = i + 1;
    }
    buf[0] = 0xFF;
    arq__cobs_encode(buf, sizeof(buf));
    unsigned char encoded[256];
    for (int i = 0; i < 256; i += 2) {
        encoded[i] = 2;
        encoded[i + 1] = i + 1;
    }
    encoded[254] = 1;
    encoded[255] = 0;
    MEMCMP_EQUAL(buf, encoded, sizeof(buf));
}

TEST(cobs, decode_one_zero_byte)
{
    unsigned char buf[] = { 1, 1, 0 };
    arq__cobs_decode(buf, sizeof(buf));
    unsigned char const decoded[] = { 0, 0, 0 };
    MEMCMP_EQUAL(buf, decoded, sizeof(buf));
}

TEST(cobs, decode_one_nonzero_byte)
{
    unsigned char buf[] = { 2, 123, 0 };
    arq__cobs_decode(buf, sizeof(buf));
    unsigned char const decoded[] = { 0, 123, 0 };
    MEMCMP_EQUAL(buf, decoded, sizeof(buf));
}

TEST(cobs, decode_two_zero_bytes)
{
    unsigned char buf[] = { 1, 1, 1, 0 };
    arq__cobs_decode(buf, sizeof(buf));
    unsigned char const decoded[] = { 0, 0, 0, 0 };
    MEMCMP_EQUAL(buf, decoded, sizeof(buf));
}

TEST(cobs, decode_two_nonzero_bytes)
{
    unsigned char buf[] = { 3, 123, 77, 0 };
    arq__cobs_decode(buf, sizeof(buf));
    unsigned char const decoded[] = { 0, 123, 77, 0 };
    MEMCMP_EQUAL(buf, decoded, sizeof(buf));
}

TEST(cobs, decode_every_other_byte_zero)
{
    unsigned char buf[] = { 1, 2, 10, 2, 10, 2, 10, 0 };
    arq__cobs_decode(buf, sizeof(buf));
    unsigned char const decoded[] = { 0, 0, 10, 0, 10, 0, 10, 0 };
    MEMCMP_EQUAL(buf, decoded, sizeof(buf));
}

TEST(cobs, decode_run_of_nonzero)
{
    unsigned char buf[] = { 2, 10, 4, 10, 10, 10, 0 };
    arq__cobs_decode(buf, sizeof(buf));
    unsigned char const decoded[] = { 0, 10, 0, 10, 10, 10, 0 };
    MEMCMP_EQUAL(buf, decoded, sizeof(buf));
}

TEST(cobs, decode_run_of_zeroes)
{
    unsigned char buf[] = { 2, 10, 1, 1, 1, 1, 0 };
    arq__cobs_decode(buf, sizeof(buf));
    unsigned char const decoded[] = { 0, 10, 0, 0, 0, 0, 0 };
    MEMCMP_EQUAL(buf, decoded, sizeof(buf));
}

TEST(cobs, decode_maximum_payload_size_no_zeroes)
{
    unsigned char buf[256];
    std::memset(buf, 0x55, sizeof(buf));
    buf[0] = 0xFF;
    buf[255] = 0;
    arq__cobs_decode(buf, sizeof(buf));
    unsigned char decoded[256];
    std::memset(decoded, 0x55, sizeof(decoded));
    decoded[0] = 0;
    decoded[255] = 0;
    MEMCMP_EQUAL(buf, decoded, sizeof(buf));
}

TEST(cobs, decode_maximum_payload_size_all_zeroes)
{
    unsigned char buf[256];
    std::memset(buf, 0x01, sizeof(buf));
    buf[255] = 0;
    arq__cobs_decode(buf, sizeof(buf));
    unsigned char decoded[256] = { 0 };
    MEMCMP_EQUAL(buf, decoded, sizeof(buf));
}

TEST(cobs, decode_maximum_payload_size_every_other_byte_zero)
{
    unsigned char buf[256];
    for (int i = 0; i < 256; i += 2) {
        buf[i] = 2;
        buf[i + 1] = i + 1;
    }
    buf[254] = 1;
    arq__cobs_decode(buf, sizeof(buf));
    unsigned char decoded[256];
    for (int i = 0; i < 256; i += 2) {
        decoded[i] = 0;
        decoded[i + 1] = i + 1;
    }
    MEMCMP_EQUAL(buf, decoded, sizeof(buf));
}

}

