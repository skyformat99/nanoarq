#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include "strict_mock_plugin.h"
#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestRegistry.h>
#include <CppUTestExt/MemoryReporterPlugin.h>
#include <CppUTestExt/MockSupportPlugin.h>
#include <CppUTest/TestHarness_c.h>

namespace {

void TestAssertHandler(char const *file, int line, char const *cond, char const *msg)
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

    StrictMockPlugin strictMockPlugin;
    TestRegistry::getCurrentRegistry()->installPlugin(&strictMockPlugin);

    ArqRuntimeMockPlugin arqRuntimeMockPlugin;
    ArqRuntimeMockPlugin::WellKnownInstance() = &arqRuntimeMockPlugin;
    TestRegistry::getCurrentRegistry()->installPlugin(&arqRuntimeMockPlugin);

    arq_err_t const e = arq_assert_handler_set(&TestAssertHandler);
    if (e != ARQ_OK_COMPLETED) {
        return 1;
    }

    return CommandLineTestRunner::RunAllTests(argc, argv);
}

