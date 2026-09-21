#ifndef MYTS_APP_APPLICATION_HPP
#define MYTS_APP_APPLICATION_HPP

#include "display/hardware_eink_driver.hpp"
#include "graphics/eink_display.hpp"
#include "graphics/font_renderer.hpp"
#include "terminal/terminal_session.hpp"
#include "input/input_manager.hpp"
#include "core/event_loop.hpp"

#include <linux/input.h>
#include <string_view>
#include <cstring>
#include <unistd.h>

namespace myts {
namespace app {

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
     * @param rows Character rows.
     * @param cols Character columns.
     */
    Application(const char* fb_path = "/dev/fb0",
                const char* font_path = "ter-u12n.hex",
                int rows = 24, int cols = 80)
        : driver_(fb_path),
          display_(driver_.width(), driver_.height(), driver_),
          font_(8, 12),
          session_(rows, cols),
          canvas_(driver_.width(), driver_.height()),
          font_path_(font_path) {}

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
        session_.feed_input(data);
    }

    /**
     * @brief Processes a Linux input_event and forwards produced characters to terminal PTY.
     * @param ev Input event.
     * @return Generated character sequence.
     */
    std::string_view handle_input_event(const struct input_event& ev) {
        std::string_view seq = input_.process_event(ev);
        if (!seq.empty() && session_.pty_fd() >= 0) {
            ssize_t written = ::write(session_.pty_fd(), seq.data(), seq.size());
            (void)written;
        }
        return seq;
    }

    /**
     * @brief Renders current terminal state to display surface and triggers refresh.
     */
    void render_frame() {
        session_.render(canvas_, font_, /*show_cursor=*/true);

        uint8_t* dst = driver_.surface_data();
        if (dst != nullptr && canvas_.data() != nullptr) {
            std::memcpy(dst, canvas_.data(), canvas_.size());
        }

        display_.mark_dirty(graphics::Rect(0, 0, driver_.width(), driver_.height()));
        display_.flush();
    }

    /**
     * @brief Spawns subshell and runs the main event loop.
     * @param shell Shell command to launch (default "/bin/sh").
     */
    void run(const char* shell = "/bin/sh") {
        if (!session_.spawn_pty(shell)) {
            return;
        }

        render_frame();

        // Register PTY master read callback
        loop_.register_read(session_.pty_fd(), [this](int fd) {
            char buf[1024];
            ssize_t bytes = ::read(fd, buf, sizeof(buf));
            if (bytes > 0) {
                session_.feed_input(std::string_view(buf, static_cast<size_t>(bytes)));
                render_frame();
            } else if (bytes <= 0) {
                loop_.stop();
            }
        });

        loop_.run();
    }

private:
    display::HardwareEinkDriver driver_;
    graphics::EinkDisplay display_;
    graphics::FontRenderer font_;
    terminal::TerminalSession session_;
    input::InputManager input_;
    core::EventLoop loop_;
    graphics::OwnedPixmap canvas_;
    const char* font_path_{"ter-u12n.hex"};
};

} // namespace app
} // namespace myts

#endif // MYTS_APP_APPLICATION_HPP
