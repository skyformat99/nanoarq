#include "nanoarq_in_test_project.h"

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestRegistry.h>
#include <CppUTestExt/MemoryReporterPlugin.h>
#include <CppUTest/TestHarness_c.h>

extern "C"
{
    #include <plthook.h>
}

namespace
{

void test_assert_handler(char const *file, int line, char const *cond, char const *msg)
{
    (void)msg;
    FAIL_TEXT_C_LOCATION(cond, file, (int)line);
}

}

int main(int argc, char *argv[])
{
    MemoryReporterPlugin memoryReporterPlugin;
    TestRegistry::getCurrentRegistry()->installPlugin(&memoryReporterPlugin);

    plthook_t *plthook = nullptr;

    arq_err_t const e = arq_set_assert_handler(&test_assert_handler);
    if (e != ARQ_OK_COMPLETED) {
        return 1;
    }

    auto success = CommandLineTestRunner::RunAllTests(argc, argv);

    plthook_close(plthook);
    return success;
}
