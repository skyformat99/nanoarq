/* This code is in the public domain. See the LICENSE file for details. */
#ifndef NANOARQ_H_INCLUDED
#define NANOARQ_H_INCLUDED

#ifndef NANOARQ_UINT16_TYPE
#error You must define NANOARQ_UINT16_TYPE before including nanoarq.h
#endif

#ifndef NANOARQ_UINT32_TYPE
#error You must define NANOARQ_UINT32_TYPE before including nanoarq.h
#endif

#ifndef NANOARQ_UINTPTR_TYPE
#error You must define NANOARQ_UINTPTR_TYPE before including nanoarq.h
#endif

#ifndef NANOARQ_NULL_PTR
#error You must define NANOARQ_NULL_PTR before including nanoarq.h
#endif

#ifndef NANOARQ_LITTLE_ENDIAN_CPU
#error You must define NANOARQ_LITTLE_ENDIAN_CPU to 0 or 1 before including nanoarq.h
#endif

#ifndef NANOARQ_COMPILE_CRC32
#error You must define NANOARQ_COMPILE_CRC32 to 0 or 1 before including nanoarq.h
#endif

#ifndef NANOARQ_ASSERTS_ENABLED
#error You must define NANOARQ_ASSERTS_ENABLED to 0 or 1 before including nanoarq.h
#endif

#ifndef NANOARQ_MOCKABLE
#define NANOARQ_MOCKABLE(FUNC) FUNC
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef NANOARQ_UINT16_TYPE arq_uint16_t;
typedef NANOARQ_UINT32_TYPE arq_uint32_t;
typedef NANOARQ_UINTPTR_TYPE arq_uintptr_t;
typedef unsigned char arq_uchar_t;

typedef enum
{
    ARQ_OK_COMPLETED = 0,
    ARQ_OK_MORE,
    ARQ_ERR_INVALID_PARAM,
    ARQ_ERR_SET_ASSERT_BEFORE_INIT
} arq_err_t;

typedef enum
{
    ARQ_EVENT_NONE,
    ARQ_EVENT_CONN_ESTABLISHED,
    ARQ_EVENT_CONN_CLOSED,
    ARQ_EVENT_CONN_RESET_BY_PEER,
    ARQ_EVENT_CONN_LOST_PEER_TIMEOUT
} arq_event_t;

typedef unsigned int arq_time_t;
typedef void (*arq_assert_cb_t)(char const *file, int line, char const *cond, char const *msg);
typedef arq_uint32_t (*arq_checksum_cb_t)(void const *p, int len);

typedef struct arq_cfg_t
{
    int segment_size_in_bytes;
    int message_size_in_segments;
    int send_window_size_in_messages;
    int recv_window_size_in_messages;
    arq_time_t tinygram_send_delay;
    arq_time_t retransmission_timeout;
    arq_time_t keepalive_period;
    arq_time_t disconnect_timeout;
    arq_checksum_cb_t checksum_cb;
} arq_cfg_t;

typedef struct arq_stats_t
{
    int bytes_sent;
    int bytes_recvd;
    int frames_sent;
    int frames_recvd;
    int messages_sent;
    int messages_recvd;
    int corrupted_frames_recvd;
    int retransmitted_frames_sent;
} arq_stats_t;

typedef enum
{
    ARQ_STATE_CLOSED,
    ARQ_STATE_RST_RECVD,
    ARQ_STATE_RST_SENT,
    ARQ_STATE_ESTABLISHED,
    ARQ_STATE_CLOSE_WAIT,
    ARQ_STATE_LAST_ACK,
    ARQ_STATE_FIN_WAIT_1,
    ARQ_STATE_FIN_WAIT_2,
    ARQ_STATE_CLOSING,
    ARQ_STATE_TIME_WAIT
} arq_state_t;

struct arq_t;

arq_err_t arq_assert_handler_set(arq_assert_cb_t assert_cb);
arq_err_t arq_assert_handler_get(arq_assert_cb_t *out_assert_cb);
arq_err_t arq_required_size(arq_cfg_t const *cfg, int *out_required_size);
arq_err_t arq_init(arq_cfg_t const *cfg, void *arq_seat, int arq_seat_size, struct arq_t **out_arq);
arq_err_t arq_connect(struct arq_t *arq);
arq_err_t arq_reset(struct arq_t *arq);
arq_err_t arq_close(struct arq_t *arq);
arq_err_t arq_recv(struct arq_t *arq, void *recv, int recv_max, int *out_recv_size);
arq_err_t arq_send(struct arq_t *arq, void *const send, int send_max, int *out_sent_size);
arq_err_t arq_flush_tinygrams(struct arq_t *arq);

