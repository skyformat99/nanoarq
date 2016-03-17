#pragma once

#include <CppUTest/TestHarness.h>
#include <memory>

class NanoArqHookPlugin : public TestPlugin
{
public:
    NanoArqHookPlugin();
    virtual ~NanoArqHookPlugin();
    static NanoArqHookPlugin*& WellKnownInstance();

    void Hook(void *origFunction, void *newFunction);
    void Unhook(void *origFunction);
    void postTestAction(UtestShell&, TestResult&) override;

    NanoArqHookPlugin(NanoArqHookPlugin const&) = delete;
    NanoArqHookPlugin& operator =(NanoArqHookPlugin const&) = delete;
private:
    struct Impl;
    std::unique_ptr< Impl > m;
};

#define ARQ_MOCK_HOOK(FUNCTION_NAME, NEW_FUNCTION) \
    do { \
        (void)&FUNCTION_NAME; \
        (void)&NEW_FUNCTION; \
        NanoArqHookPlugin *hookPlugin__ = NanoArqHookPlugin::WellKnownInstance(); \
        if (hookPlugin__) { \
            hookPlugin__->Hook((void *)&FUNCTION_NAME, (void *)NEW_FUNCTION); \
        } \
    } while(0)

#define ARQ_MOCK_UNHOOK(FUNCTION_NAME) \
    do { \
        (void)&FUNCTION_NAME; \
        NanoArqHookPlugin *hookPlugin__ = NanoArqHookPlugin::WellKnownInstance(); \
        if (hookPlugin__) { \
            hookPlugin__->Unhook((void *)&FUNCTION_NAME); \
        } \
    } while(0)

