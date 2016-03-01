#define ARQ_LITTLE_ENDIAN_CPU 1
#define ARQ_COMPILE_CRC32 1

#include <cstddef>

#if ARQ_USE_C_STDLIB == 0
    #define ARQ_UINT16_TYPE unsigned short
    #define ARQ_UINT32_TYPE unsigned int
    #define ARQ_UINTPTR_TYPE unsigned long
    #define ARQ_NULL_PTR nullptr
    #define ARQ_MEMCPY(DST, SRC, LEN) __builtin_memcpy((DST), (SRC), (unsigned)(LEN))
#endif

#define ARQ_IMPLEMENTATION
#include "arq.h"

