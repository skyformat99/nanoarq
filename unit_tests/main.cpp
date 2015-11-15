#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestRegistry.h>
#include <CppUTestExt/MemoryReporterPlugin.h>

extern "C"
{
    #include <plthook.h>
}

int main(int argc, char *argv[])
{
    MemoryReporterPlugin memoryReporterPlugin;
    TestRegistry::getCurrentRegistry()->installPlugin(&memoryReporterPlugin);

    plthook_t *plthook = nullptr;
    auto success = CommandLineTestRunner::RunAllTests(argc, argv);

    plthook_close(plthook);
    return success;
}
