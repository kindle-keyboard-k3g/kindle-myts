/**
 * @file metrics.hpp
 * @brief Performance and telemetry metrics tracking for kindle-myts.
 *
 * Tracks e-ink refresh cycles, dirty bounding box unions, PTY I/O throughput,
 * and user input event frequencies.
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <algorithm>
#include "graphics/geometry.hpp"
#include "core/string_builder.hpp"

namespace myts::core {

/**
 * @struct MetricsSnapshot
 * @brief Immutable point-in-time metrics counters.
 */
struct MetricsSnapshot {
    uint64_t partial_refreshes{0};
    uint64_t full_refreshes{0};
    uint64_t pty_bytes_read{0};
    uint64_t pty_bytes_written{0};
    uint64_t input_events{0};
    graphics::Rect dirty_bounding_box{0, 0, 0, 0};
};

/**
 * @class MetricsCollector
 * @brief Collects performance telemetry with zero heap allocations.
 */
class MetricsCollector {
public:
    constexpr MetricsCollector() noexcept = default;

    /**
     * @brief Record an E-Ink screen refresh event and update bounding box.
     * @param dirty_area The rectangle area refreshed.
     * @param full_flash True if full waveform flash, false if partial update.
     */
    void record_refresh(const graphics::Rect& dirty_area, bool full_flash) noexcept {
        if (full_flash) {
            full_refreshes_++;
        } else {
            partial_refreshes_++;
        }

        if (dirty_area.is_empty()) {
            return;
        }

        if (dirty_bounding_box_.is_empty()) {
            dirty_bounding_box_ = dirty_area;
            return;
        }

        int32_t min_x = std::min(dirty_bounding_box_.x, dirty_area.x);
        int32_t min_y = std::min(dirty_bounding_box_.y, dirty_area.y);
        int32_t max_x = std::max(dirty_bounding_box_.x + dirty_bounding_box_.width, dirty_area.x + dirty_area.width);
        int32_t max_y = std::max(dirty_bounding_box_.y + dirty_bounding_box_.height, dirty_area.y + dirty_area.height);

        dirty_bounding_box_ = graphics::Rect{
            min_x,
            min_y,
            max_x - min_x,
            max_y - min_y
        };
    }

    /**
     * @brief Record bytes received from the terminal PTY.
     */
    void record_pty_read(size_t bytes) noexcept {
        pty_bytes_read_ += bytes;
    }

    /**
     * @brief Record bytes sent to the terminal PTY.
     */
    void record_pty_write(size_t bytes) noexcept {
        pty_bytes_written_ += bytes;
    }

    /**
     * @brief Record a keyboard or 5-way controller event.
     */
    void record_input_event() noexcept {
        input_events_++;
    }

    /**
     * @brief Capture current snapshot of metrics.
     */
    [[nodiscard]] MetricsSnapshot snapshot() const noexcept {
        return MetricsSnapshot{
            partial_refreshes_,
            full_refreshes_,
            pty_bytes_read_,
            pty_bytes_written_,
            input_events_,
            dirty_bounding_box_
        };
    }

    /**
     * @brief Reset telemetry counters.
     */
    void reset() noexcept {
        partial_refreshes_ = 0;
        full_refreshes_ = 0;
        pty_bytes_read_ = 0;
        pty_bytes_written_ = 0;
        input_events_ = 0;
        dirty_bounding_box_ = graphics::Rect{0, 0, 0, 0};
    }

    /**
     * @brief Format a concise single-line summary of telemetry metrics.
     * @param out Destination StringBuilder.
     */
    void format_summary(StringBuilder& out) const {
        out.append("Refreshes: ");
        out.append_decimal(static_cast<int>(full_refreshes_));
        out.append(" full, ");
        out.append_decimal(static_cast<int>(partial_refreshes_));
        out.append(" partial | PTY: ");
        out.append_decimal(static_cast<int>(pty_bytes_read_));
        out.append(" in, ");
        out.append_decimal(static_cast<int>(pty_bytes_written_));
        out.append(" out | Inputs: ");
        out.append_decimal(static_cast<int>(input_events_));
        out.append(" | DirtyBox: ");
        out.append_decimal(dirty_bounding_box_.width);
        out.append("x");
        out.append_decimal(dirty_bounding_box_.height);
        out.append("@(");
        out.append_decimal(dirty_bounding_box_.x);
        out.append(",");
        out.append_decimal(dirty_bounding_box_.y);
        out.append(")");
    }

private:
    uint64_t partial_refreshes_{0};
    uint64_t full_refreshes_{0};
    uint64_t pty_bytes_read_{0};
    uint64_t pty_bytes_written_{0};
    uint64_t input_events_{0};
    graphics::Rect dirty_bounding_box_{0, 0, 0, 0};
};

} // namespace myts::core
