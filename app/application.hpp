/**
 * @file application.hpp
 * @brief Top-level application coordinator linking EventLoop, TerminalSession,
 * InputManager, EinkDisplay, and Diagnostics Subsystem.
 */

#ifndef MYTS_APP_APPLICATION_HPP
#define MYTS_APP_APPLICATION_HPP

#include "display/hardware_eink_driver.hpp"
#include "graphics/eink_display.hpp"
#include "graphics/font_renderer.hpp"
#include "graphics/debug_overlay.hpp"
#include "help/help_controller.hpp"
#include "help/help_renderer.hpp"
#include "terminal/terminal_session.hpp"
#include "input/input_manager.hpp"
#include "core/event_loop.hpp"
#include "core/logger.hpp"
#include "core/metrics.hpp"

#include <linux/input.h>
#include <string_view>
#include <cstring>
#include <algorithm>
#include <vector>
#include <unistd.h>

namespace myts {
namespace app {

/**
 * @struct DebugConfig
 * @brief Configuration parameters for runtime diagnostics and telemetry.
 */
struct DebugConfig {
    bool enable_debug{false};
    bool enable_overlay{false};
    bool enable_metrics{false};
    core::LogLevel level{core::LogLevel::Info};
    const char* log_file{nullptr};
};

/**
 * @brief Top-level application coordinator linking EventLoop, TerminalSession, InputManager, and EinkDisplay.
 *
 * Replaces legacy procedural C entry point (myts.c) with clean RAII lifecycle management.
 */
class Application {
public:
    /**
     * @brief Constructs application instance with configured hardware parameters.
     * @param fb_path Framebuffer device path.
     * @param font_path Path to hex font file.
     * @param rows Character rows (0 = auto-calculate to fill full screen).
     * @param cols Character columns (0 = auto-calculate to fill full screen).
     */
    Application(const char* fb_path = "/dev/fb0",
                const char* font_path = "ter-u12n.hex",
                int rows = 0, int cols = 0)
        : driver_(fb_path),
          display_(driver_.width(), driver_.height(), driver_),
          font_(8, 12),
          session_(resolve_rows(rows, driver_.height(), 12),
                   resolve_cols(cols, driver_.width(), 8)),
          canvas_(driver_.width(), driver_.height()),
          font_path_(font_path) {}

    /**
     * @brief Configures runtime diagnostic settings and logging sinks.
     */
    void configure_debug(const DebugConfig& cfg) {
        debug_cfg_ = cfg;
        core::Logger::instance().set_level(cfg.level);

        if (cfg.log_file != nullptr && cfg.log_file[0] != '\0') {
            int fd = ::open(cfg.log_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd >= 0) {
                core::Logger::instance().set_output_fd(fd);
            }
        }

        MYTS_LOG_INFO("APP", "Debug diagnostics initialized");
    }

    /**
     * @brief Checks if debug mode is active.
     */
    [[nodiscard]] bool is_debug_enabled() const noexcept {
        return debug_cfg_.enable_debug;
    }

    /**
     * @brief Accesses metrics collector.
     */
    [[nodiscard]] core::MetricsCollector& metrics() noexcept {
        return metrics_;
    }

    /**
     * @brief Accesses metrics collector (const).
     */
    [[nodiscard]] const core::MetricsCollector& metrics() const noexcept {
        return metrics_;
    }

    /**
     * @brief Initializes font, canvas surface, and session subshell.
     * @return true on successful initialization.
     */
    bool init() {
        bool font_ok = font_.load_hex_file(font_path_);
        if (!font_ok) {
            // Fallback font data for space (0x20) and glyphs if file not found
            const char* fallback =
                "0020:000000000000000000000000\n"
                "0041:0000003c66667e6666000000\n";
            font_.load_hex_data(fallback);
        }

        canvas_.clear(0xFF);
        return true;
    }

    /**
     * @brief Accesses terminal session.
     */
    [[nodiscard]] terminal::TerminalSession& session() noexcept {
        return session_;
    }

