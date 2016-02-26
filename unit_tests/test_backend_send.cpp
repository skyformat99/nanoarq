#include "nanoarq_in_test_project.h"
#include <CppUTest/TestHarness.h>

#include <cstring>

TEST_GROUP(backend_send) {};

namespace
{

TEST(backend_send, send_ptr_get_invalid_params)
{
    arq_t arq;
    void const *send;
    int size;
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_send_ptr_get(nullptr, &send, &size));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_send_ptr_get(&arq, nullptr, &size));
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_send_ptr_get(&arq, &send, nullptr));
}

struct Fixture
{
    Fixture()
    {
        arq__send_wnd_ptr_init(&arq.send_wnd_ptr);
        arq.send_frame.len = 1234;
        arq.send_frame.buf = (arq_uchar_t *)0x12345678;
    }

    arq_t arq;
    void const *send = nullptr;
    int size = 0;
};

TEST(backend_send, send_ptr_returns_success_when_nothing_to_send)
{
    Fixture f;
    arq_err_t const e = arq_backend_send_ptr_get(&f.arq, &f.send, &f.size);
    CHECK_EQUAL(ARQ_OK_COMPLETED, e);
}

TEST(backend_send, send_ptr_get_writes_zero_and_null_when_nothing_to_send)
{
    Fixture f;
    f.send = (void *)1;
    f.size = 1;
    arq_backend_send_ptr_get(&f.arq, &f.send, &f.size);
    CHECK_EQUAL((void const *)NULL, f.send);
    CHECK_EQUAL(0, f.size);
}

TEST(backend_send, send_ptr_get_sets_usr_to_zero_when_nothing_to_send)
{
    Fixture f;
    arq_backend_send_ptr_get(&f.arq, &f.send, &f.size);
    CHECK_EQUAL(0, f.arq.send_frame.usr);
}

TEST(backend_send, send_ptr_get_sets_usr_to_one_when_valid)
{
    Fixture f;
    f.arq.send_wnd_ptr.valid = 1;
    arq_backend_send_ptr_get(&f.arq, &f.send, &f.size);
    CHECK_EQUAL(1, f.arq.send_frame.usr);
}

TEST(backend_send, send_ptr_get_sets_buf_to_send_frame_when_valid)
{
    Fixture f;
    f.arq.send_wnd_ptr.valid = 1;
    arq_backend_send_ptr_get(&f.arq, &f.send, &f.size);
    CHECK_EQUAL((void const *)f.arq.send_frame.buf, f.send);
}

TEST(backend_send, send_ptr_get_sets_len_to_send_frame_len_when_valid)
{
    Fixture f;
    f.arq.send_wnd_ptr.valid = 1;
    arq_backend_send_ptr_get(&f.arq, &f.send, &f.size);
    CHECK_EQUAL(f.arq.send_frame.len, f.size);
}

TEST(backend_send, send_ptr_release_invalid_params)
{
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_backend_send_ptr_release(nullptr));
}

TEST(backend_send, send_ptr_release_sets_usr_to_zero)
{
    Fixture f;
    f.arq.send_frame.usr = 1;
    arq_err_t const e = arq_backend_send_ptr_release(&f.arq);
    CHECK_EQUAL(ARQ_OK_COMPLETED, e);
    CHECK_EQUAL(0, f.arq.send_frame.usr);
}

}