arq_err_t arq_backend_poll(struct arq_t *arq,
                           arq_time_t dt,
                           int *out_drain_send_size,
                           int *out_recv_size,
                           arq_event_t *out_event,
                           arq_time_t *out_next_poll);

arq_err_t arq_backend_drain_send(struct arq_t *arq, void *out_send, int send_max, int *out_send_size);
arq_err_t arq_backend_fill_recv(struct arq_t *arq, void const *recv, int recv_max, int *out_recv_size);

#if NANOARQ_COMPILE_CRC32 == 1
arq_uint32_t arq_crc32(void const *buf, int size);
#endif

/* Internal API */

typedef struct arq_t
{
    arq_cfg_t cfg;
    arq_stats_t stats;
    arq_state_t state;
} arq_t;

typedef struct arq__lin_alloc_t
{
    arq_uchar_t *base;
    arq_uchar_t *top;
    int capacity;
} arq__lin_alloc_t;

void arq__lin_alloc_init(arq__lin_alloc_t *a, void *base, int capacity);
void *arq__lin_alloc_alloc(arq__lin_alloc_t *a, int size, int align);

enum
{
    ARQ_FRAME_HEADER_SIZE = 12,
    ARQ_FRAME_COBS_OVERHEAD = 2
};

typedef struct arq__frame_hdr_t
{
    int version;
    int seg_len;
    int win_size;
    int seq_num;
    int msg_len;
    int seg_id;
    int ack_num;
    arq_uint16_t ack_seg_mask;
    char rst;
    char fin;
} arq__frame_hdr_t;

int  arq__frame_len(int seg_len);

void arq__frame_write(arq__frame_hdr_t const *hdr,
                      void const *seg,
                      arq_checksum_cb_t checksum,
                      void *out_frame,
                      int frame_max);
int  arq__frame_hdr_write(arq__frame_hdr_t const *hdr, void *out_frame);
int  arq__frame_seg_write(void const *seg, void *out_frame, int len);
void arq__frame_checksum_write(arq_checksum_cb_t checksum, void *checksum_seat, void *frame, int len);
void arq__frame_encode(void *frame, int len);

typedef enum
{
    ARQ_FRAME_READ_RESULT_SUCCESS,
    ARQ_FRAME_READ_RESULT_ERR_CHECKSUM,
    ARQ_FRAME_READ_RESULT_ERR_MALFORMED
} arq__frame_read_result_t;

arq__frame_read_result_t arq__frame_read(void *frame,
                                         int frame_len,
                                         arq_checksum_cb_t checksum,
                                         arq__frame_hdr_t *out_hdr,
                                         void const **out_seg);
void arq__frame_decode(void *frame, int len);
void arq__frame_hdr_read(void const *buf, arq__frame_hdr_t *out_frame_hdr);
arq__frame_read_result_t arq__frame_checksum_read(void const *frame,
                                                  int frame_len,
                                                  int seg_len,
                                                  arq_checksum_cb_t checksum);

arq_uint16_t arq__hton16(arq_uint16_t x);
arq_uint16_t arq__ntoh16(arq_uint16_t x);
arq_uint32_t arq__hton32(arq_uint32_t x);
arq_uint32_t arq__ntoh32(arq_uint32_t x);

void arq__cobs_encode(void *p, int len);
void arq__cobs_decode(void *p, int len);

#if defined(__cplusplus)
}
#endif
#endif

#ifdef NANOARQ_IMPLEMENTATION

#ifdef NANOARQ_IMPLEMENTATION_INCLUDED
#error nanoarq.h already #included with NANOARQ_IMPLEMENTATION_INCLUDED #defined
#endif

#define NANOARQ_IMPLEMENTATION_INCLUDED

#if NANOARQ_ASSERTS_ENABLED == 1
static arq_assert_cb_t s_assert_cb = NANOARQ_NULL_PTR;
#define NANOARQ_ASSERT(COND) do { if (!(COND)) { s_assert_cb(__FILE__, __LINE__, #COND, ""); } } while (0)
#define NANOARQ_ASSERT_FAIL() s_assert_cb(__FILE__, __LINE__, "", "explicit assert")
#else
#define NANOARQ_ASSERT(COND) (void)sizeof(COND)
#define NANOARQ_ASSERT_FAIL()
#endif

