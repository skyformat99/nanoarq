#include "nanoarq_in_test_project.h"
#include "CppUTest/TestHarness.h"

TEST_GROUP(arq_assert) {};

namespace
{

TEST(arq_assert, set_and_get_assert)
{
    struct Local
    {
        static void a(char const *file, int line, char const *cond, char const *msg)
        {
            (void)file;
            (void)line;
            (void)cond;
            (void)msg;
        }
    };

    arq_assert_cb_t orig;
    CHECK_EQUAL(ARQ_OK_COMPLETED, arq_get_assert_handler(&orig));
    CHECK(orig != &Local::a);

    CHECK_EQUAL(ARQ_OK_COMPLETED, arq_set_assert_handler(&Local::a));
    arq_assert_cb_t actual;
    CHECK_EQUAL(ARQ_OK_COMPLETED, arq_get_assert_handler(&actual));
    CHECK_EQUAL(&Local::a, actual);

    CHECK_EQUAL(ARQ_OK_COMPLETED, arq_set_assert_handler(orig));
}

}

