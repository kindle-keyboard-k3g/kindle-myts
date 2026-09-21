#include "tests/test_framework.h"
#include "core/byte_buffer.hpp"
#include "core/string_builder.hpp"

using namespace myts::core;

int test_byte_buffer_basic_ops(void)
{
    ByteBuffer buf;
    ASSERT_EQ(buf.size(), 0);
    ASSERT_NULL(buf.data());

    const uint8_t sample[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    ASSERT_TRUE(buf.append(sample, sizeof(sample)));
    ASSERT_EQ(buf.size(), 5);
    ASSERT_NOT_NULL(buf.data());
    ASSERT_EQ(buf.data()[0], 0x01);
    ASSERT_EQ(buf.data()[4], 0x05);

    // Shift prefix
    ASSERT_TRUE(buf.shift_prefix(2));
    ASSERT_EQ(buf.size(), 3);
    ASSERT_EQ(buf.data()[0], 0x03);

    // Shift beyond size should fail
    ASSERT_FALSE(buf.shift_prefix(10));
    ASSERT_EQ(buf.size(), 3);

    buf.clear();
    ASSERT_EQ(buf.size(), 0);
    return 0;
}

int test_string_builder_basic_ops(void)
{
    StringBuilder sb;
    ASSERT_EQ(sb.size(), 0);
    ASSERT_STR_EQ(sb.c_str(), "");

    sb.append("Kindle ");
    sb.append("Terminal ");
    sb.append_decimal(3);
    sb.append_char('G');

    ASSERT_EQ(sb.size(), 18);
    ASSERT_STR_EQ(sb.c_str(), "Kindle Terminal 3G");
    ASSERT_TRUE(sb.view() == "Kindle Terminal 3G");

    sb.clear();
    ASSERT_EQ(sb.size(), 0);
    ASSERT_STR_EQ(sb.c_str(), "");
    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_byte_buffer_basic_ops);
    RUN_TEST(test_string_builder_basic_ops);
TEST_MAIN_END()
