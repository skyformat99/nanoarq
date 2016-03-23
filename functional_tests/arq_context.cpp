#include "arq_context.h"

ArqContext::ArqContext(arq_cfg_t const &config)
{
    arq.cfg = config;

    send_wnd_buf.resize(send_wnd_msgs.size() *
                        arq.cfg.segment_length_in_bytes *
                        arq.cfg.message_length_in_segments);

    recv_wnd_buf.resize(recv_wnd_msgs.size() *
                        arq.cfg.segment_length_in_bytes *
                        arq.cfg.message_length_in_segments);

    arq.send_wnd.w.msg = send_wnd_msgs.data();
    arq.send_wnd.w.buf = send_wnd_buf.data();
    arq.send_frame.buf = send_frame.data();
    arq.send_wnd.rtx = rtx_timers.data();

    arq.recv_wnd.ack = recv_wnd_ack.data();
    arq.recv_wnd.w.msg = recv_wnd_msgs.data();
    arq.recv_wnd.w.buf = recv_wnd_buf.data();

    arq__wnd_init(&arq.send_wnd.w,
                  send_wnd_msgs.size(),
                  arq.cfg.message_length_in_segments * arq.cfg.segment_length_in_bytes,
                  arq.cfg.segment_length_in_bytes);
    arq__send_frame_init(&arq.send_frame, send_frame.size());
    arq__send_wnd_ptr_init(&arq.send_wnd_ptr);
    arq__wnd_init(&arq.recv_wnd.w,
                  recv_wnd_msgs.size(),
                  arq.cfg.message_length_in_segments * arq.cfg.segment_length_in_bytes,
                  arq.cfg.segment_length_in_bytes);
    arq__recv_frame_init(&arq.recv_frame, recv_frame.data(), recv_frame.size());
    arq__recv_wnd_ptr_init(&arq.recv_wnd_ptr);
    arq__recv_wnd_rst(&arq.recv_wnd);
}

