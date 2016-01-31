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

    void postTestAction(UtestShell&, TestResult&) override;

    PltHookPlugin(PltHookPlugin const&) = delete;
    PltHookPlugin& operator =(PltHookPlugin const&) = delete;

private:
    struct Impl;
    std::unique_ptr< Impl > m;
};

#define NANOARQ_HOOK(FUNCTION_NAME, NEW_FUNCTION) \
    do { \
        (void)FUNCTION_NAME; \
        (void)NEW_FUNCTION; \
        PltHookPlugin *pltHookPlugin__ = PltHookPlugin::WellKnownInstance(); \
        if (pltHookPlugin__) { \
            pltHookPlugin__->Hook(#FUNCTION_NAME, reinterpret_cast< void * >(NEW_FUNCTION)); \
        } else { \
            FAIL("Test called NANOARQ_HOOK, but PltHookPlugin is not initialized."); \
        } \
    } while(0)

#define NANOARQ_UNHOOK(FUNCTION_NAME) \
    do { \
        (void)FUNCTION_NAME; \
        PltHookPlugin *pltHookPlugin__ = PltHookPlugin::WellKnownInstance(); \
        if (pltHookPlugin__) { \
            pltHookPlugin__->Unhook(#FUNCTION_NAME); \
        } else { \
            FAIL("Test tried to NANOARQ_UNHOOK, but PltHookPlugin is not initialized."); \
        } \
    } while(0)

