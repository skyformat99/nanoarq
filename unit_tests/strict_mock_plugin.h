#pragma once

#include <CppUTest/TestHarness.h>

class StrictMockPlugin : public TestPlugin
{
public:
    StrictMockPlugin();
    virtual ~StrictMockPlugin();

    void preTestAction(UtestShell&, TestResult&) override;

    StrictMockPlugin(StrictMockPlugin const&) = delete;
    StrictMockPlugin& operator =(StrictMockPlugin const&) = delete;
};

