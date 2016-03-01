#include "nanoarq_unit_test.h"

#ifdef __APPLE__
#define ARQ_SYMBOL_PREFIX "_"
#else
#define ARQ_SYMBOL_PREFIX ""
#endif

#define ARQ_MOCK(FUNCTION_NAME) \
    extern void *FUNCTION_NAME##_ARQ_ORIG; \
    void *FUNCTION_NAME##_ARQ_THUNK_TARGET = &FUNCTION_NAME##_ARQ_ORIG;

ARQ_MOCK_LIST()
#undef ARQ_MOCK

#define ARQ_MOCK(FUNCTION_NAME) \
    __asm( \
        ".text\n\t" \
        ".p2align 4,,15\n\t" \
        ".globl " ARQ_SYMBOL_PREFIX #FUNCTION_NAME "\n\t" \
        ARQ_SYMBOL_PREFIX #FUNCTION_NAME ":\n\t" \
        ".cfi_startproc\n\t" \
        "movq " ARQ_SYMBOL_PREFIX #FUNCTION_NAME "_ARQ_THUNK_TARGET (%rip), %r11\n\t" \
        "jmp *%r11\n\t" \
        ".cfi_endproc\n\t" \
    );

ARQ_MOCK_LIST()
#undef ARQ_MOCK

