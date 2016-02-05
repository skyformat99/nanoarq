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

    NANOARQ_MOCK_HOOK(arq__frame_size, Local::MockFrameSize);
    Local::Called() = false;
    arq__frame_size(16);
    CHECK(Local::Called());
}

TEST(mock_hooks, can_unhook_nanoarq_function)
{
    struct Local
    {
        static int& Count() { static int count = 0; return count; }
        static void MockFrameSize(int) { Count()++; }
    };

    NANOARQ_MOCK_HOOK(arq__frame_size, Local::MockFrameSize);
    Local::Count() = 0;
    arq__frame_size(8);
    CHECK_EQUAL(1, Local::Count());
    NANOARQ_MOCK_UNHOOK(arq__frame_size);
    arq__frame_size(8);
    CHECK_EQUAL(1, Local::Count());
}

void FailIfCalled(int)
{
    FAIL("arq__frame_size was not unhooked between tests");
}

TEST(mock_hooks, hooks_removed_after_test_1)
{
    arq__frame_size(24);
    NANOARQ_MOCK_HOOK(arq__frame_size, FailIfCalled);
}

TEST(mock_hooks, hooks_removed_after_test_2)
{
    arq__frame_size(24);
    NANOARQ_MOCK_HOOK(arq__frame_size, FailIfCalled);
}

TEST(mock_hooks, can_mock_with_hooks)
{
    struct Local
    {
        static int MockArqFrameSize(int segmentLength)
        {
            return mock().actualCall("arq__frame_size").withParameter("seg_len", segmentLength).returnIntValue();
        }
    };

    int const mockSegmentLength = 67;
    int const mockReturnValue = 123;
    mock().expectOneCall("arq__frame_size").withParameter("seg_len", mockSegmentLength).andReturnValue(mockReturnValue);

    NANOARQ_MOCK_HOOK(arq__frame_size, Local::MockArqFrameSize);
    int const actualReturnValue = arq__frame_size(mockSegmentLength);
    CHECK_EQUAL(mockReturnValue, actualReturnValue);
}

}

