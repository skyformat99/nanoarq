#pragma once
#include "arq_in_functional_tests.h"
#include <vector>
#include <array>

struct ArqContext
{
    explicit ArqContext(arq_cfg_t const &config);
    ~ArqContext() = default;
    ArqContext() = delete;
    ArqContext(ArqContext const &) = delete;
    ArqContext &operator =(ArqContext const &) = delete;

    arq_t arq;
    std::array< arq__msg_t, 16 > send_wnd_msgs;
    std::array< arq_time_t, 16 > rtx_timers;
    std::array< unsigned char, 256 > send_frame;
    std::vector< unsigned char > send_wnd_buf;
    std::vector< unsigned char > recv_wnd_buf;
    std::array< unsigned char, 256 > recv_frame;
    std::array< arq__msg_t, 16 > recv_wnd_msgs;
    std::array< arq_uchar_t, 16 > recv_wnd_ack;
};

