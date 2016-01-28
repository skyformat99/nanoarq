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

#ifndef NANOARQ_COMPILE_AS_CPP
#error You must define NANOARQ_COMPILE_AS_CPP to 0 or 1 before including nanoarq.h
#endif

#if defined(__cplusplus) && (NANOARQ_COMPILE_AS_CPP == 0)
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
arq_uint32_t arq_util_crc32(arq_uint32_t crc, void const *buf, int size);
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
    NANOARQ_FRAME_HEADER_SIZE = 12,
    NANOARQ_FRAME_COBS_OVERHEAD = 2
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

int arq__frame_size(int max_seg_len);
void arq__frame_hdr_read(void const *buf, arq__frame_hdr_t *out_frame_hdr);
void arq__frame_hdr_write(arq__frame_hdr_t const *hdr, void *out_buf);
void arq__frame_read(void const *frame, arq__frame_hdr_t *out_hdr, void const **out_seg);
void arq__frame_write(arq__frame_hdr_t const *hdr, void const *seg, void *out_frame, int frame_max);
void arq__frame_encode(void *frame, int len);
void arq__frame_decode(void *frame, int len);

arq_uint16_t arq__hton16(arq_uint16_t x);
arq_uint16_t arq__ntoh16(arq_uint16_t x);
arq_uint32_t arq__hton32(arq_uint32_t x);
arq_uint32_t arq__ntoh32(arq_uint32_t x);

void arq__cobs_encode(void *p, int len);
void arq__cobs_decode(void *p, int len);

#if defined(__cplusplus) && (NANOARQ_COMPILE_AS_CPP == 0)
}
#endif
#endif

#ifdef NANOARQ_IMPLEMENTATION

#ifdef NANOARQ_IMPLEMENTATION_INCLUDED
#error nanoarq.h already #included with NANOARQ_IMPLEMENTATION_INCLUDED #defined
#endif

#define NANOARQ_IMPLEMENTATION_INCLUDED

#define NANOARQ_ASSERT(COND)          do { if (!(COND)) { s_assert_cb(__FILE__, __LINE__, #COND, ""); } } while (0)
#define NANOARQ_ASSERT_MSG(COND, MSG) do { if (!(COND)) { s_assert_cb(__FILE__, __LINE__, #COND, MSG); } } while (0)
#define NANOARQ_ASSERT_FAIL()         s_assert_cb(__FILE__, __LINE__, "", "explicit assert")

static arq_assert_cb_t s_assert_cb = NANOARQ_NULL_PTR;

arq_err_t arq_assert_handler_set(arq_assert_cb_t assert_cb)
{
    if (!assert_cb) {
        return ARQ_ERR_INVALID_PARAM;
    }
    s_assert_cb = assert_cb;
    return ARQ_OK_COMPLETED;
}

arq_err_t arq_assert_handler_get(arq_assert_cb_t *out_assert_cb)
{
    if (!out_assert_cb) {
        return ARQ_ERR_INVALID_PARAM;
    }
    *out_assert_cb = s_assert_cb;
    return ARQ_OK_COMPLETED;
}

arq_err_t arq_required_size(arq_cfg_t const *cfg, int *out_required_size)
{
    (void)cfg;
    *out_required_size = 0;
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

arq_uint32_t arq__hton32(arq_uint32_t x)
{
#if NANOARQ_LITTLE_ENDIAN_CPU == 1
    return (x >> 24) | ((x & 0x00FF0000) >> 8) | ((x & 0x0000FF00) << 8) | (x << 24);
#else
    return x;
#endif
}

arq_uint32_t arq__ntoh32(arq_uint32_t x)
{
    return arq__hton32(x);
}

void arq__frame_hdr_read(void const *buf, arq__frame_hdr_t *out_frame_hdr)
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
    NANOARQ_ASSERT((src - (arq_uchar_t const *)buf) == NANOARQ_FRAME_HEADER_SIZE);
}

void arq__frame_hdr_write(arq__frame_hdr_t const *frame_hdr, void *out_buf)
{
    arq_uchar_t *dst = (arq_uchar_t *)out_buf;
    arq_uint16_t tmp_n;
    arq_uchar_t const *src = (arq_uchar_t const *)&tmp_n;
    NANOARQ_ASSERT(frame_hdr && out_buf);
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
    tmp_n = arq__hton16(frame_hdr->ack_seg_mask);        /* ack_seg_mask */
    *dst++ = src[0] & 0x0F;
    *dst++ = src[1];
    NANOARQ_ASSERT((dst - (arq_uchar_t const *)out_buf) == NANOARQ_FRAME_HEADER_SIZE);
}

int arq__frame_size(int segment_size)
{
    NANOARQ_ASSERT(segment_size <= 256);
    return NANOARQ_FRAME_COBS_OVERHEAD + NANOARQ_FRAME_HEADER_SIZE + segment_size;
}

void arq__frame_write(arq__frame_hdr_t const *hdr, void const *seg, void *out_frame, int frame_max)
{
    int i;
    arq_uchar_t *dst = (arq_uchar_t *)out_frame + 1;
    arq_uchar_t const *src = (arq_uchar_t const *)seg;
    NANOARQ_ASSERT(hdr && out_frame);
    NANOARQ_ASSERT((seg || (hdr->seg_len == 0)) && (frame_max >= arq__frame_size(hdr->seg_len)));
    arq__frame_hdr_write(hdr, dst);
    dst += NANOARQ_FRAME_HEADER_SIZE;
    for (i = 0; i < hdr->seg_len; ++i) {
        *dst++ = *src++;
    }
}

void arq__frame_read(void const *frame, arq__frame_hdr_t *out_hdr, void const **out_seg)
{
    arq_uchar_t const *h = (arq_uchar_t const *)frame + 1;
    NANOARQ_ASSERT(frame && out_hdr && out_seg);
    arq__frame_hdr_read(h, out_hdr);
    *out_seg = (void const *)(h + NANOARQ_FRAME_HEADER_SIZE);
}

void arq__frame_encode(void *frame, int len)
{
    arq__cobs_encode(frame, len);
}

void arq__frame_decode(void *frame, int len)
{
    arq__cobs_decode(frame, len);
}

void arq__cobs_encode(void *p, int len)
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

void arq__cobs_decode(void *p, int len)
 {
    arq_uchar_t *c = (arq_uchar_t *)p;
    arq_uchar_t const *e = c + (len - 1);
    NANOARQ_ASSERT(p && (len >= 3) && (len <= 256));
    while (c < e) {
        arq_uchar_t *next = c + *c;
        *c = 0;
        c = next;
    }
}

#endif

