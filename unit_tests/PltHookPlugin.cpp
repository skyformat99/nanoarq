#include "PltHookPlugin.h"
#include <vector>
#include <cstdio>

extern "C"
{
    #include <plthook.h>
}

struct HookedFunction
{
    HookedFunction(std::string const &name_) : name(name_) {}
    std::string name;
    void *orig = nullptr;
};

struct PltHookPlugin::Impl
{
    plthook_t *pltHook = nullptr;
    std::vector< HookedFunction > hooks;
};

PltHookPlugin::PltHookPlugin()
    : TestPlugin("PltHookPlugin")
    , m(new Impl)
{
    if (plthook_open(&m->pltHook, nullptr) != 0) {
        std::fprintf(stderr, "PltHookPlugin: plthook_open() failed, plugin is disabled.\n");
        this->disable();
    }

    unsigned int i = 0;
    char const *functionName;
    void **functionAddr;
    while (plthook_enum(m->pltHook, &i, &functionName, &functionAddr) == 0) {
        m->hooks.push_back(HookedFunction(functionName));
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
    auto found = find_if(m->hooks.begin(), m->hooks.end(),
        [=](HookedFunction const &f) { return f.name.find(partialFunctionName) != std::string::npos; });

    if (found == m->hooks.end()) {
        std::fprintf(stderr,
                     "PltHookPlugin::Hook(): No function matches '%s', nothing to hook.\n",
                     partialFunctionName);
        return;
    }

    if (plthook_replace(m->pltHook, found->name.c_str(), newFunction, &found->orig) != 0) {
        std::fprintf(stderr,
                     "PltHookPlugin::Hook(): plthook_replace() failed with '%s'\n",
                     found->name.c_str());
    }
}

void PltHookPlugin::Unhook(char const *partialFunctionName)
{
    auto found = find_if(m->hooks.begin(), m->hooks.end(),
        [=](HookedFunction const &f) { return f.name.find(partialFunctionName) != std::string::npos; });

    if (found == m->hooks.end()) {
        std::fprintf(stderr,
                     "PltHookPlugin::Unhook(): No function matches '%s', nothing to unhook.\n",
                     partialFunctionName);
        return;
    }

    void *old_hook;
    if (plthook_replace(m->pltHook, found->name.c_str(), found->orig, &old_hook) != 0) {
        std::fprintf(stderr,
                     "PltHookPlugin::Unhook(): plthook_replace() failed with '%s'\n",
                     found->name.c_str());
        return;
    }

    found->orig = nullptr;
}

void PltHookPlugin::postTestAction(UtestShell&, TestResult&)
{
    for (auto& hook : m->hooks) {
        if (hook.orig == nullptr) {
            continue;
        }

        void *old_hook;
        if (plthook_replace(m->pltHook, hook.name.c_str(), hook.orig, &old_hook) != 0) {
            std::fprintf(stderr,
                         "PltHookPlugin::postTestAction(): plthook_replace() failed on '%s'\n",
                         hook.name.c_str());
        }

        hook.orig = nullptr;
    }
}

