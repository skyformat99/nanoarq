#include "PltHookPlugin.h"
#include <cstdio>

extern "C"
{
    #include <plthook.h>
}

struct PltHookPlugin::Impl
{
    plthook_t *pltHook = nullptr;
};

PltHookPlugin::PltHookPlugin()
    : TestPlugin("PltHookPlugin")
    , m(new Impl)
{
    if (plthook_open(&m->pltHook, nullptr) != 0) {
        std::fprintf(stderr, "PltHookPlugin: plthook_open failed, plugin is disabled.\n");
        this->disable();
    }
}

PltHookPlugin::~PltHookPlugin()
{
    plthook_close(m->pltHook);
}

PltHookPlugin*& PltHookPlugin::WellKnownInstance()
{
    static PltHookPlugin *p = nullptr;
    return p;
}

void PltHookPlugin::Hook(char const *partialFunctionName, void *newFunction)
{
    (void)partialFunctionName;
    (void)newFunction;
}

void PltHookPlugin::Unhook(char const *partialFunctionName)
{
    (void)partialFunctionName;
}

void PltHookPlugin::preTestAction(UtestShell&, TestResult&)
{
}

void PltHookPlugin::postTestAction(UtestShell&, TestResult&)
{
}

