#include "nanoarq_in_test_project.h"

#ifdef __APPLE__
#define NANOARQ_SYMBOL_PREFIX "_"
#else
#define NANOARQ_SYMBOL_PREFIX ""
#endif

#define NANOARQ_MOCK(FUNCTION_NAME) \
    extern void *FUNCTION_NAME##_NANOARQ_ORIG; \
    void *FUNCTION_NAME##_NANOARQ_THUNK_TARGET = &FUNCTION_NAME##_NANOARQ_ORIG;

NANOARQ_MOCK_LIST()
#undef NANOARQ_MOCK

#define NANOARQ_MOCK(FUNCTION_NAME) \
    __asm( \
        ".text\n\t" \
        ".p2align 4,,15\n\t" \
        ".globl " NANOARQ_SYMBOL_PREFIX #FUNCTION_NAME "\n\t" \
        NANOARQ_SYMBOL_PREFIX #FUNCTION_NAME ":\n\t" \
        "movq " NANOARQ_SYMBOL_PREFIX #FUNCTION_NAME "_NANOARQ_THUNK_TARGET (%rip), %r11\n\t" \
        "jmp *%r11\n\t" \
    );

NANOARQ_MOCK_LIST()
#undef NANOARQ_MOCK

