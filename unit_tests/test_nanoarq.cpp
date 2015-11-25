#include "nanoarq.h"
#include "CppUTest/TestHarness.h"

TEST_GROUP(nanoarq) {};

namespace
{

TEST(nanoarq, required_size_returns_zero)
{
    int required_size = 1;
    arq_cfg_t cfg;
    CHECK_EQUAL(ARQ_OK_COMPLETED, arq_required_size(&cfg, &required_size));
    CHECK_EQUAL(0, required_size);
}

}

