#include "nanoarq_in_test_project.h"
#include <CppUTest/TestHarness.h>

#include <cstring>

TEST_GROUP(send_window_ptr) {};

namespace
{

TEST(send_window_ptr, init_sets_usr_and_valid_to_zero)
{
    arq__send_wnd_ptr_t p;
    p.valid = 1;
    arq__send_wnd_ptr_init(&p);
    CHECK_EQUAL(0, p.valid);
}

}

