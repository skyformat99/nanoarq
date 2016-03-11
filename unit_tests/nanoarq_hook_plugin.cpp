#include "nanoarq_hook_plugin.h"
#include <vector>
#include <algorithm>

using namespace std;

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
    auto hr = find_if(begin(m->hooks), end(m->hooks), [=](HookRecord &h){ return h.thunkAddr == thunkAddr; });

    if (hr == end(m->hooks)) {
        fprintf(stderr,
                "NanoArqHookPlugin::Unhook(): No function matches '%p', nothing to unhook.\n",
                thunkAddr);
        return;
    }

    *hr->thunkAddr = hr->orig;
    m->hooks.erase(hr);
}

void NanoArqHookPlugin::postTestAction(UtestShell&, TestResult&)
{
    for_each(begin(m->hooks), end(m->hooks), [](HookRecord& h) { *h.thunkAddr = h.orig; });
    m->hooks.clear();
}

