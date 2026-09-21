#include "tests/test_framework.h"
#include "dynstring.h"

int test_dynstring_create_and_free(void)
{
    dynstr ds = ds_create(32);
    ASSERT_NOT_NULL(ds);
    ASSERT_EQ(ds_len(ds), 0);
    ASSERT_STR_EQ(ds_data(ds), "");
    ds = ds_free(ds);
    ASSERT_NULL(ds);
    return 0;
}

int test_dynstring_dsprintf(void)
{
    dynstr ds = NULL;
    int ret = dsprintf(&ds, "Hello %s %d!", "Kindle", 3);
    ASSERT_TRUE(ret > 0);
    ASSERT_NOT_NULL(ds);
    ASSERT_STR_EQ(ds_data(ds), "Hello Kindle 3!");
    ASSERT_EQ(ds_len(ds), 15);

    // Append formatted text
    ret = dsprintf(&ds, " Next %d", 42);
    ASSERT_TRUE(ret > 0);
    ASSERT_STR_EQ(ds_data(ds), "Hello Kindle 3! Next 42");

    ds = ds_free(ds);
    ASSERT_NULL(ds);
    return 0;
}

int test_dynstring_append_and_growth(void)
{
    dynstr ds = NULL;
    const char *text = "0123456789";
    for (int i = 0; i < 100; i++) {
        int err = ds_append(&ds, text, 10);
        ASSERT_EQ(err, 0);
    }
    ASSERT_EQ(ds_len(ds), 1000);
    ASSERT_TRUE(ds_size(ds) >= 1000);

    ds = ds_free(ds);
    ASSERT_NULL(ds);
    return 0;
}

int test_dynstring_truncate_and_reset(void)
{
    dynstr ds = NULL;
    dsprintf(&ds, "Initial test string for truncation");
    ASSERT_EQ(ds_len(ds), 34);

    int err = ds_truncate(&ds, 7);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(ds_len(ds), 7);
    ASSERT_STR_EQ(ds_data(ds), "Initial");

    ds_reset(ds);
    ASSERT_EQ(ds_len(ds), 0);
    ASSERT_STR_EQ(ds_data(ds), "");

    ds = ds_free(ds);
    ASSERT_NULL(ds);
    return 0;
}

int test_dynstring_shift(void)
{
    dynstr ds = NULL;
    dsprintf(&ds, "PREFIX_CONTENT");
    int remaining = ds_shift(ds, 7);
    ASSERT_EQ(remaining, 7);
    ASSERT_STR_EQ(ds_data(ds), "CONTENT");
    ASSERT_EQ(ds_len(ds), 7);

    // Shift entire remainder
    remaining = ds_shift(ds, 7);
    ASSERT_EQ(remaining, 0);
    ASSERT_STR_EQ(ds_data(ds), "");

    // Shift beyond length returns -1
    remaining = ds_shift(ds, 1);
    ASSERT_EQ(remaining, -1);

    ds = ds_free(ds);
    ASSERT_NULL(ds);
    return 0;
}

int test_dynstring_ref(void)
{
    const char *static_str = "Read-only static string reference";
    dynstr ds = ds_ref(static_str, strlen(static_str));
    ASSERT_NOT_NULL(ds);
    ASSERT_EQ(ds_len(ds), (int)strlen(static_str));
    ASSERT_STR_EQ(ds_data(ds), static_str);

    // Shifting readonly string
    int rem = ds_shift(ds, 10);
    ASSERT_EQ(rem, (int)strlen(static_str) - 10);
    ASSERT_STR_EQ(ds_data(ds), "static string reference");

    ds = ds_free(ds);
    ASSERT_NULL(ds);
    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_dynstring_create_and_free);
    RUN_TEST(test_dynstring_dsprintf);
    RUN_TEST(test_dynstring_append_and_growth);
    RUN_TEST(test_dynstring_truncate_and_reset);
    RUN_TEST(test_dynstring_shift);
    RUN_TEST(test_dynstring_ref);
TEST_MAIN_END()
