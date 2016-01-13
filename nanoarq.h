#ifndef NANOARQ_H_INCLUDED
#define NANOARQ_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NANOARQ_UINT32_BASE_TYPE
#error You must define NANOARQ_UINT32_BASE_TYPE before including nanoarq.h
#endif

#ifndef NANOARQ_NULL_PTR
#error You must define NANOARQ_NULL_PTR before including nanoarq.h
#endif

#ifndef NANOARQ_COMPILE_CRC32
#error You must define NANOARQ_COMPILE_CRC32 to 0 or 1 before including nanoarq.h
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

struct arq_t;
typedef NANOARQ_UINT32_BASE_TYPE arq_uint32_t;
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
    void *user;
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

// update API. arq_send doesn't push any bytes down to be drained, and doesn't update any timers.
// call arq_poll after some number of arq_send calls, and after 'out_next_poll' time units.
arq_err_t arq_backend_poll(arq_t *arq,
                           arq_time_t dt,
                           int *out_drain_send_size,
                           int *out_recv_size,
                           arq_event_t *out_event,
                           arq_time_t *out_next_poll);

// glue API for connecting to data source / sink (UART, pipe, etc)
arq_err_t arq_backend_drain_send(arq_t *arq, void *out_send, int send_max, int *out_send_size);
arq_err_t arq_backend_fill_recv(arq_t *arq, void *recv, int recv_max, int *out_recv_size);

#if NANOARQ_COMPILE_CRC32 == 1
arq_uint32_t arq_crc32(arq_uint32_t crc, void const *buf, int size);
#endif

///////////// Internal API

int arq__cobs_encode(void const *src, int src_size, void *dst, int dst_max);
int arq__cobs_decode(void const *src, int src_size, void *dst, int dst_max);

#ifdef __cplusplus
}
#endif
#endif

#ifdef NANOARQ_IMPLEMENTATION

#ifdef NANOARQ_IMPLEMENTATION_INCLUDED
    #error nanoarq.h already #included with NANOARQ_IMPLEMENTATION_INCLUDED #defined
#endif

#define NANOARQ_IMPLEMENTATION_INCLUDED

static arq_assert_cb_t s_assert_cb = NANOARQ_NULL_PTR;

#define ARQ_ASSERT(COND) do { \
        if (!(COND)) { \
            s_assert_cb(__FILE__, __LINE__, #COND, ""); \
        } \
    } while (0)

#define ARQ_ASSERT_MSG(COND, MSG) do { \
        if (!(COND)) { \
            s_assert_cb(__FILE__, __LINE__, #COND, MSG); \
        } \
    } while (0)

#define ARQ_ASSERT_FAIL() s_assert_cb(__FILE__, __LINE__, "", "explicit assert")

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

#endif