arq_err_t arq_assert_handler_set(arq_assert_cb_t assert_cb)
{
    (void)assert_cb;
#if NANOARQ_ASSERTS_ENABLED == 1
    if (!assert_cb) {
        return ARQ_ERR_INVALID_PARAM;
    }
    s_assert_cb = assert_cb;
#endif
    return ARQ_OK_COMPLETED;
}

arq_err_t arq_assert_handler_get(arq_assert_cb_t *out_assert_cb)
{
#if NANOARQ_ASSERTS_ENABLED == 1
    if (!out_assert_cb) {
        return ARQ_ERR_INVALID_PARAM;
    }
    *out_assert_cb = s_assert_cb;
#else
    *out_assert_cb = NANOARQ_NULL_PTR;
#endif
    return ARQ_OK_COMPLETED;
}

arq_err_t arq_required_size(arq_cfg_t const *cfg, int *out_required_size)
{
    (void)cfg;
    *out_required_size = 0;
    return ARQ_OK_COMPLETED;
}

arq_err_t arq_connect(struct arq_t *arq)
{
    (void)arq;
    return ARQ_OK_COMPLETED;
}

arq_err_t arq_backend_drain_send(struct arq_t *arq, void *out_send, int send_max, int *out_send_size)
{
    (void)arq;
    (void)out_send;
    (void)send_max;
    (void)out_send_size;
    return ARQ_OK_COMPLETED;
}

void arq__lin_alloc_init(arq__lin_alloc_t *a, void *base, int capacity)
{
    NANOARQ_ASSERT(a && base && (capacity > 0));
    a->base = (arq_uchar_t *)base;
    a->top = (arq_uchar_t *)base;
    a->capacity = capacity;
}

void *arq__lin_alloc_alloc(arq__lin_alloc_t *a, int size, int align)
{
    arq_uchar_t *p =
        (arq_uchar_t *)(((arq_uintptr_t)a->top + ((arq_uintptr_t)align - 1)) & ~((arq_uintptr_t)align - 1));
    arq_uchar_t *new_top = p + size;
    int new_size = new_top - a->base;
    NANOARQ_ASSERT(a && (size > 0) && (align <= 32) && (align > 0) && ((align & (align - 1)) == 0));
    NANOARQ_ASSERT(new_size <= a->capacity);
    if (new_size > a->capacity) {
        return NANOARQ_NULL_PTR;
    }
    a->top = new_top;
    return p;
}

arq_uint16_t arq__hton16(arq_uint16_t x)
{
#if NANOARQ_LITTLE_ENDIAN_CPU == 1
    return (x << 8) | (x >> 8);
#else
    return x;
#endif
}

arq_uint16_t arq__ntoh16(arq_uint16_t x)
{
    return arq__hton16(x);
}

arq_uint32_t NANOARQ_MOCKABLE(arq__hton32)(arq_uint32_t x)
{
#if NANOARQ_LITTLE_ENDIAN_CPU == 1
    return (x >> 24) | ((x & 0x00FF0000) >> 8) | ((x & 0x0000FF00) << 8) | (x << 24);
#else
    return x;
#endif
}

arq_uint32_t NANOARQ_MOCKABLE(arq__ntoh32)(arq_uint32_t x)
{
    return arq__hton32(x);
}

void NANOARQ_MOCKABLE(arq__frame_hdr_read)(void const *buf, arq__frame_hdr_t *out_frame_hdr)
{
    arq_uchar_t const *src = (arq_uchar_t const *)buf;
    arq_uint16_t tmp_n;
    arq_uchar_t *dst = (arq_uchar_t *)&tmp_n;
    NANOARQ_ASSERT(buf && out_frame_hdr);
    out_frame_hdr->version = *src++;                    /* version */
    out_frame_hdr->seg_len = *src++;                    /* seg_len */
    out_frame_hdr->fin = !!(*src & (1 << 0));           /* flags */
    out_frame_hdr->rst = !!(*src++ & (1 << 1));
    out_frame_hdr->win_size = *src++;                   /* win_size */
    dst[0] = src[0] >> 4;
    dst[1] = (src[0] << 4) | (src[1] >> 4);
    out_frame_hdr->seq_num = arq__ntoh16(tmp_n);        /* seq_num */
    ++src;
    out_frame_hdr->msg_len = (arq_uchar_t)((src[0] << 4) | (src[1] >> 4)); /* msg_len */
    ++src;
    dst[0] = src[0] & 0x0F;                             /* seg_id */
    dst[1] = src[1];
    out_frame_hdr->seg_id = arq__ntoh16(tmp_n);
    src += 2;
    dst[0] = src[0] >> 4;                               /* ack_num */
    dst[1] = (src[0] << 4) | (src[1] >> 4);
    out_frame_hdr->ack_num = arq__ntoh16(tmp_n);
    src += 2;
    dst[0] = src[0] & 0x0F;                             /* ack_seg_mask */
    dst[1] = src[1];
    out_frame_hdr->ack_seg_mask = arq__ntoh16(tmp_n);
    src += 2;
    NANOARQ_ASSERT((src - (arq_uchar_t const *)buf) == ARQ_FRAME_HEADER_SIZE);
}

