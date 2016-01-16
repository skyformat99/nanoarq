#ifndef NANOARQ_H_INCLUDED
#define NANOARQ_H_INCLUDED

#ifndef NANOARQ_UINT8_TYPE
#error You must define NANOARQ_UINT8_TYPE before including nanoarq.h
#endif

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

typedef enum
{
    ARQ_OK_COMPLETED = 0,
    ARQ_OK_MORE,
    ARQ_ERR_INVALID_PARAM,
    ARQ_ERR_SET_ASSERT_BEFORE_INIT
} arq_err_t;

typedef enum
{
    ARQ_EVENT_CONN_ESTABLISHED,
    ARQ_EVENT_CONN_CLOSED,
    ARQ_EVENT_CONN_RESET_BY_PEER,
    ARQ_EVENT_CONN_LOST_PEER_TIMEOUT
} arq_event_t;

typedef NANOARQ_UINT8_TYPE arq_uint8_t;
typedef NANOARQ_UINT16_TYPE arq_uint16_t;
typedef NANOARQ_UINT32_TYPE arq_uint32_t;
typedef NANOARQ_UINTPTR_TYPE arq_uintptr_t;

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

typedef struct arq_t
{
    arq_cfg_t cfg;
    arq_stats_t stats;
    arq_state_t state;
} arq_t;

arq_err_t arq_set_assert_handler(arq_assert_cb_t assert_cb);
arq_err_t arq_get_assert_handler(arq_assert_cb_t *out_assert_cb);

arq_err_t arq_required_size(arq_cfg_t const *cfg, int *out_required_size);
arq_err_t arq_init(arq_cfg_t const *cfg, void *arq_seat, int arq_seat_size, arq_t **out_arq);
arq_err_t arq_connect(arq_t *arq);
arq_err_t arq_reset(arq_t *arq);
arq_err_t arq_close(arq_t *arq);
arq_err_t arq_recv(arq_t *arq, void *recv, int recv_max, int *out_recv_size);
arq_err_t arq_send(arq_t *arq, void *send, int send_max, int *out_sent_size);
arq_err_t arq_flush_tinygrams(arq_t *arq);

arq_err_t arq_backend_poll(arq_t *arq,
                           arq_time_t dt,
                           int *out_drain_send_size,
                           int *out_recv_size,
                           arq_event_t *out_event,
                           arq_time_t *out_next_poll);

arq_err_t arq_backend_drain_send(arq_t *arq, void *out_send, int send_max, int *out_send_size);
arq_err_t arq_backend_fill_recv(arq_t *arq, void *recv, int recv_max, int *out_recv_size);

#if NANOARQ_COMPILE_CRC32 == 1
arq_uint32_t arq_util_crc32(arq_uint32_t crc, void const *buf, int size);
#endif

///////////// Internal API

typedef struct arq__lin_alloc_t
{
    char *base;
    char *top;
    int capacity;
} arq__lin_alloc_t;

void arq__lin_alloc_init(arq__lin_alloc_t *a, void *base, int capacity);
void *arq__lin_alloc_alloc(arq__lin_alloc_t *a, int size, int align);

typedef struct arq__frame_hdr_t
{
    int version;
    int seg_len;
    int win_size;
    int seq_num;
    int msg_len;
    int seg_id;
    int ack_num;
    unsigned int ack_seg_mask;
    char rst;
    char fin;
} arq__frame_hdr_t;

int arq__frame_hdr_read(void const *buf, arq__frame_hdr_t *out_frame_hdr);
int arq__frame_hdr_write(arq__frame_hdr_t const *frame_hdr, void *out_buf);

arq_uint16_t arq__hton16(arq_uint16_t x);
arq_uint16_t arq__ntoh16(arq_uint16_t x);
arq_uint32_t arq__hton32(arq_uint32_t x);
arq_uint32_t arq__ntoh32(arq_uint32_t x);

int arq__cobs_encode(void const *src, int src_size, void *dst, int dst_max);
int arq__cobs_decode(void const *src, int src_size, void *dst, int dst_max);

