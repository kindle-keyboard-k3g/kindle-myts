#include "tests/test_framework.h"
#include "config/config.hpp"

using namespace myts::config;

int test_config_text_parsing(void)
{
    Config cfg;
    std::string_view ini_text =
        "[general]\n"
        "device = kindle3\n"
        "bpp = 4\n"
        "; inline comment\n"
        "# full line comment\n"
        "orientation = portrait\n"
        "\n"
        "[network]\n"
        "host = 10.250.50.165\n"
        "port = 22\n";

    ASSERT_TRUE(cfg.load_text(ini_text));

    // Case-insensitive section and key lookup
    auto dev = cfg.get("general", "device");
    ASSERT_TRUE(dev.has_value());
    ASSERT_TRUE(*dev == "kindle3");

    auto bpp = cfg.get("GENERAL", "BPP");
    ASSERT_TRUE(bpp.has_value());
    ASSERT_TRUE(*bpp == "4");

    auto host = cfg.get("network", "host");
    ASSERT_TRUE(host.has_value());
    ASSERT_TRUE(*host == "10.250.50.165");

    auto port = cfg.get_int("network", "port", 80);
    ASSERT_EQ(port, 22);

    // Non-existent entries
    auto missing = cfg.get("general", "nonexistent");
    ASSERT_FALSE(missing.has_value());

    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_config_text_parsing);
TEST_MAIN_END()
