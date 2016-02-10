#pragma once

#define NANOARQ_MOCKABLE(FUNCTION_NAME) FUNCTION_NAME##_NANOARQ_ORIG

#define NANOARQ_MOCK_LIST() \
    NANOARQ_MOCK(arq__frame_len) \
    NANOARQ_MOCK(arq__frame_hdr_read) \
    NANOARQ_MOCK(arq__frame_hdr_write) \
    NANOARQ_MOCK(arq__frame_seg_write) \
    NANOARQ_MOCK(arq__frame_checksum_write) \
    NANOARQ_MOCK(arq__frame_read) \
    NANOARQ_MOCK(arq__frame_checksum_read) \
    NANOARQ_MOCK(arq__frame_write) \
    NANOARQ_MOCK(arq__frame_encode) \
    NANOARQ_MOCK(arq__frame_decode) \
    NANOARQ_MOCK(arq__cobs_encode) \
    NANOARQ_MOCK(arq__cobs_decode) \
    NANOARQ_MOCK(arq__hton32) \
    NANOARQ_MOCK(arq__ntoh32)

#ifdef __cplusplus
    #define NANOARQ_MOCK(FUNCTION_NAME) extern "C" { extern void *FUNCTION_NAME##_NANOARQ_THUNK_TARGET; }
#else
    #define NANOARQ_MOCK(FUNCTION_NAME) extern void *FUNCTION_NAME##_NANOARQ_THUNK_TARGET;
#endif

NANOARQ_MOCK_LIST()
#undef NANOARQ_MOCK