    /**
     * @brief Accesses underlying hardware driver.
     */
    [[nodiscard]] display::HardwareEinkDriver& driver() noexcept {
        return driver_;
    }

    /**
     * @brief Feeds external stream slice into terminal emulator.
     */
    void feed_terminal_output(std::string_view data) {
        metrics_.record_pty_read(data.size());
        session_.feed_input(data);
    }

    /**
     * @brief Checks if help screen is currently displayed.
     */
    [[nodiscard]] bool is_help_active() const noexcept {
        return help_.active();
    }

    /**
     * @brief Accesses help controller.
     */
    [[nodiscard]] const help::HelpController& help_controller() const noexcept {
        return help_;
    }

    /**
     * @brief Processes a Linux input_event and forwards produced characters to terminal PTY.
     * @param ev Input event.
     * @return Generated character sequence.
     */
    std::string_view handle_input_event(const struct input_event& ev) {
        metrics_.record_input_event();
        std::string_view seq = input_.process_event(ev);
        help::HelpRoute route = help_.before_terminal_write(ev, input_.modifiers(), seq);

        if (route == help::HelpRoute::Exit) {
            restore_terminal_display();
            return {};
        }
        if (route == help::HelpRoute::RedrawFull) {
            render_help(/*full=*/true);
            return {};
        }
        if (route == help::HelpRoute::RedrawDelta) {
            render_help(/*full=*/false);
            return {};
        }
        if (route == help::HelpRoute::Consume) {
            return {};
        }

        if (!seq.empty() && session_.pty_fd() >= 0) {
            ssize_t written = ::write(session_.pty_fd(), seq.data(), seq.size());
            if (written > 0) {
                metrics_.record_pty_write(static_cast<size_t>(written));
            }
        }

        if (!seq.empty()) {
            help::HelpRoute after = help_.after_terminal_write(seq);
            if (after == help::HelpRoute::RedrawFull) {
                render_help(/*full=*/true);
            }
        }

        return seq;
    }

    /**
     * @brief Renders current terminal state to display surface and triggers refresh.
     *
     * Computes a minimal dirty rectangle from changed rows so only the affected
     * screen strip is sent to the e-ink controller, eliminating per-keystroke
     * full GC16 flashes.
     */
    void render_frame() {
        auto dirty_rows = session_.take_dirty_rows();
        graphics::Rect dirty = compute_dirty_rect(dirty_rows);
        if (dirty.is_empty()) { return; }

        session_.render(canvas_, font_, /*show_cursor=*/true, &dirty_rows);

        if (debug_cfg_.enable_overlay) {
            overlay_.render(canvas_, font_, metrics_.snapshot(), 0, 0);
            int oh = font_.glyph_height() * 2;
            graphics::Rect odirty{0, 0, driver_.width(), oh};
            int ux1 = std::min(dirty.x, odirty.x);
            int uy1 = std::min(dirty.y, odirty.y);
            int ux2 = std::max(dirty.x + dirty.width, odirty.x + odirty.width);
            int uy2 = std::max(dirty.y + dirty.height, odirty.y + odirty.height);
            dirty = graphics::Rect{ux1, uy1, ux2 - ux1, uy2 - uy1};
        }

        copy_canvas_to_driver();

        display_.mark_dirty(dirty);
        bool was_full = (display_.partial_updates_count() + 1 >= display_.refresh_config().partial_limit);
        metrics_.record_refresh(dirty, was_full);
        display_.flush();
    }

