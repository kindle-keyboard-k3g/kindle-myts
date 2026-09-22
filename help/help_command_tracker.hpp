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
        return std::string_view(buffer_, len_);
    }

    bool feed(std::string_view seq) noexcept {
        bool triggered = false;
        for (char c : seq) {
            triggered = process_char(c) || triggered;
        }
        return triggered;
    }

private:
    bool process_char(char c) noexcept {
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

    bool check_and_reset() noexcept {
        bool match = (current_text() == "help");
        reset();
        return match;
    }

    void handle_backspace() noexcept {
        if (len_ == 0) {
            return;
        }
        --len_;
        buffer_[len_] = '\0';
    }

    static bool is_control_char(char c) noexcept {
        return static_cast<unsigned char>(c) < 32 && c != '\r' && c != '\n';
    }

    void append_char(char c) noexcept {
        if (len_ < sizeof(buffer_) - 1) {
            buffer_[len_++] = c;
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
