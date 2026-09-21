#ifndef MYTS_CORE_EVENT_LOOP_HPP
#define MYTS_CORE_EVENT_LOOP_HPP

#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstdint>
#include <array>
#include <functional>
#include <chrono>

namespace myts {
namespace core {

/**
 * @brief Embedded, low-overhead event dispatcher built on top of POSIX select().
 *
 * Designed to manage file descriptor I/O notifications and timers with predictable memory
 * utilization on the Kindle 3 ARMv6 platform.
 */
class EventLoop {
public:
    static constexpr size_t MAX_WATCHERS = 16;
    static constexpr size_t MAX_TIMERS = 8;

    using IoCallback = std::function<void(int fd)>;
    using TimerCallback = std::function<void()>;

    /**
     * @brief Constructs a new EventLoop dispatcher.
     */
    EventLoop() noexcept
        : watcher_count_(0), timer_count_(0), next_timer_id_(1), running_(true) {}

    /**
     * @brief Registers a file descriptor for readable events.
     * @param fd POSIX file descriptor to monitor.
     * @param cb Callback invoked when data is ready to read.
     * @return true if successfully registered, false if maximum watchers exceeded.
     */
    bool register_read(int fd, IoCallback cb) {
        if (fd < 0 || watcher_count_ >= MAX_WATCHERS) {
            return false;
        }
        for (size_t i = 0; i < watcher_count_; ++i) {
            if (watchers_[i].fd == fd) {
                watchers_[i].cb = std::move(cb);
                return true;
            }
        }
        watchers_[watcher_count_] = FdWatcher{fd, std::move(cb)};
        ++watcher_count_;
        return true;
    }

    /**
     * @brief Unregisters a file descriptor from the event loop.
     * @param fd POSIX file descriptor to remove.
     */
    void unregister(int fd) noexcept {
        for (size_t i = 0; i < watcher_count_; ++i) {
            if (watchers_[i].fd == fd) {
                watchers_[i] = std::move(watchers_[watcher_count_ - 1]);
                --watcher_count_;
                return;
            }
        }
    }

    /**
     * @brief Schedules a one-shot timer callback.
     * @param delay_ms Milliseconds until expiration.
     * @param cb Callback invoked on timer expiry.
     * @return Unique timer ID, or 0 on failure.
     */
    uint32_t set_timer(uint32_t delay_ms, TimerCallback cb) {
        if (timer_count_ >= MAX_TIMERS) {
            return 0;
        }
        uint64_t now_ms = current_time_ms();
        uint32_t id = next_timer_id_++;
        if (next_timer_id_ == 0) {
            next_timer_id_ = 1;
        }
        timers_[timer_count_] = TimerEntry{id, now_ms + delay_ms, std::move(cb)};
        ++timer_count_;
        return id;
    }

    /**
     * @brief Cancels a previously scheduled timer.
     * @param timer_id Identifier returned by set_timer.
     */
    void cancel_timer(uint32_t timer_id) noexcept {
        for (size_t i = 0; i < timer_count_; ++i) {
            if (timers_[i].id == timer_id) {
                timers_[i] = std::move(timers_[timer_count_ - 1]);
                --timer_count_;
                return;
            }
        }
    }

    /**
     * @brief Dispatches a single step of the event loop.
     * @param timeout_ms Maximum millisecond duration to block in select().
     * @return true if an event or timer was processed or timeout reached safely.
     */
    bool step(int timeout_ms) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        int max_fd = -1;

        for (size_t i = 0; i < watcher_count_; ++i) {
            int fd = watchers_[i].fd;
            if (fd >= 0) {
                FD_SET(fd, &read_fds);
                if (fd > max_fd) {
                    max_fd = fd;
                }
            }
        }

        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        int res = select(max_fd + 1, &read_fds, nullptr, nullptr, (timeout_ms >= 0) ? &tv : nullptr);
        if (res > 0) {
            for (size_t i = 0; i < watcher_count_; ++i) {
                int fd = watchers_[i].fd;
                if (fd >= 0 && FD_ISSET(fd, &read_fds)) {
                    if (watchers_[i].cb) {
                        watchers_[i].cb(fd);
                    }
                }
            }
        }

        // Check timers
        uint64_t now_ms = current_time_ms();
        for (size_t i = 0; i < timer_count_; ) {
            if (now_ms >= timers_[i].expire_ms) {
                TimerCallback cb = std::move(timers_[i].cb);
                timers_[i] = std::move(timers_[timer_count_ - 1]);
                --timer_count_;
                if (cb) {
                    cb();
                }
            } else {
                ++i;
            }
        }

        return res >= 0;
    }

    /**
     * @brief Continuously runs the event loop until stopped.
     * @param poll_timeout_ms Max duration in ms per step iteration (default 50ms).
     */
    void run(int poll_timeout_ms = 50) {
        running_ = true;
        while (running_) {
            step(poll_timeout_ms);
        }
    }

    /**
     * @brief Signals the event loop to stop processing.
     */
    void stop() noexcept {
        running_ = false;
    }

    /**
     * @brief Checks if event loop is currently active.
     */
    [[nodiscard]] bool is_running() const noexcept {
        return running_;
    }

private:
    struct FdWatcher {
        int fd{-1};
        IoCallback cb;
    };

    struct TimerEntry {
        uint32_t id{0};
        uint64_t expire_ms{0};
        TimerCallback cb;
    };

    [[nodiscard]] static uint64_t current_time_ms() noexcept {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return static_cast<uint64_t>(tv.tv_sec) * 1000 + (tv.tv_usec / 1000);
    }

    std::array<FdWatcher, MAX_WATCHERS> watchers_{};
    size_t watcher_count_{0};

    std::array<TimerEntry, MAX_TIMERS> timers_{};
    size_t timer_count_{0};
    uint32_t next_timer_id_{1};

    bool running_{true};
};

} // namespace core
} // namespace myts

#endif // MYTS_CORE_EVENT_LOOP_HPP
