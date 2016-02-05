#pragma once

#define NANOARQ_MOCKABLE(FUNCTION_NAME) FUNCTION_NAME##__

#define NANOARQ_MOCK_LIST() \
    NANOARQ_MOCK(arq__frame_size) \
    NANOARQ_MOCK(arq__frame_read) \
    NANOARQ_MOCK(arq__frame_write)

#ifdef __cplusplus
    #define NANOARQ_MOCK(FUNCTION_NAME) extern "C" { extern void *FUNCTION_NAME##_NANOARQ_THUNK_TARGET; }
#else
    #define NANOARQ_MOCK(FUNCTION_NAME) extern void *FUNCTION_NAME##_NANOARQ_THUNK_TARGET;
#endif

NANOARQ_MOCK_LIST()
#undef NANOARQ_MOCK

