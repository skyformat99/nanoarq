#include <CppUTest/TestHarness.h>
#include <memory>

class PltHookPlugin : public TestPlugin
{
public:
    PltHookPlugin();
    virtual ~PltHookPlugin();
    static PltHookPlugin*& WellKnownInstance();

    void Hook(char const *partialFunctionName, void *newFunction);
    void Unhook(char const *partialFunctionName);

    void preTestAction(UtestShell&, TestResult&) override;
    void postTestAction(UtestShell&, TestResult&) override;

    PltHookPlugin(PltHookPlugin const&) = delete;
    PltHookPlugin& operator =(PltHookPlugin const&) = delete;

private:
    struct Impl;
    std::unique_ptr< Impl > m;
};

