#include "tests/test_framework.h"
#include "config.h"

int verbose = 0;

int test_config_immediate_parsing(void)
{
    const char *ini_data =
        "\n"
        "[general]\n"
        "device = kindle3\n"
        "bpp = 4\n"
        "; this is a comment\n"
        "# another comment\n"
        "orientation = portrait\n"
        "\n"
        "[screen]\n"
        "width = 600\n"
        "height = 800\n";

    struct config *cfg = cfg_read(ini_data, ".", NULL);
    ASSERT_NOT_NULL(cfg);

    const char *dev = cfg_find_val(cfg, "general", "device");
    ASSERT_NOT_NULL(dev);
    ASSERT_STR_EQ(dev, "kindle3");

    const char *bpp = cfg_find_val(cfg, "general", "bpp");
    ASSERT_NOT_NULL(bpp);
    ASSERT_STR_EQ(bpp, "4");

    const char *w = cfg_find_val(cfg, "screen", "width");
    ASSERT_NOT_NULL(w);
    ASSERT_STR_EQ(w, "600");

    const char *h = cfg_find_val(cfg, "screen", "height");
    ASSERT_NOT_NULL(h);
    ASSERT_STR_EQ(h, "800");

    // Missing key returns NULL
    const char *missing = cfg_find_val(cfg, "screen", "nonexistent");
    ASSERT_NULL(missing);

    // Missing section returns NULL
    missing = cfg_find_val(cfg, "nonexistent", "width");
    ASSERT_NULL(missing);

    cfg_free(cfg);
    return 0;
}

int test_config_whitespace_and_comments(void)
{
    const char *ini_data =
        "\n"
        "[network]   \n"
        "   host   =   10.250.50.165   ; server ip\n"
        "# full line comment\n"
        "port = 22 ; ssh port\n"
        "quoted = \"value with ; inside\"\n";

    struct config *cfg = cfg_read(ini_data, ".", NULL);
    ASSERT_NOT_NULL(cfg);

    const char *host = cfg_find_val(cfg, "network", "host");
    ASSERT_NOT_NULL(host);
    ASSERT_STR_EQ(host, "10.250.50.165");

    const char *port = cfg_find_val(cfg, "network", "port");
    ASSERT_NOT_NULL(port);
    ASSERT_STR_EQ(port, "22");

    const char *quoted = cfg_find_val(cfg, "network", "quoted");
    ASSERT_NOT_NULL(quoted);
    ASSERT_STR_EQ(quoted, "\"value with ; inside\"");

    cfg_free(cfg);
    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_config_immediate_parsing);
    RUN_TEST(test_config_whitespace_and_comments);
TEST_MAIN_END()
