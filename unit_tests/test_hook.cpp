#include "nanoarq_in_test_project.h"
#include "PltHookPlugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(hook) {};

namespace
{

TEST(hook, can_hook_nanoarq_function)
{
    struct Local
    {
        static bool& Called() { static bool called = false; return called; }
        static arq_err_t FakeArqConnect(arq_t *) { Called() = true; return ARQ_OK_COMPLETED; }
    };

    NANOARQ_HOOK(arq_connect, Local::FakeArqConnect);

    Local::Called() = false;
    arq_connect(nullptr);
    CHECK(Local::Called());
}

TEST(hook, can_unhook_nanoarq_function)
{
    struct Local
    {
        static int& Count() { static int count = 0; return count; }
        static arq_err_t FakeArqConnect(arq_t *) { Count()++; return ARQ_OK_COMPLETED; }
    };

    NANOARQ_HOOK(arq_connect, Local::FakeArqConnect);
    Local::Count() = 0;
    arq_connect(nullptr);
    CHECK_EQUAL(1, Local::Count());
    NANOARQ_UNHOOK(arq_connect);
    arq_connect(nullptr);
    CHECK_EQUAL(1, Local::Count());
}

arq_err_t FailIfCalled(arq_t *)
{
    FAIL("arq_connect was not unhooked between tests");
    return ARQ_OK_COMPLETED;
}

TEST(hook, hooks_removed_after_test_1)
{
    arq_connect(nullptr);
    NANOARQ_HOOK(arq_connect, FailIfCalled);
}

TEST(hook, hooks_removed_after_test_2)
{
    arq_connect(nullptr);
    NANOARQ_HOOK(arq_connect, FailIfCalled);
}

TEST(hook, can_mock_with_hooks)
{
    struct Local
    {
        static arq_err_t MockArqConnect(arq_t *arq)
        {
            return (arq_err_t)mock().actualCall("arq_connect").withParameter("arq", arq).returnIntValue();
        }
    };

    arq_t *mockArq = reinterpret_cast< arq_t * >(0x12345678);
    int mockReturnValue = 123;
    mock().expectOneCall("arq_connect").withParameter("arq", mockArq).andReturnValue(mockReturnValue);

    NANOARQ_HOOK(arq_connect, Local::MockArqConnect);
    int actualReturnValue = arq_connect(mockArq);
    CHECK_EQUAL(mockReturnValue, actualReturnValue);
}

}

