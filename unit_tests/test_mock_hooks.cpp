#include "nanoarq_in_test_project.h"
#include "nanoarq_hook_plugin.h"

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(mock_hooks) {};

namespace
{

TEST(mock_hooks, can_hook_nanoarq_function)
{
    struct Local
    {
        static bool& Called() { static bool called = false; return called; }
        static void MockFrameSize(int) { Called() = true; }
    };

    ARQ_MOCK_HOOK(arq__frame_len, Local::MockFrameSize);
    Local::Called() = false;
    arq__frame_len(16);
    CHECK(Local::Called());
}

TEST(mock_hooks, can_unhook_nanoarq_function)
{
    struct Local
    {
        static int& Count() { static int count = 0; return count; }
        static void MockFrameSize(int) { Count()++; }
    };

    ARQ_MOCK_HOOK(arq__frame_len, Local::MockFrameSize);
    Local::Count() = 0;
    arq__frame_len(8);
    CHECK_EQUAL(1, Local::Count());
    ARQ_MOCK_UNHOOK(arq__frame_len);
    arq__frame_len(8);
    CHECK_EQUAL(1, Local::Count());
}

void FailIfCalled(int)
{
    FAIL("arq__frame_len was not unhooked between tests");
}

TEST(mock_hooks, hooks_removed_after_test_1)
{
    arq__frame_len(24);
    ARQ_MOCK_HOOK(arq__frame_len, FailIfCalled);
}

TEST(mock_hooks, hooks_removed_after_test_2)
{
    arq__frame_len(24);
    ARQ_MOCK_HOOK(arq__frame_len, FailIfCalled);
}

TEST(mock_hooks, can_mock_with_hooks)
{
    struct Local
    {
        static int MockArqFrameSize(int segmentLength)
        {
            return mock().actualCall("arq__frame_len").withParameter("seg_len", segmentLength).returnIntValue();
        }
    };

    int const mockSegmentLength = 67;
    int const mockReturnValue = 123;
    mock().expectOneCall("arq__frame_len").withParameter("seg_len", mockSegmentLength).andReturnValue(mockReturnValue);

    ARQ_MOCK_HOOK(arq__frame_len, Local::MockArqFrameSize);
    int const actualReturnValue = arq__frame_len(mockSegmentLength);
    CHECK_EQUAL(mockReturnValue, actualReturnValue);
}

}

