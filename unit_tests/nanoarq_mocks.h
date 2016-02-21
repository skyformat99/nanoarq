#pragma once

#define ARQ_MOCKABLE(FUNCTION_NAME) FUNCTION_NAME##_ARQ_ORIG

#define ARQ_MOCK_LIST() \
    ARQ_MOCK(arq__send_wnd_rst) \
    ARQ_MOCK(arq__send_wnd_send) \
    ARQ_MOCK(arq__send_wnd_ack) \
    ARQ_MOCK(arq__send_wnd_flush) \
    ARQ_MOCK(arq__frame_len) \
    ARQ_MOCK(arq__frame_hdr_read) \
    ARQ_MOCK(arq__frame_hdr_write) \
    ARQ_MOCK(arq__frame_seg_write) \
    ARQ_MOCK(arq__frame_checksum_write) \
    ARQ_MOCK(arq__frame_read) \
    ARQ_MOCK(arq__frame_checksum_read) \
    ARQ_MOCK(arq__frame_write) \
    ARQ_MOCK(arq__cobs_encode) \
    ARQ_MOCK(arq__cobs_decode) \
    ARQ_MOCK(arq__hton32) \
    ARQ_MOCK(arq__ntoh32)

#ifdef __cplusplus
    #define ARQ_MOCK(FUNCTION_NAME) extern "C" { extern void *FUNCTION_NAME##_ARQ_THUNK_TARGET; }
#else
    #define ARQ_MOCK(FUNCTION_NAME) extern void *FUNCTION_NAME##_ARQ_THUNK_TARGET;
#endif

ARQ_MOCK_LIST()
#undef ARQ_MOCK

