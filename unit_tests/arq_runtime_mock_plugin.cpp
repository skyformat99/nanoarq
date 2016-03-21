#include "arq_runtime_mock_plugin.h"
#include "replace_arq_runtime_function.h"
#include <vector>
#include <algorithm>

using namespace std;

struct HookRecord
{
    HookRecord(void *arqRuntimeFunction_, void *originalFunction_)
        : arqRuntimeFunction(arqRuntimeFunction_)
        , originalFunction(originalFunction_)
    {
    }
    void *arqRuntimeFunction = nullptr;
    void *originalFunction = nullptr;
};

struct ArqRuntimeMockPlugin::Impl
{
    std::vector< HookRecord > hooks;
};

ArqRuntimeMockPlugin::ArqRuntimeMockPlugin()
    : TestPlugin("ArqRuntimeMockPlugin")
    , m(new Impl)
{
    m->hooks.reserve(256);
}

ArqRuntimeMockPlugin::~ArqRuntimeMockPlugin()
{
}

ArqRuntimeMockPlugin*& ArqRuntimeMockPlugin::WellKnownInstance()
{
    static ArqRuntimeMockPlugin *p = nullptr;
    return p;
}

void ArqRuntimeMockPlugin::Hook(void *arqRuntimeFunction, void *newFunction)
{
    void *originalFunction = nullptr;
    bool const ok = ReplaceArqRuntimeFunction(arqRuntimeFunction, newFunction, &originalFunction);
    if (ok) {
        m->hooks.emplace_back(HookRecord(arqRuntimeFunction, originalFunction));
    } else {
        fprintf(stderr, "ArqRuntimeMockPlugin::Hook() No function found matching '%p', nothing to hook.\n",
                arqRuntimeFunction);
    }
}

void ArqRuntimeMockPlugin::Unhook(void *arqRuntimeFunction)
{
    auto found = std::find_if(std::begin(m->hooks), std::end(m->hooks), [=](HookRecord const &h) {
        return h.arqRuntimeFunction == arqRuntimeFunction;
    });
    if (found == std::end(m->hooks)) {
        fprintf(stderr,
                "ArqRuntimeMockPlugin::Unhook(): No function matches '%p', nothing to unhook.\n",
                arqRuntimeFunction);
        return;
    }
    bool const ok = ReplaceArqRuntimeFunction(found->arqRuntimeFunction, found->originalFunction, nullptr);
    if (!ok) {
        fprintf(stderr,
                "ArqRuntimeMockPlugin::Unhook(): Unhooking function at '%p' failed!\n",
                found->arqRuntimeFunction);
    }
    m->hooks.erase(found);
}

void ArqRuntimeMockPlugin::postTestAction(UtestShell&, TestResult&)
{
    std::for_each(std::begin(m->hooks), std::end(m->hooks), [](HookRecord const& h) {
        ReplaceArqRuntimeFunction(h.arqRuntimeFunction, h.originalFunction, nullptr);
    });
    m->hooks.clear();
}

