#pragma once

#include <CppUTest/TestHarness.h>
#include <memory>

class ArqRuntimeMockPlugin : public TestPlugin
{
public:
    ArqRuntimeMockPlugin();
    virtual ~ArqRuntimeMockPlugin();
    static ArqRuntimeMockPlugin*& WellKnownInstance();

    void Hook(void *origFunction, void *newFunction);
    void Unhook(void *origFunction);
    void postTestAction(UtestShell&, TestResult&) override;

    ArqRuntimeMockPlugin(ArqRuntimeMockPlugin const&) = delete;
    ArqRuntimeMockPlugin& operator =(ArqRuntimeMockPlugin const&) = delete;
private:
    struct Impl;
    std::unique_ptr< Impl > m;
};

#define ARQ_MOCK_HOOK(FUNCTION_NAME, NEW_FUNCTION) \
    do { \
        (void)&FUNCTION_NAME; \
        (void)&NEW_FUNCTION; \
        ArqRuntimeMockPlugin *plugin_ = ArqRuntimeMockPlugin::WellKnownInstance(); \
        if (plugin_) { \
            plugin_->Hook((void *)&FUNCTION_NAME, (void *)NEW_FUNCTION); \
        } \
    } while(0)

#define ARQ_MOCK_UNHOOK(FUNCTION_NAME) \
    do { \
        (void)&FUNCTION_NAME; \
        ArqRuntimeMockPlugin *plugin_ = ArqRuntimeMockPlugin::WellKnownInstance(); \
        if (plugin_) { \
            plugin_->Unhook((void *)&FUNCTION_NAME); \
        } \
    } while(0)

