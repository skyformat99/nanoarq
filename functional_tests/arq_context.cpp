#include "functional_tests.h"

ArqContext::ArqContext(arq_cfg_t const &config)
{
    unsigned size;
    arq_err_t const e = arq_required_size(&config, &size);
    CHECK(ARQ_SUCCEEDED(e));

    seat = std::malloc(size);
    arq__lin_alloc_t la;
    arq__lin_alloc_init(&la, seat, size);

    arq = arq__alloc(&config, &la);
    arq->cfg = config;

    arq__wnd_init(&arq->send_wnd.w,
                  arq->cfg.send_window_size_in_messages,
                  arq->cfg.message_length_in_segments * arq->cfg.segment_length_in_bytes,
                  arq->cfg.segment_length_in_bytes);
    arq__send_frame_init(&arq->send_frame, arq__frame_len(arq->cfg.segment_length_in_bytes));
    arq__send_wnd_ptr_init(&arq->send_wnd_ptr);
    arq__wnd_init(&arq->recv_wnd.w,
                  arq->cfg.recv_window_size_in_messages,
                  arq->cfg.message_length_in_segments * arq->cfg.segment_length_in_bytes,
                  arq->cfg.segment_length_in_bytes);
    arq__recv_frame_init(&arq->recv_frame, arq__frame_len(arq->cfg.segment_length_in_bytes));
    arq__recv_wnd_ptr_init(&arq->recv_wnd_ptr);
    arq__recv_wnd_rst(&arq->recv_wnd);
}

ArqContext::~ArqContext()
{
    std::free(seat);
}

