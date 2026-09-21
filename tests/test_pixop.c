#include "tests/test_framework.h"
#include "pixop.h"

int test_pixop_truncate_normal(void)
{
    int ofs = 50;
    int len = 100;
    int bound = 600;

    c_truncate(&ofs, &len, bound);
    ASSERT_EQ(ofs, 50);
    ASSERT_EQ(len, 100);
    return 0;
}

int test_pixop_truncate_negative_offset(void)
{
    int ofs = -20;
    int len = 100;
    int bound = 600;

    c_truncate(&ofs, &len, bound);
    ASSERT_EQ(ofs, 0);
    ASSERT_EQ(len, 80); // reduced by negative offset
    return 0;
}

int test_pixop_truncate_overflow_bound(void)
{
    int ofs = 550;
    int len = 100;
    int bound = 600;

    c_truncate(&ofs, &len, bound);
    ASSERT_EQ(ofs, 550);
    ASSERT_EQ(len, 50); // clipped at 600
    return 0;
}

int test_pixop_truncate_offset_beyond_bound(void)
{
    int ofs = 650;
    int len = 100;
    int bound = 600;

    c_truncate(&ofs, &len, bound);
    ASSERT_EQ(ofs, 600);
    ASSERT_EQ(len, 0);
    return 0;
}

int test_pixop_truncate_negative_offset_greater_than_len(void)
{
    int ofs = -150;
    int len = 100;
    int bound = 600;

    c_truncate(&ofs, &len, bound);
    ASSERT_EQ(ofs, 0);
    ASSERT_EQ(len, 0);
    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_pixop_truncate_normal);
    RUN_TEST(test_pixop_truncate_negative_offset);
    RUN_TEST(test_pixop_truncate_overflow_bound);
    RUN_TEST(test_pixop_truncate_offset_beyond_bound);
    RUN_TEST(test_pixop_truncate_negative_offset_greater_than_len);
TEST_MAIN_END()
