#pragma once

#include <CppUTest/TestHarness.h>
#include <memory>

class NanoArqHookPlugin : public TestPlugin
{
public:
    NanoArqHookPlugin();
    virtual ~NanoArqHookPlugin();
    static NanoArqHookPlugin*& WellKnownInstance();

    void Hook(void **thunkAddress, void *newFunction);
    void Unhook(void **thunkAddress);
    void postTestAction(UtestShell&, TestResult&) override;

    NanoArqHookPlugin(NanoArqHookPlugin const&) = delete;
    NanoArqHookPlugin& operator =(NanoArqHookPlugin const&) = delete;
private:
    struct Impl;
    std::unique_ptr< Impl > m;
};

#define NANOARQ_MOCK_HOOK(FUNCTION_NAME, NEW_FUNCTION) \
    do { \
        (void)&FUNCTION_NAME; \
        (void)&NEW_FUNCTION; \
        NanoArqHookPlugin *hookPlugin__ = NanoArqHookPlugin::WellKnownInstance(); \
        if (hookPlugin__) { \
            hookPlugin__->Hook(&FUNCTION_NAME##_NANOARQ_THUNK_TARGET, reinterpret_cast< void * >(NEW_FUNCTION)); \
        } \
    } while(0)

#define NANOARQ_MOCK_UNHOOK(FUNCTION_NAME) \
    do { \
        (void)&FUNCTION_NAME; \
        NanoArqHookPlugin *hookPlugin__ = NanoArqHookPlugin::WellKnownInstance(); \
        if (hookPlugin__) { \
            hookPlugin__->Unhook(&FUNCTION_NAME##_NANOARQ_THUNK_TARGET); \
        } \
    } while(0)

