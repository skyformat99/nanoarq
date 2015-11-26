#ifndef NANOARQ_H_INCLUDED
#define NANOARQ_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    ARQ_OK_COMPLETED = 0,
    ARQ_OK_MORE,
    ARQ_ERR_INVALID_PARAM,
} arq_err_t;

typedef enum
{
    ARQ_STATE_NOT_CONNECTED,
    ARQ_STATE_SENT_RST,
    ARQ_STATE_SENT_RST_ACK,
    ARQ_STATE_CONNECTED
} arq_state_t;

typedef struct arq_t arq_t;
typedef unsigned int arq_time_t;
typedef void (*arq_assert_cb_t)(char const *file, int line, char const *cond, char const *msg);
typedef void (*arq_state_cb_t)(arq_t *arq, arq_state_t old_state, arq_state_t new_state);

typedef struct arq_cfg_t
{
    int send_frame_size;
    int send_window_count;
    int recv_frame_size;
    int recv_window_count;
    arq_time_t retransmission_timeout;
    arq_assert_cb_t assert_cb;
    arq_state_cb_t state_cb;
} arq_cfg_t;

// initialization / connection
arq_err_t arq_required_size(arq_cfg_t const *cfg, int *out_required_size);
arq_err_t arq_init(arq_t **out_arq, void *arq_seat, int arq_seat_size, arq_cfg_t const *cfg);
arq_err_t arq_connect(arq_t *arq);
arq_err_t arq_close(arq_t *arq);

// primary API. non-blocking, all calls return ARQ_COMPLETED or ARQ_MORE.
arq_err_t arq_send(arq_t *arq, void *send, int send_max, int *out_sent_size, arq_time_t now, arq_time_t *out_poll);
arq_err_t arq_recv(arq_t *arq, void *recv, int recv_max, int *out_recv_size, arq_time_t now, arq_time_t *out_poll);
arq_err_t arq_poll(arq_t *arq, unsigned int now, arq_time_t *out_poll);

// glue API for connecting to data source / sink (UART, pipe, etc)
arq_err_t arq_backend_drain_send(arq_t *arq, void *out_send, int send_max, int *out_send_size);
arq_err_t arq_backend_fill_recv(arq_t *arq, void *recv, int recv_max, int *out_recv_size);

typedef struct arq_t
{
    arq_cfg_t cfg;
    arq_state_t state;
} arq_t;

#ifdef __cplusplus
}
#endif
#endif

#ifdef NANOARQ_IMPLEMENTATION

#ifdef NANOARQ_IMPLEMENTATION_INCLUDED
    #error nanoarq.h already #included with NANOARQ_IMPLEMENTATION_INCLUDED #defined
#endif

#define NANOARQ_IMPLEMENTATION_INCLUDED

arq_err_t arq_required_size(arq_cfg_t const *cfg, int *out_required_size)
{
    (void)cfg;
    *out_required_size = 0;
    return ARQ_OK_COMPLETED;
}

#endif

