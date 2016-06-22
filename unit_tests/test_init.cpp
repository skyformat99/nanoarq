#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <vector>

TEST_GROUP(init) {};

namespace {

TEST(init, invalid_params)
{
    arq_cfg_t cfg;
    arq_uchar_t seat;
    arq_t *arq;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_init(nullptr, &seat, 1, &arq));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_init(&cfg, nullptr, 1, &arq));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_init(&cfg, &seat, 1, nullptr));
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

void MockInit(arq_t *arq)
{
    mock().actualCall("arq__init").withParameter("arq", arq);
}

struct BaseFixture
{
    BaseFixture()
    {
        ARQ_MOCK_HOOK(arq__check_cfg, MockCheckCfg);
        ARQ_MOCK_HOOK(arq__lin_alloc_init, MockLinAllocInit);
        ARQ_MOCK_HOOK(arq__init, MockInit);
        seat.resize(1024 * 1024);
    }
    arq_cfg_t cfg;
    arq_t *arq;
    std::vector< arq_uchar_t > seat;
};

struct Fixture : BaseFixture
{
    Fixture()
    {
        ARQ_MOCK_HOOK(arq__alloc, MockAlloc<0>);
    }
};

TEST(init, returns_failure_from_check_cfg_if_check_cfg_fails)
{
    Fixture f;
    mock().expectOneCall("arq__check_cfg").withParameter("cfg", (void const *)&f.cfg).andReturnValue(-15);
    arq_err_t const e = arq_init(&f.cfg, f.seat.data(), f.seat.size(), &f.arq);
    CHECK_EQUAL(-15, (int)e);
}

TEST(init, initializes_linear_allocator_with_user_seat_and_size)
{
    Fixture f;
    mock().expectOneCall("arq__check_cfg").ignoreOtherParameters().andReturnValue(ARQ_OK_COMPLETED);
    mock().expectOneCall("arq__lin_alloc_init").withParameter("base", f.seat.data())
                                               .withParameter("capacity", f.seat.size())
                                               .ignoreOtherParameters();
    mock().ignoreOtherCalls();
    arq_init(&f.cfg, f.seat.data(), f.seat.size(), &f.arq);
}

TEST(init, calls_arq_alloc_with_config_struct)
{
    Fixture f;
    mock().expectOneCall("arq__check_cfg").ignoreOtherParameters().andReturnValue(ARQ_OK_COMPLETED);
    mock().expectOneCall("arq__alloc").withParameter("cfg", (void const *)&f.cfg)
                                      .ignoreOtherParameters()
                                      .andReturnValue((void *)f.seat.data());
    mock().ignoreOtherCalls();
    arq_init(&f.cfg, f.seat.data(), f.seat.size(), &f.arq);
}

TEST(init, returns_error_if_alloc_returns_null)
{
    Fixture f;
    mock().expectOneCall("arq__check_cfg").ignoreOtherParameters().andReturnValue(ARQ_OK_COMPLETED);
    mock().expectOneCall("arq__alloc").withParameter("cfg", (void const *)&f.cfg)
                                      .ignoreOtherParameters()
                                      .andReturnValue((void *)NULL);
    mock().ignoreOtherCalls();
    arq_err_t const e = arq_init(&f.cfg, f.seat.data(), f.seat.size(), &f.arq);
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, e);
}

TEST(init, calls_arq_private_init_if_alloc_succeeds)
{
    Fixture f;
    mock().expectOneCall("arq__check_cfg").ignoreOtherParameters().andReturnValue(ARQ_OK_COMPLETED);
    mock().expectOneCall("arq__alloc").ignoreOtherParameters().andReturnValue(f.seat.data());
    mock().expectOneCall("arq__init").withParameter("arq", f.seat.data());
    mock().ignoreOtherCalls();
    arq_init(&f.cfg, f.seat.data(), f.seat.size(), &f.arq);
}

TEST(init, returns_success)
{
    arq_cfg_t cfg;
    cfg.send_window_size_in_messages = 1;
    cfg.recv_window_size_in_messages = 1;
    cfg.segment_length_in_bytes = 64;
    cfg.message_length_in_segments = 1;
    cfg.connection_rst_period = 100;
    cfg.connection_rst_attempts = 3;
    std::vector< arq_uchar_t > seat(1024 * 1024);
    arq_t *arq;
    arq_err_t const e = arq_init(&cfg, seat.data(), seat.size(), &arq);
    CHECK_EQUAL(ARQ_OK_COMPLETED, e);
}

}

