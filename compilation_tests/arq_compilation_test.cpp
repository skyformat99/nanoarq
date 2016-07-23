// including these to ensure no naming collisions with arq.h
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <list>
#include <stack>
#include <queue>
#include <new>
#include <exception>
#include <stdexcept>
#include <memory>
#include <algorithm>
#include <iterator>
#include <cstddef>

#include <stdint.h>

using namespace std;

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
