#include "nanoarq_hook_plugin.h"
#include <vector>
#include <algorithm>

struct HookRecord
{
    explicit HookRecord(void **thunkAddr_) : thunkAddr(thunkAddr_), orig(*thunkAddr_) {}
    void **thunkAddr = nullptr;
    void *orig = nullptr;
};

struct NanoArqHookPlugin::Impl
{
    std::vector< HookRecord > hooks;
};

NanoArqHookPlugin::NanoArqHookPlugin()
    : TestPlugin("NanoArqHookPlugin")
    , m(new Impl)
{
    m->hooks.reserve(256);
}

NanoArqHookPlugin::~NanoArqHookPlugin()
{
}

NanoArqHookPlugin*& NanoArqHookPlugin::WellKnownInstance()
{
    static NanoArqHookPlugin *p = nullptr;
    return p;
}

void NanoArqHookPlugin::Hook(void **thunkAddress, void *newFunction)
{
    m->hooks.emplace_back(HookRecord(thunkAddress));
    *thunkAddress = newFunction;
}

void NanoArqHookPlugin::Unhook(void **thunkAddr)
{
    auto hr = std::find_if(std::begin(m->hooks), std::end(m->hooks),
        [=](HookRecord const &h){ return h.thunkAddr == thunkAddr; });

    if (hr == std::end(m->hooks)) {
        std::fprintf(stderr,
                     "NanoArqHookPlugin::Unhook(): No function matches '%p', nothing to unhook.\n",
                     thunkAddr);
        return;
    }

    *hr->thunkAddr = hr->orig;
    m->hooks.erase(hr);
}

void NanoArqHookPlugin::postTestAction(UtestShell&, TestResult&)
{
    std::for_each(std::begin(m->hooks), std::end(m->hooks), [](HookRecord& h) { *h.thunkAddr = h.orig; });
    m->hooks.clear();
}

