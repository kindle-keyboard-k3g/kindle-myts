/**
 * @file test_logger.cpp
 * @brief Unit tests for zero-allocation diagnostic logger (core/logger.hpp)
 */

#include "test_framework.h"
#include "core/logger.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <string_view>

using namespace myts::core;

static int test_logger_level_filtering() {
    Logger logger;
    logger.set_level(LogLevel::Warn);

    // Create a pipe to capture logger output
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);

    logger.set_output_fd(pipefd[1]);

    // Debug and Info should be filtered out
    logger.log(LogLevel::Debug, "TEST", "This debug message should be dropped");
    logger.log(LogLevel::Info, "TEST", "This info message should be dropped");

    // Warn and Error should pass through
    logger.log(LogLevel::Warn, "TEST", "Warning triggered");
    logger.log(LogLevel::Error, "TEST", "Error triggered");

    // Close write end to read till EOF
    logger.set_output_fd(-1);
    close(pipefd[1]);

    char buf[512] = {0};
    ssize_t n = read(pipefd[0], buf, sizeof(buf) - 1);
    close(pipefd[0]);

    ASSERT_TRUE(n > 0);
    std::string_view out(buf, static_cast<size_t>(n));

    // Confirm dropped messages are absent
    ASSERT_TRUE(out.find("This debug message should be dropped") == std::string_view::npos);
    ASSERT_TRUE(out.find("This info message should be dropped") == std::string_view::npos);

    // Confirm active messages are present
    ASSERT_TRUE(out.find("Warning triggered") != std::string_view::npos);
    ASSERT_TRUE(out.find("Error triggered") != std::string_view::npos);
    ASSERT_TRUE(out.find("[WARN]") != std::string_view::npos);
    ASSERT_TRUE(out.find("[ERROR]") != std::string_view::npos);
    ASSERT_TRUE(out.find("[TEST]") != std::string_view::npos);
    return 0;
}

static int test_logger_formatting_and_timestamp() {
    Logger logger;
    logger.set_level(LogLevel::Trace);

    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);
    logger.set_output_fd(pipefd[1]);

    logger.log(LogLevel::Trace, "SUB", "Trace details 123");

    logger.set_output_fd(-1);
    close(pipefd[1]);

    char buf[256] = {0};
    ssize_t n = read(pipefd[0], buf, sizeof(buf) - 1);
    close(pipefd[0]);

    ASSERT_TRUE(n > 0);
    std::string_view out(buf, static_cast<size_t>(n));

    // Format should contain: [TIMESTAMP] [TRACE] [SUB] Trace details 123\n
    ASSERT_TRUE(out.find("[TRACE]") != std::string_view::npos);
    ASSERT_TRUE(out.find("[SUB]") != std::string_view::npos);
    ASSERT_TRUE(out.find("Trace details 123\n") != std::string_view::npos);
    return 0;
}

static int test_logger_global_instance_and_macros() {
    auto& logger = Logger::instance();
    logger.set_level(LogLevel::Debug);

    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);
    logger.set_output_fd(pipefd[1]);

    MYTS_LOG_DEBUG("EINK", "Refreshing eink panel");

    logger.set_output_fd(-1);
    close(pipefd[1]);

    char buf[256] = {0};
    ssize_t n = read(pipefd[0], buf, sizeof(buf) - 1);
    close(pipefd[0]);

#if defined(NODEBUG)
    // In NODEBUG mode, MYTS_LOG_DEBUG should be eliminated
    ASSERT_EQ(n, 0);
#else
    ASSERT_TRUE(n > 0);
    std::string_view out(buf, static_cast<size_t>(n));
    ASSERT_TRUE(out.find("[EINK]") != std::string_view::npos);
    ASSERT_TRUE(out.find("Refreshing eink panel") != std::string_view::npos);
#endif
    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_logger_level_filtering);
    RUN_TEST(test_logger_formatting_and_timestamp);
    RUN_TEST(test_logger_global_instance_and_macros);
TEST_MAIN_END()
