#ifndef MYTS_TERMINAL_TERMINAL_SESSION_HPP
#define MYTS_TERMINAL_TERMINAL_SESSION_HPP

#include "terminal/ansi_parser.hpp"
#include "graphics/font_renderer.hpp"
#include "graphics/pixmap.hpp"
#include "core/raii.hpp"

#include <vector>
#include <string_view>
#include <cstdint>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>

namespace myts {
namespace terminal {

/**
 * @brief Represents a single character cell inside the terminal grid.
 */
struct Cell {
    char c{' '};
    uint8_t attr{0};
};

/**
 * @brief Terminal session managing 2D character buffer, ANSI decoding, and PTY I/O.
 *
 * Implements IAnsiHandler to receive parsed escape sequences.
 * Supports scrolling, cursor navigation, PTY subshell execution, and E-Ink surface rendering.
 */
class TerminalSession : public IAnsiHandler {
public:
    /**
     * @brief Constructs a terminal session with specified row and column dimensions.
     * @param rows Vertical character count (default 24).
     * @param cols Horizontal character count (default 80).
     */
    TerminalSession(int rows = 24, int cols = 80)
        : rows_(rows > 0 ? rows : 24),
          cols_(cols > 0 ? cols : 80),
          cursor_row_(0),
          cursor_col_(0),
          cells_(static_cast<size_t>(rows_ * cols_), Cell{' ', 0}),
          parser_(*this) {}

    ~TerminalSession() override = default;

    // Non-copyable, movable
    TerminalSession(const TerminalSession&) = delete;
    TerminalSession& operator=(const TerminalSession&) = delete;
    TerminalSession(TerminalSession&&) noexcept = default;
    TerminalSession& operator=(TerminalSession&&) noexcept = default;

    /**
     * @brief Total number of rows in the terminal grid.
     */
    [[nodiscard]] int rows() const noexcept { return rows_; }

    /**
     * @brief Total number of columns in the terminal grid.
     */
    [[nodiscard]] int cols() const noexcept { return cols_; }

    /**
     * @brief Current cursor row (0-indexed).
     */
    [[nodiscard]] int cursor_row() const noexcept { return cursor_row_; }

    /**
     * @brief Current cursor column (0-indexed).
     */
    [[nodiscard]] int cursor_col() const noexcept { return cursor_col_; }

    /**
     * @brief Gets character at specified grid position.
     * @param row Grid row (0 to rows-1).
     * @param col Grid col (0 to cols-1).
     * @return Character byte, or ' ' if out of bounds.
     */
    [[nodiscard]] char char_at(int row, int col) const noexcept {
        if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
            return ' ';
        }
        return cells_[static_cast<size_t>(row * cols_ + col)].c;
    }

    /**
     * @brief Gets attribute byte at specified grid position.
     */
    [[nodiscard]] uint8_t attr_at(int row, int col) const noexcept {
        if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
            return 0;
        }
        return cells_[static_cast<size_t>(row * cols_ + col)].attr;
    }

    /**
     * @brief Feeds raw input stream (from PTY or synthetic source) into ANSI decoder.
     * @param data Stream slice to parse.
     */
    void feed_input(std::string_view data) {
        parser_.feed(data);
    }

    /**
     * @brief Handler callback for ANSI Cursor Position (CUP).
     * @param row 1-indexed row.
     * @param col 1-indexed column.
     */
    void on_cursor_move(int row, int col) override {
        int target_row = (row > 0) ? (row - 1) : 0;
        int target_col = (col > 0) ? (col - 1) : 0;

        if (target_row >= rows_) target_row = rows_ - 1;
        if (target_col >= cols_) target_col = cols_ - 1;

        cursor_row_ = target_row;
        cursor_col_ = target_col;
    }

    /**
     * @brief Handler callback for ANSI Erase Display (ED).
     * @param mode Erase mode: 0 (cursor to end), 1 (start to cursor), 2 (entire display).
     */
    void on_erase_display(int mode) override {
        if (mode == 2) {
            for (auto& cell : cells_) {
                cell = Cell{' ', 0};
            }
            cursor_row_ = 0;
            cursor_col_ = 0;
        } else if (mode == 0) {
            int start_idx = cursor_row_ * cols_ + cursor_col_;
            for (size_t i = static_cast<size_t>(start_idx); i < cells_.size(); ++i) {
                cells_[i] = Cell{' ', 0};
            }
        } else if (mode == 1) {
            int end_idx = cursor_row_ * cols_ + cursor_col_;
            for (int i = 0; i <= end_idx && static_cast<size_t>(i) < cells_.size(); ++i) {
                cells_[static_cast<size_t>(i)] = Cell{' ', 0};
            }
        }
    }

