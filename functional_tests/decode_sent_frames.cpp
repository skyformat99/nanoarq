#include "nanoarq_in_test_project.h"
#include <CppUTest/TestHarness.h>
#include <array>
#include <vector>

TEST_GROUP(functional) {};

namespace
{

TEST(functional, decode_sent_frames)
{
    arq_t arq;
    std::array< arq__msg_t, 64 > send_wnd_msgs;
    std::vector< unsigned char > send_frame(256);

    int const segment_length_in_bytes = 128;
    int const message_length_in_segments = 10;
    arq_time_t const retransmission_timeout = 100;

    std::vector< unsigned char > send_wnd_buf;
    send_wnd_buf.resize((send_wnd_msgs.size() * segment_length_in_bytes * message_length_in_segments));

    arq.send_wnd.msg = send_wnd_msgs.data();

    arq__send_wnd_init(&arq.send_wnd,
                       send_wnd_msgs.size(),
                       message_length_in_segments,
                       segment_length_in_bytes,
                       retransmission_timeout);
    arq__send_frame_init(&arq.send_frame, send_frame.size());
    arq__send_wnd_ptr_init(&arq.send_wnd_ptr);
}

}