    /**
     * @brief Spawns subshell and runs the main event loop.
     * @param shell Shell command to launch (default "/bin/sh").
     */
    void run(const char* shell = "/bin/sh") {
        if (!session_.spawn_pty(shell)) {
            MYTS_LOG_ERROR("APP", "Failed to spawn PTY shell");
            return;
        }

        // Wipe full screen on initial startup to clear prior UI / book content
        copy_canvas_to_driver();
        display_.refresh_full();

        render_frame();

        // Register PTY master read callback
        loop_.register_read(session_.pty_fd(), [this](int fd) {
            char buf[1024];
            ssize_t bytes = ::read(fd, buf, sizeof(buf));
            if (bytes > 0) {
                feed_terminal_output(std::string_view(buf, static_cast<size_t>(bytes)));
                if (!help_.active()) {
                    render_frame();
                }
            } else if (bytes <= 0) {
                MYTS_LOG_INFO("PTY", "PTY closed or EOF reached");
                loop_.stop();
            }
        });

        // Register Linux input event handlers (/dev/input/event0, event1, event2)
        const char* const input_devs[] = {
            "/dev/input/event0", // Keyboard
            "/dev/input/event1", // Five-way controller
            "/dev/input/event2"  // Volume keys
        };

        for (const char* dev_path : input_devs) {
            int input_fd = ::open(dev_path, O_RDONLY | O_NONBLOCK);
            if (input_fd >= 0) {
                MYTS_LOG_INFO("INPUT", dev_path);
                loop_.register_read(input_fd, [this](int fd) {
                    struct input_event ev{};
                    while (::read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
                        handle_input_event(ev);
                    }
                });
            }
        }

        loop_.run();
    }

private:
    static int resolve_rows(int requested, int screen_height, int glyph_height) noexcept {
        if (requested > 0) { return requested; }
        if (screen_height > 0 && glyph_height > 0) {
            return screen_height / glyph_height;
        }
        return 24;
    }

    static int resolve_cols(int requested, int screen_width, int glyph_width) noexcept {
        if (requested > 0) { return requested; }
        if (screen_width > 0 && glyph_width > 0) {
            return screen_width / glyph_width;
        }
        return 80;
    }

    [[nodiscard]] graphics::Rect compute_dirty_rect(const std::vector<bool>& dirty_rows) const noexcept {
        int gh = font_.glyph_height();
        int first = -1;
        int last = -1;
        for (int r = 0; r < static_cast<int>(dirty_rows.size()); ++r) {
            if (!dirty_rows[static_cast<size_t>(r)]) { continue; }
            if (first < 0) { first = r; }
            last = r;
        }
        if (first < 0) { return graphics::Rect{0, 0, 0, 0}; }
        return graphics::Rect{0, first * gh, driver_.width(), (last - first + 1) * gh};
    }

    void render_help(bool full = true) {
        graphics::Rect dirty = full
            ? help_renderer_.render_full(canvas_, font_, help_.screen().state())
            : help_renderer_.render_delta(canvas_, font_, help_.screen().state());
        copy_canvas_to_driver();
        display_.mark_dirty(dirty);
        display_.flush();
    }

    void restore_terminal_display() {
        canvas_.clear(0xFF);
        session_.mark_all_dirty();
        session_.render(canvas_, font_, /*show_cursor=*/true, nullptr);
        if (debug_cfg_.enable_overlay) {
            overlay_.render(canvas_, font_, metrics_.snapshot(), 0, 0);
        }
        copy_canvas_to_driver();
        display_.refresh_full();
    }

    void copy_canvas_to_driver() noexcept {
        uint8_t* dst = driver_.surface_data();
        if (dst != nullptr && canvas_.data() != nullptr) {
            std::memcpy(dst, canvas_.data(), canvas_.size());
        }
    }

    display::HardwareEinkDriver driver_;
    graphics::EinkDisplay display_;
    graphics::FontRenderer font_;
    terminal::TerminalSession session_;
    input::InputManager input_;
    core::EventLoop loop_;
    graphics::OwnedPixmap canvas_;
    const char* font_path_{"ter-u12n.hex"};
    DebugConfig debug_cfg_{};
    core::MetricsCollector metrics_{};
    graphics::DebugOverlay overlay_{};
    help::HelpController help_{};
    help::HelpRenderer help_renderer_{};
};

} // namespace app
} // namespace myts

#endif // MYTS_APP_APPLICATION_HPP
