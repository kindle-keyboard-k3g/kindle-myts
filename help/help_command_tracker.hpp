#ifndef MYTS_HELP_HELP_COMMAND_TRACKER_HPP
#define MYTS_HELP_HELP_COMMAND_TRACKER_HPP

#include <cstdint>
#include <string_view>

namespace myts {
namespace help {

/**
 * @brief Zero-allocation observer for typed "help\r" command sequence.
 */
class HelpCommandTracker {
public:
    HelpCommandTracker() noexcept = default;

    void reset() noexcept {
        len_ = 0;
        buffer_[0] = '\0';
    }

    [[nodiscard]] std::string_view current_text() const noexcept {
        return std::string_view(buffer_, len_ & LEN_MASK);
    }

    bool feed(std::string_view seq) noexcept {
        bool triggered = false;
        for (char c : seq) {
            triggered = process_char(c) || triggered;
        }
        return triggered;
    }

private:
    static constexpr uint8_t FLAG_ESCAPE = 0x80;
    static constexpr uint8_t FLAG_CSI    = 0x40;
    static constexpr uint8_t FLAG_MASK   = 0xC0;
    static constexpr uint8_t LEN_MASK    = 0x0F;

    bool process_char(char c) noexcept {
        uint8_t flags = len_ & FLAG_MASK;
        if (flags != 0) {
            return process_escape_char(c, flags);
        }
        if (c == '\033') {
            len_ = FLAG_ESCAPE;
            buffer_[0] = '\0';
            return false;
        }
        if (c == '\r' || c == '\n') {
            return check_and_reset();
        }
        if (c == '\x7f' || c == '\b') {
            handle_backspace();
            return false;
        }
        if (is_control_char(c)) {
            reset();
            return false;
        }
        append_char(c);
        return false;
    }

    bool process_escape_char(char c, uint8_t flags) noexcept {
        if (c == '\r' || c == '\n') {
            reset();
            return false;
        }
        if (flags == FLAG_ESCAPE) {
            if (c == '[') {
                len_ = FLAG_CSI;
                return false;
            }
            reset();
            return false;
        }
        auto uc = static_cast<unsigned char>(c);
        if (uc >= 0x40 && uc <= 0x7E) {
            reset();
        }
        return false;
    }

    bool check_and_reset() noexcept {
        bool match = (current_text() == "help");
        reset();
        return match;
    }

    void handle_backspace() noexcept {
        uint8_t l = len_ & LEN_MASK;
        if (l == 0) {
            return;
        }
        --l;
        len_ = l;
        buffer_[l] = '\0';
    }

    static bool is_control_char(char c) noexcept {
        return static_cast<unsigned char>(c) < 32 && c != '\r' && c != '\n';
    }

    void append_char(char c) noexcept {
        uint8_t l = len_ & LEN_MASK;
        if (l < sizeof(buffer_) - 1) {
            buffer_[l] = c;
            len_ = l + 1;
            buffer_[len_] = '\0';
            return;
        }
        buffer_[0] = '~';
        len_ = sizeof(buffer_) - 1;
    }

    char buffer_[8]{};
    uint8_t len_{0};
};

} // namespace help
} // namespace myts

#endif // MYTS_HELP_HELP_COMMAND_TRACKER_HPP
