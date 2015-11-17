#include "nanoarq.h"
#include "CppUTest/TestHarness.h"

TEST_GROUP(nanoarq) {};

namespace
{

TEST(nanoarq, required_size_returns_zero)
{
    int required_size = 1;
    CHECK_EQUAL(ARQ_SUCCESS, arq_required_size(&required_size));
    CHECK_EQUAL(0, required_size);
}

}

