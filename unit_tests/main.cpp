#include "nanoarq_in_test_project.h"
#include "nanoarq_hook_plugin.h"

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestRegistry.h>
#include <CppUTestExt/MemoryReporterPlugin.h>
#include <CppUTestExt/MockSupportPlugin.h>
#include <CppUTest/TestHarness_c.h>

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

    MockSupportPlugin mockSupportPlugin;
    TestRegistry::getCurrentRegistry()->installPlugin(&mockSupportPlugin);

    NanoArqHookPlugin nanoArqHookPlugin;
    NanoArqHookPlugin::WellKnownInstance() = &nanoArqHookPlugin;
    TestRegistry::getCurrentRegistry()->installPlugin(&nanoArqHookPlugin);

    arq_err_t const e = arq_assert_handler_set(&test_assert_handler);
    if (e != ARQ_OK_COMPLETED) {
        return 1;
    }

    return CommandLineTestRunner::RunAllTests(argc, argv);
}

