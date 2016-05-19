#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(reset) {};

namespace {

TEST(reset, invalid_params)
{
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_reset(nullptr));
}

struct Fixture
{
    static void MockArqRst(arq_t *a)
    {
        mock().actualCall("arq__rst").withParameter("arq", a);
    }

    Fixture()
    {
        ARQ_MOCK_HOOK(arq__rst, MockArqRst);
    }
};

TEST(reset, passes_arq_to_rst)
{
    Fixture f;
    arq_t a;
    mock().expectOneCall("arq__rst").withParameter("arq", &a);
    arq_reset(&a);
}

TEST(reset, returns_success)
{
    Fixture f;
    arq_t a;
    mock().ignoreOtherCalls();
    arq_err_t const e = arq_reset(&a);
    CHECK_EQUAL(ARQ_OK_COMPLETED, e);
}

}