int NANOARQ_MOCKABLE(arq__frame_hdr_write)(arq__frame_hdr_t const *frame_hdr, void *out_buf)
{
    arq_uchar_t *dst = (arq_uchar_t *)out_buf;
    arq_uint16_t tmp_n;
    arq_uchar_t const *src = (arq_uchar_t const *)&tmp_n;
    NANOARQ_ASSERT(frame_hdr && out_buf && ((frame_hdr->ack_seg_mask & 0xF000) == 0));
    *dst++ = (arq_uchar_t)frame_hdr->version;                          /* version */
    *dst++ = (arq_uchar_t)frame_hdr->seg_len;                          /* seg_len */
    *dst++ = (!!frame_hdr->fin) | ((!!frame_hdr->rst) << 1);           /* flags */
    *dst++ = (arq_uchar_t)frame_hdr->win_size;                         /* win_size */
    tmp_n = arq__hton16((arq_uint16_t)frame_hdr->seq_num);             /* seq_num + msg_len */
    *dst++ = (src[0] << 4) | (src[1] >> 4);
    *dst++ = (src[1] << 4) | (arq_uchar_t)(frame_hdr->msg_len >> 4);
    tmp_n = arq__hton16((arq_uint16_t)frame_hdr->seg_id);              /* msg_len + seg_id */
    *dst++ = (arq_uchar_t)(frame_hdr->msg_len << 4) | (src[0] & 0x0F);
    *dst++ = src[1];
    tmp_n = arq__hton16((arq_uint16_t)frame_hdr->ack_num);             /* ack_num */
    *dst++ = (src[0] << 4) | (src[1] >> 4);
    *dst++ = src[1] << 4;
    tmp_n = arq__hton16(frame_hdr->ack_seg_mask);                      /* ack_seg_mask */
    *dst++ = src[0] & 0x0F;
    *dst++ = src[1];
    NANOARQ_ASSERT((dst - (arq_uchar_t const *)out_buf) == ARQ_FRAME_HEADER_SIZE);
    return dst - (arq_uchar_t const *)out_buf;
}

int NANOARQ_MOCKABLE(arq__frame_seg_write)(void const *seg, void *out_buf, int len)
{
    arq_uchar_t const *src = (arq_uchar_t const *)(seg);
    arq_uchar_t *dst = (arq_uchar_t *)out_buf;
    NANOARQ_ASSERT(seg && out_buf && (len > 0));
    while (len--) {
        *dst++ = *src++;
    }
    return len;
}

int NANOARQ_MOCKABLE(arq__frame_len)(int seg_len)
{
    NANOARQ_ASSERT(seg_len <= 256);
    return ARQ_FRAME_COBS_OVERHEAD + ARQ_FRAME_HEADER_SIZE + seg_len + 4;
}

void NANOARQ_MOCKABLE(arq__frame_write)(arq__frame_hdr_t const *hdr,
                                        void const *seg,
                                        arq_checksum_cb_t checksum,
                                        void *out_frame,
                                        int frame_max)
{
    arq_uchar_t *dst = (arq_uchar_t *)out_frame + 1;
    int const frame_len = arq__frame_len(hdr->seg_len);
    NANOARQ_ASSERT(hdr && out_frame);
    NANOARQ_ASSERT((seg || (hdr->seg_len == 0)) && (frame_max >= frame_len));
    dst += arq__frame_hdr_write(hdr, dst);
    dst += arq__frame_seg_write(seg, dst, hdr->seg_len);
    arq__frame_checksum_write(checksum, dst, out_frame, frame_len);
    arq__frame_encode(out_frame, frame_len);
}

