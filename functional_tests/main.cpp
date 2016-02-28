#include "nanoarq_in_test_project.h"

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestRegistry.h>
#include <CppUTestExt/MemoryReporterPlugin.h>
#include <CppUTest/TestHarness_c.h>

namespace
{

void TestAssertHandler(char const *file, int line, char const *cond, char const *msg)
{
    (void)msg;
    FAIL_TEXT_C_LOCATION(cond, file, (int)line);
}

}

int main(int, char *argv[])
{
    MemoryReporterPlugin memoryReporterPlugin;
    TestRegistry::getCurrentRegistry()->installPlugin(&memoryReporterPlugin);

    arq_err_t const e = arq_assert_handler_set(&TestAssertHandler);
    if (e != ARQ_OK_COMPLETED) {
        return 1;
    }

    char const *verbose_argv[] = { argv[0], "-v" };
    return CommandLineTestRunner::RunAllTests(2, verbose_argv);
}


