#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(required_size) {};

namespace {

TEST(required_size, invalid_params)
{
    arq_cfg_t cfg;
    unsigned req;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_required_size(nullptr, &req));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_required_size(&cfg, nullptr));
}

arq_err_t MockCheckCfg(arq_cfg_t const *cfg)
{
    return (arq_err_t)mock().actualCall("arq__check_cfg").withParameter("cfg", cfg).returnIntValue();
}

void MockLinAllocInit(arq__lin_alloc_t *a, void *base, unsigned capacity)
{
    mock().actualCall("arq__lin_alloc_init").withParameter("a", a)
                                            .withParameter("base", base)
                                            .withParameter("capacity", capacity);
}

template< unsigned Size = 0 >
arq_t *MockAlloc(arq_cfg_t const *cfg, arq__lin_alloc_t *la)
{
    if (la) {
        la->size = Size;
    }
    return (arq_t *)mock().actualCall("arq__alloc").withParameter("cfg", cfg)
                                                   .withParameter("la", la)
                                                   .returnPointerValue();
}

struct BaseFixture
{
    BaseFixture()
    {
        ARQ_MOCK_HOOK(arq__check_cfg, MockCheckCfg);
        ARQ_MOCK_HOOK(arq__lin_alloc_init, MockLinAllocInit);
    }
    arq_cfg_t cfg;
    unsigned size;
};

struct Fixture : BaseFixture
{
    Fixture()
    {
        ARQ_MOCK_HOOK(arq__alloc, MockAlloc<0>);
    }
};

TEST(required_size, returns_failure_from_check_cfg_if_check_cfg_fails)
{
    Fixture f;
    mock().expectOneCall("arq__check_cfg").withParameter("cfg", (void const *)&f.cfg).andReturnValue(-15);
    arq_err_t const e = arq_required_size(&f.cfg, &f.size);
    CHECK_EQUAL(-15, (int)e);
}

TEST(required_size, initializes_linear_allocator_with_null_ptr_and_max_capacity)
{
    Fixture f;
    mock().expectOneCall("arq__check_cfg").ignoreOtherParameters().andReturnValue(ARQ_OK_COMPLETED);
    mock().expectOneCall("arq__lin_alloc_init").withParameter("base", (void *)NULL)
                                               .withParameter("capacity", (unsigned)-1)
                                               .ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_required_size(&f.cfg, &f.size);
}

TEST(required_size, calls_arq_alloc_with_config_struct)
{
    Fixture f;
    mock().expectOneCall("arq__check_cfg").ignoreOtherParameters().andReturnValue(ARQ_OK_COMPLETED);
    mock().expectOneCall("arq__alloc").withParameter("cfg", (void const *)&f.cfg)
                                      .ignoreOtherParameters()
                                      .andReturnValue((void *)NULL);
    mock().ignoreOtherCalls();
    arq_required_size(&f.cfg, &f.size);
}

TEST(required_size, writes_result_of_linear_allocator_size_to_size_output_parameter)
{
    BaseFixture f;
    ARQ_MOCK_HOOK(arq__alloc, MockAlloc<1234>);
    mock().expectOneCall("arq__check_cfg").ignoreOtherParameters().andReturnValue(ARQ_OK_COMPLETED);
    mock().ignoreOtherCalls();
    arq_required_size(&f.cfg, &f.size);
    CHECK_EQUAL(1234, f.size);
}

TEST(required_size, returns_success)
{
    Fixture f;
    mock().expectOneCall("arq__check_cfg").ignoreOtherParameters().andReturnValue(ARQ_OK_COMPLETED);
    mock().expectOneCall("arq__lin_alloc_init").ignoreOtherParameters();
    mock().expectOneCall("arq__alloc").ignoreOtherParameters().andReturnValue((void *)NULL);
    arq_err_t const e = arq_required_size(&f.cfg, &f.size);
    CHECK_EQUAL(ARQ_OK_COMPLETED, e);
}

}