#if defined(__cplusplus) && (NANOARQ_COMPILE_AS_CPP == 0)
}
#endif
#endif

#ifdef NANOARQ_IMPLEMENTATION

#ifdef NANOARQ_IMPLEMENTATION_INCLUDED
#error nanoarq.h already #included with NANOARQ_IMPLEMENTATION_INCLUDED #defined
#endif

#define NANOARQ_IMPLEMENTATION_INCLUDED

#define ARQ_ASSERT(COND)          do { if (!(COND)) { s_assert_cb(__FILE__, __LINE__, #COND, ""); } } while (0)
#define ARQ_ASSERT_MSG(COND, MSG) do { if (!(COND)) { s_assert_cb(__FILE__, __LINE__, #COND, MSG); } } while (0)
#define ARQ_ASSERT_FAIL()         s_assert_cb(__FILE__, __LINE__, "", "explicit assert")

static arq_assert_cb_t s_assert_cb = NANOARQ_NULL_PTR;

arq_err_t arq_set_assert_handler(arq_assert_cb_t assert_cb)
{
    if (!assert_cb) {
        return ARQ_ERR_INVALID_PARAM;
    }
    s_assert_cb = assert_cb;
    return ARQ_OK_COMPLETED;
}

arq_err_t arq_get_assert_handler(arq_assert_cb_t *out_assert_cb)
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
    ARQ_ASSERT(a && base && (capacity > 0));
    a->base = (char *)base;
    a->top = (char *)base;
    a->capacity = capacity;
}

void *arq__lin_alloc_alloc(arq__lin_alloc_t *a, int size, int align)
{
    ARQ_ASSERT(a && (size > 0) && (align <= 32) && (align > 0) && ((align & (align - 1)) == 0));
    char *p = (char *)(((arq_uintptr_t)a->top + ((arq_uintptr_t)align - 1)) & ~((arq_uintptr_t)align - 1));
    char *new_top = p + size;
    int new_size = new_top - a->base;
    ARQ_ASSERT(new_size <= a->capacity);
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

int arq__frame_hdr_read(void const *buf, arq__frame_hdr_t *out_frame_hdr)
{
    ARQ_ASSERT(buf && out_frame_hdr);
    arq_uint8_t const *src = (arq_uint8_t const *)buf;
    out_frame_hdr->version = *src++;
    out_frame_hdr->seg_len = *src++;
    arq_uint8_t flags = *src++;
    out_frame_hdr->fin = !!(flags & (1 << 0));
    out_frame_hdr->rst = !!(flags & (1 << 1));
    out_frame_hdr->win_size = *src++;
    arq_uint16_t seq_num_h = *src >> 4;
    seq_num_h |= (*src++ << 4) << 8;
    seq_num_h |= (*src >> 4) << 8;
    out_frame_hdr->seq_num = arq__ntoh16(seq_num_h);
    arq_uint8_t msg_len = *src++ << 4;
    msg_len |= *src >> 4;
    out_frame_hdr->msg_len = msg_len;
    arq_uint16_t seg_id_h = *src++ & 0x0F;
    seg_id_h |= *src++ << 8;
    out_frame_hdr->seg_id = arq__ntoh16(seg_id_h);
    arq_uint16_t ack_num_h = *src >> 4;
    ack_num_h |= (*src++ << 4) << 8;
    ack_num_h |= (*src++ >> 4) << 8;
    out_frame_hdr->ack_num = arq__ntoh16(ack_num_h);
    arq_uint16_t ack_seg_mask_h = *src++ & 0x0F;
    ack_seg_mask_h |= *src++ << 8;
    out_frame_hdr->ack_seg_mask = arq__ntoh16(ack_seg_mask_h);
    return (int)(src - (arq_uint8_t const *)buf);
}

int arq__frame_hdr_write(arq__frame_hdr_t const *frame_hdr, void *out_buf)
{
    (void)frame_hdr;
    (void)out_buf;
    return 0;
}

#endif

