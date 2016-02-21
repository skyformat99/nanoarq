#include "nanoarq_in_test_project.h"
#include <CppUTest/TestHarness.h>

TEST_GROUP(arq_assert) {};

namespace
{

#if ARQ_ASSERTS_ENABLED == 1
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
    arq_err_t ok = arq_assert_handler_get(&orig);
    CHECK_EQUAL(ARQ_OK_COMPLETED, ok);
    CHECK(orig != &Local::a);
    ok = arq_assert_handler_set(&Local::a);
    CHECK_EQUAL(ARQ_OK_COMPLETED, ok);
    arq_assert_cb_t actual;
    ok = arq_assert_handler_get(&actual);
    CHECK_EQUAL(ARQ_OK_COMPLETED, ok);
    CHECK_EQUAL((void *)&Local::a, (void *)actual);
    ok = arq_assert_handler_set(orig);
    CHECK_EQUAL(ARQ_OK_COMPLETED, ok);
}
#endif

}

