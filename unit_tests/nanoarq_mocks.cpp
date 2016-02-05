#include "nanoarq_in_test_project.h"

#define NANOARQ_MOCK(FUNCTION_NAME) \
    extern void *FUNCTION_NAME##__; \
    void *FUNCTION_NAME##_NANOARQ_THUNK_TARGET = &FUNCTION_NAME##__; \
    __asm( \
        ".globl _" #FUNCTION_NAME "\n\t"\
        "_" #FUNCTION_NAME ":\n\t"\
        "movq _" #FUNCTION_NAME "_NANOARQ_THUNK_TARGET (%rip), %r11\n\t"\
        "jmp *%r11\n\t"\
    );

NANOARQ_MOCK_LIST()
#undef NANOARQ_MOCK

