#include "strict_mock_plugin.h"
#include <CppUTestExt/MockSupport.h>

StrictMockPlugin::StrictMockPlugin()
    : TestPlugin("StrictMockPlugin")
{
}

StrictMockPlugin::~StrictMockPlugin()
{
}

void StrictMockPlugin::preTestAction(UtestShell&, TestResult&)
{
    mock().strictOrder();
}

