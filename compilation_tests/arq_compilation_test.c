/* including all of these to ensure no naming collisions in arq.h */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <errno.h>
#include <limits.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <time.h>

#define ARQ_LITTLE_ENDIAN_CPU 1
#define ARQ_COMPILE_CRC32 1

#if ARQ_USE_C_STDLIB == 0
    #define ARQ_UINT16_TYPE uint16_t
    #define ARQ_UINT32_TYPE uint32_t
    #define ARQ_UINTPTR_TYPE uintptr_t
    #define ARQ_NULL_PTR NULL
    #define ARQ_MEMCPY(DST, SRC, LEN) __builtin_memcpy((DST), (SRC), (unsigned)(LEN))
#endif

#define ARQ_IMPLEMENTATION
#include "arq.h"

