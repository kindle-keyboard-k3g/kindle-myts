/**
 * @file logger.hpp
 * @brief Zero-allocation diagnostic logging system for kindle-myts.
 *
 * Provides configurable log levels, high-resolution timestamps, subsystem tags,
 * and direct POSIX write semantics to avoid heap allocation overhead during logging.
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>
#include <unistd.h>
#include <fcntl.h>
#include <ctime>

namespace myts::core {

/**
 * @enum LogLevel
 * @brief Diagnostic severity levels.
 */
enum class LogLevel : uint8_t {
    Trace = 0,
    Debug = 1,
    Info  = 2,
    Warn  = 3,
    Error = 4,
    None  = 5
};

/**
 * @class Logger
 * @brief Low-overhead diagnostic logger supporting fast output to file descriptors.
 */
class Logger {
public:
    constexpr Logger() noexcept : min_level_(LogLevel::Info), output_fd_(STDERR_FILENO) {}

    /**
     * @brief Get singleton Logger instance.
     */
    static Logger& instance() noexcept {
        static Logger global_logger;
        return global_logger;
    }

    /**
     * @brief Set minimum log severity threshold.
     */
    void set_level(LogLevel level) noexcept {
        min_level_ = level;
    }

    /**
     * @brief Get current log level threshold.
     */
    [[nodiscard]] LogLevel level() const noexcept {
        return min_level_;
    }

    /**
     * @brief Configure output file descriptor.
     * @param fd POSIX file descriptor (e.g., STDERR_FILENO or file descriptor). Pass -1 to disable.
     */
    void set_output_fd(int fd) noexcept {
        output_fd_ = fd;
    }

    /**
     * @brief Log a message with subsystem tag and severity.
     *
     * Assembles timestamp, tag, and message directly into a stack buffer and writes
     * via ::write() with zero heap allocation.
     *
     * @param level Severity level.
     * @param tag Subsystem identifier (e.g., "PTY", "EINK").
     * @param message Log message body.
     */
    void log(LogLevel level, std::string_view tag, std::string_view message) noexcept {
        if (level < min_level_ || output_fd_ < 0) {
            return;
        }

        char buffer[1024];
        size_t offset = 0;

        // 1. Format timestamp: [HH:MM:SS]
        struct timespec ts{};
        clock_gettime(CLOCK_REALTIME, &ts);
        struct tm tm_buf{};
        localtime_r(&ts.tv_sec, &tm_buf);

        buffer[offset++] = '[';
        append_2digits(buffer, offset, tm_buf.tm_hour);
        buffer[offset++] = ':';
        append_2digits(buffer, offset, tm_buf.tm_min);
        buffer[offset++] = ':';
        append_2digits(buffer, offset, tm_buf.tm_sec);
        buffer[offset++] = ']';
        buffer[offset++] = ' ';

        // 2. Format Level tag
        const char* lvl_str = level_to_string(level);
        while (*lvl_str && offset < sizeof(buffer) - 64) {
            buffer[offset++] = *lvl_str++;
        }
        buffer[offset++] = ' ';

        // 3. Format Subsystem tag: [TAG]
        buffer[offset++] = '[';
        for (char c : tag) {
            if (offset >= sizeof(buffer) - 32) break;
            buffer[offset++] = c;
        }
        buffer[offset++] = ']';
        buffer[offset++] = ' ';

        // 4. Format Message
        for (char c : message) {
            if (offset >= sizeof(buffer) - 2) break;
            buffer[offset++] = c;
        }

        buffer[offset++] = '\n';

        // Write directly to file descriptor
        ssize_t written = 0;
        while (written < static_cast<ssize_t>(offset)) {
            ssize_t ret = ::write(output_fd_, buffer + written, offset - static_cast<size_t>(written));
            if (ret <= 0) {
                break;
            }
            written += ret;
        }
    }

private:
    static void append_2digits(char* buf, size_t& offset, int val) noexcept {
        buf[offset++] = static_cast<char>('0' + ((val / 10) % 10));
        buf[offset++] = static_cast<char>('0' + (val % 10));
    }

    static const char* level_to_string(LogLevel level) noexcept {
        switch (level) {
            case LogLevel::Trace: return "[TRACE]";
            case LogLevel::Debug: return "[DEBUG]";
            case LogLevel::Info:  return "[INFO]";
            case LogLevel::Warn:  return "[WARN]";
            case LogLevel::Error: return "[ERROR]";
            default:              return "[UNKNOWN]";
        }
    }

    LogLevel min_level_;
    int output_fd_;
};

} // namespace myts::core

#if !defined(NODEBUG)
#define MYTS_LOG_TRACE(tag, msg) ::myts::core::Logger::instance().log(::myts::core::LogLevel::Trace, tag, msg)
#define MYTS_LOG_DEBUG(tag, msg) ::myts::core::Logger::instance().log(::myts::core::LogLevel::Debug, tag, msg)
#define MYTS_LOG_INFO(tag, msg)  ::myts::core::Logger::instance().log(::myts::core::LogLevel::Info, tag, msg)
#define MYTS_LOG_WARN(tag, msg)  ::myts::core::Logger::instance().log(::myts::core::LogLevel::Warn, tag, msg)
#define MYTS_LOG_ERROR(tag, msg) ::myts::core::Logger::instance().log(::myts::core::LogLevel::Error, tag, msg)
#else
#define MYTS_LOG_TRACE(tag, msg) ((void)0)
#define MYTS_LOG_DEBUG(tag, msg) ((void)0)
#define MYTS_LOG_INFO(tag, msg)  ((void)0)
#define MYTS_LOG_WARN(tag, msg)  ((void)0)
#define MYTS_LOG_ERROR(tag, msg) ((void)0)
#endif