    /**
     * @brief Handler callback for printing a single character.
     * @param c Character to print or control character to process.
     */
    void on_print_char(char c) override {
        switch (c) {
        case '\r':
            cursor_col_ = 0;
            break;

        case '\n':
            advance_row();
            break;

        case '\b':
            if (cursor_col_ > 0) {
                --cursor_col_;
            }
            break;

        case '\t': {
            int next_tab = (cursor_col_ + 8) & ~7;
            if (next_tab >= cols_) {
                advance_row();
                cursor_col_ = 0;
            } else {
                cursor_col_ = next_tab;
            }
            break;
        }

        default:
            if (static_cast<unsigned char>(c) >= 32) {
                if (cursor_col_ >= cols_) {
                    advance_row();
                    cursor_col_ = 0;
                }
                size_t idx = static_cast<size_t>(cursor_row_ * cols_ + cursor_col_);
                cells_[idx] = Cell{c, 0};
                ++cursor_col_;
            }
            break;
        }
    }

    /**
     * @brief Spawns a shell or command in a new pseudo-terminal (PTY).
     * @param shell Command path (e.g. "/bin/sh").
     * @return true if PTY and child process launched successfully.
     */
    bool spawn_pty(const char* shell = "/bin/sh") {
        int master = ::posix_openpt(O_RDWR | O_NOCTTY);
        if (master < 0) {
            return false;
        }

        if (::grantpt(master) < 0 || ::unlockpt(master) < 0) {
            ::close(master);
            return false;
        }

        const char* slave_name = ::ptsname(master);
        if (slave_name == nullptr) {
            ::close(master);
            return false;
        }

        pid_t pid = ::fork();
        if (pid < 0) {
            ::close(master);
            return false;
        }

        if (pid == 0) {
            // Child process
            ::setsid();
            int slave = ::open(slave_name, O_RDWR);
            if (slave >= 0) {
                ::dup2(slave, STDIN_FILENO);
                ::dup2(slave, STDOUT_FILENO);
                ::dup2(slave, STDERR_FILENO);
                if (slave > STDERR_FILENO) {
                    ::close(slave);
                }
            }
            ::close(master);

            struct winsize ws{};
            ws.ws_row = static_cast<unsigned short>(rows_);
            ws.ws_col = static_cast<unsigned short>(cols_);
            ::ioctl(STDIN_FILENO, TIOCSWINSZ, &ws);

            ::execlp(shell, shell, nullptr);
            ::_exit(127);
        }

        pty_master_.reset(master);
        child_pid_ = pid;
        return true;
    }

    /**
     * @brief Gets raw master file descriptor for PTY I/O.
     */
    [[nodiscard]] int pty_fd() const noexcept {
        return pty_master_.get();
    }

    /**
     * @brief Renders the entire terminal grid into a 4bpp raster surface.
     * @param dst Destination pixmap.
     * @param font FontRenderer instance for glyph rasterization.
     * @param show_cursor If true, draws cursor at current position.
     */
    void render(graphics::OwnedPixmap& dst, const graphics::FontRenderer& font,
                bool show_cursor = true) const {
        int gw = font.glyph_width();
        int gh = font.glyph_height();

        for (int r = 0; r < rows_; ++r) {
            int y = r * gh;
            for (int c = 0; c < cols_; ++c) {
                int x = c * gw;
                size_t idx = static_cast<size_t>(r * cols_ + c);
                char ch = cells_[idx].c;

                bool is_cursor = show_cursor && (r == cursor_row_ && c == cursor_col_);
                uint8_t fg = is_cursor ? 0x0F : 0x00;
                uint8_t bg = is_cursor ? 0x00 : 0x0F;

                font.draw_char(dst, x, y, static_cast<uint32_t>(static_cast<unsigned char>(ch)), fg, bg);
            }
        }
    }

private:
    void advance_row() {
        if (cursor_row_ + 1 < rows_) {
            ++cursor_row_;
        } else {
            scroll_up();
        }
    }

    void scroll_up() {
        if (rows_ <= 1) {
            return;
        }
        size_t row_stride = static_cast<size_t>(cols_);
        std::memmove(cells_.data(), cells_.data() + row_stride,
                     static_cast<size_t>(rows_ - 1) * row_stride * sizeof(Cell));
        for (int c = 0; c < cols_; ++c) {
            cells_[static_cast<size_t>((rows_ - 1) * cols_ + c)] = Cell{' ', 0};
        }
    }

    int rows_{24};
    int cols_{80};
    int cursor_row_{0};
    int cursor_col_{0};
    std::vector<Cell> cells_;
    AnsiParser parser_;
    core::UniqueFd pty_master_;
    pid_t child_pid_{-1};
};

} // namespace terminal
} // namespace myts

#endif // MYTS_TERMINAL_TERMINAL_SESSION_HPP