void NANOARQ_MOCKABLE(arq__frame_checksum_write)(arq_checksum_cb_t checksum,
                                                 void *checksum_seat,
                                                 void *frame,
                                                 int len)
{
    arq_uint32_t c;
    arq_uchar_t const *src = (arq_uchar_t const *)&c;
    arq_uchar_t *dst = (arq_uchar_t *)checksum_seat;
    NANOARQ_ASSERT(checksum && checksum_seat && frame && (len > 0));
    c = arq__hton32(checksum((arq_uchar_t const *)frame + 1, len - 1 - 4));
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
    *dst = *src;
}

void NANOARQ_MOCKABLE(arq__frame_encode)(void *frame, int len)
{
    arq__cobs_encode(frame, len);
}

arq__frame_read_result_t NANOARQ_MOCKABLE(arq__frame_read)(void *frame,
                                                           int frame_len,
                                                           arq_checksum_cb_t checksum,
                                                           arq__frame_hdr_t *out_hdr,
                                                           void const **out_seg)
{
    arq_uchar_t const *h = (arq_uchar_t const *)frame + 1;
    NANOARQ_ASSERT(frame && out_hdr && out_seg);
    arq__frame_decode(frame, frame_len);
    arq__frame_hdr_read(h, out_hdr);
    *out_seg = (void const *)(h + ARQ_FRAME_HEADER_SIZE);
    return arq__frame_checksum_read(frame, frame_len, out_hdr->seg_len, checksum);
}

arq__frame_read_result_t NANOARQ_MOCKABLE(arq__frame_checksum_read)(void const *frame,
                                                                    int frame_len,
                                                                    int seg_len,
                                                                    arq_checksum_cb_t checksum)
{
    arq_uint32_t frame_checksum, computed_checksum;
    arq_uchar_t *dst = (arq_uchar_t *)&frame_checksum;
    arq_uchar_t const *src;
    NANOARQ_ASSERT(frame && checksum);
    if (seg_len + ARQ_FRAME_COBS_OVERHEAD + ARQ_FRAME_HEADER_SIZE + 4 > frame_len) {
        return ARQ_FRAME_READ_RESULT_ERR_MALFORMED;
    }
    src = (arq_uchar_t const *)frame + 1 + ARQ_FRAME_HEADER_SIZE + seg_len;
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
    *dst = *src;
    computed_checksum = checksum((arq_uchar_t const *)frame + 1, ARQ_FRAME_HEADER_SIZE + seg_len);
    frame_checksum = arq__ntoh32(frame_checksum);
    if (frame_checksum != computed_checksum) {
        return ARQ_FRAME_READ_RESULT_ERR_CHECKSUM;
    }
    return ARQ_FRAME_READ_RESULT_SUCCESS;
}

void NANOARQ_MOCKABLE(arq__frame_decode)(void *frame, int len)
{
    arq__cobs_decode(frame, len);
}

void NANOARQ_MOCKABLE(arq__cobs_encode)(void *p, int len)
{
    arq_uchar_t *patch = (arq_uchar_t *)p;
    arq_uchar_t *c = (arq_uchar_t *)p + 1;
    arq_uchar_t const *e = (arq_uchar_t const *)p + (len - 1);
    NANOARQ_ASSERT(p && (len >= 3) && (len <= 256));
    while (c < e) {
        if (*c == 0) {
            *patch = (arq_uchar_t)(c - patch);
            patch = c;
        }
        ++c;
    }
    *patch = (arq_uchar_t)(c - patch);
    *c = 0;
}

void NANOARQ_MOCKABLE(arq__cobs_decode)(void *p, int len)
 {
    arq_uchar_t *c = (arq_uchar_t *)p;
    arq_uchar_t const *e = c + (len - 1);
    NANOARQ_ASSERT(p && (len >= 3) && (len <= 256));
    while (c < e) {
        arq_uchar_t *next = c + *c;
        NANOARQ_ASSERT(c != next);
        *c = 0;
        c = next;
    }
}

#if NANOARQ_COMPILE_CRC32 == 1
arq_uint32_t arq_crc32(void const *buf, int size)
{
    static const arq_uint32_t crc32_tab[] = {
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
        0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
        0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
        0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
        0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
        0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
        0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
        0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
        0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
        0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
        0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
        0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
        0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
        0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
        0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
        0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
        0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
        0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
        0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
        0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
        0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
        0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
        0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
        0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
        0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
        0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
        0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
        0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
        0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
        0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
        0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
        0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
        0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
        0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
        0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
        0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
        0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
        0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
        0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
        0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };

    arq_uchar_t const *p = (arq_uchar_t const *)buf;
    arq_uint32_t crc = ~0U;
    while (size--) {
        crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ ~0U;
}
#endif

#endif

