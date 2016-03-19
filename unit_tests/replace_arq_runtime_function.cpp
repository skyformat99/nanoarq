#include "replace_arq_runtime_function.h"
#include "arq_mock_list.h"
#include <algorithm>

// create all of the prototypes, since they aren't exposed via header file
#define ARQ_MOCK(FUNCTION_NAME) extern void *FUNCTION_NAME##_ARQ_ORIG; extern void *FUNCTION_NAME;
ARQ_MOCK_LIST()
#undef ARQ_MOCK

// define all of the thunk targets
#define ARQ_MOCK(FUNCTION_NAME) void *FUNCTION_NAME##_ARQ_THUNK_TARGET = &FUNCTION_NAME##_ARQ_ORIG;
ARQ_MOCK_LIST()
#undef ARQ_MOCK

struct FunctionRecord
{
    void const *trampolineFunction;
    void **thunkSeat;
};

// create an immutable array of all mocked functions and their thunk targets
namespace {
#define ARQ_MOCK(FUNCTION_NAME) { &FUNCTION_NAME, &FUNCTION_NAME##_ARQ_THUNK_TARGET },
FunctionRecord s_functionRecords[] = { ARQ_MOCK_LIST() };
#undef ARQ_MOCK
}

// clang's convention on OSX is to prefix C functions with _, but gcc and linux don't.
#ifdef __APPLE__
    #define ARQ_SYMBOL_PREFIX "_"
#else
    #define ARQ_SYMBOL_PREFIX ""
#endif

// create the thunk with a tail call so the parameters remain intact.
// x64 abi says r11 is a scratch register, so load the thunk there.
#define ARQ_MOCK(FUNCTION_NAME) \
    __asm( \
        ".text\n\t" \
        ".p2align 4\n\t" \
        ".globl " ARQ_SYMBOL_PREFIX #FUNCTION_NAME "\n\t" \
        ARQ_SYMBOL_PREFIX #FUNCTION_NAME ":\n\t" \
        ".cfi_startproc\n\t" \
        "movq " ARQ_SYMBOL_PREFIX #FUNCTION_NAME "_ARQ_THUNK_TARGET (%rip), %r11\n\t" \
        "jmp *%r11\n\t" \
        ".cfi_endproc\n\t" \
    );

ARQ_MOCK_LIST()
#undef ARQ_MOCK

bool ReplaceArqRuntimeFunction(void *arqRuntimeFunction, void *newFunction, void **out_originalFunction)
{
    auto found = std::find_if(std::begin(s_functionRecords),
                              std::end(s_functionRecords),
                              [=](FunctionRecord const &fr) {
        return fr.trampolineFunction == arqRuntimeFunction;
    });

    if (found == std::end(s_functionRecords)) {
        printf("ReplaceArqRuntimeFunction(): Function '%p' not found.\n", arqRuntimeFunction);
        return false;
    }

    if (out_originalFunction) {
        *out_originalFunction = *found->thunkSeat;
    }
    *found->thunkSeat = newFunction;
    return true;
}

