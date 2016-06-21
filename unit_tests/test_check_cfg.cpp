#include "arq_in_unit_tests.h"
#include <CppUTest/TestHarness.h>

TEST_GROUP(check_cfg) {};

namespace {

struct Fixture
{
    Fixture()
    {
        cfg.message_length_in_segments = 1;
        cfg.segment_length_in_bytes = 1;
        cfg.send_window_size_in_messages = 1;
        cfg.recv_window_size_in_messages = 1;
        cfg.connection_rst_period = 100;
        cfg.connection_rst_attempts = 10;
    }
    arq_cfg_t cfg;
};

TEST(check_cfg, valid)
{
    Fixture f;
    CHECK(ARQ_SUCCEEDED(arq__check_cfg(&f.cfg)));
}

TEST(check_cfg, invalid_send_wnd_size)
{
    Fixture f;
    f.cfg.send_window_size_in_messages = 0;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq__check_cfg(&f.cfg));
}

TEST(check_cfg, invalid_recv_wnd_size)
{
    Fixture f;
    f.cfg.recv_window_size_in_messages = 0;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq__check_cfg(&f.cfg));
}

TEST(check_cfg, invalid_seg_len)
{
    Fixture f;
    f.cfg.segment_length_in_bytes = 0;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq__check_cfg(&f.cfg));
}

TEST(check_cfg, invlalid_wnd_len)
{
    Fixture f;
    f.cfg.message_length_in_segments = 0;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq__check_cfg(&f.cfg));
}

TEST(check_cfg, invalid_connection_attempts)
{
    Fixture f;
    f.cfg.connection_rst_attempts = 0;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq__check_cfg(&f.cfg));
    f.cfg.connection_rst_attempts = 1;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq__check_cfg(&f.cfg));
}

TEST(check_cfg, invalid_connection_period)
{
    Fixture f;
    f.cfg.connection_rst_period = 0;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq__check_cfg(&f.cfg));
}

}

