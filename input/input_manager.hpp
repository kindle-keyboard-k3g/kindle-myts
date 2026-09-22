#ifndef MYTS_INPUT_INPUT_MANAGER_HPP
#define MYTS_INPUT_INPUT_MANAGER_HPP

#include "input/key_catalog.hpp"
#include <linux/input.h>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace myts {
namespace input {

/**
 * @brief Tracks modifier state (Shift, Ctrl, Alt, Sym) for keyboard decoding.
 */
struct ModifierState {
    bool shift{false};
    bool ctrl{false};
    bool alt{false};
    bool sym{false};
};

/**
 * @brief Zero-allocation Linux input event translator for Kindle hardware keypad and 5-way joystick.
 *
 * Translates low-level Linux struct input_event into ANSI terminal character sequences.
 * Adheres to embedded C++ constraints (-fno-exceptions, -fno-rtti, zero dynamic allocation).
 */
class InputManager {
public:
    InputManager() noexcept = default;

    /**
     * @brief Current active modifier keys.
     */
    [[nodiscard]] const ModifierState& modifiers() const noexcept {
        return mods_;
    }

    /**
     * @brief Resets all active modifier states.
     */
    void reset_modifiers() noexcept {
        mods_ = ModifierState{};
    }

    /**
     * @brief Processes a Linux input_event and decodes it to an ANSI/ASCII sequence.
     * @param ev Reference to Linux input_event structure.
     * @return String view containing the decoded character sequence, or empty string view if no character produced.
     */
    std::string_view process_event(const struct input_event& ev) noexcept {
        if (ev.type != EV_KEY) {
            return {};
        }

        // Handle modifier presses and releases (value 0=release, 1=press, 2=repeat)
        if (is_modifier(ev.code)) {
            update_modifier(ev.code, ev.value != 0);
            return {};
        }

        // On key release, no character is emitted
        if (ev.value == 0) {
            return {};
        }

        return translate_keycode(ev.code);
    }

private:
    static bool is_modifier(uint16_t code) noexcept {
        return code == 42 || code == 54   // Shift (L/R)
            || code == 29 || code == 97   // Ctrl (L/R)
            || code == KeyCatalog::CODE_AA_CTRL_K3 || code == KeyCatalog::CODE_AA_CTRL_DX
            || code == 56 || code == 100  // Alt (L/R)
            || code == KeyCatalog::CODE_SYM_K3 || code == KeyCatalog::CODE_SYM_DX;
    }

    void update_modifier(uint16_t code, bool active) noexcept {
        if (code == 42 || code == 54) {
            mods_.shift = active;
            return;
        }
        if (code == 29 || code == 97 ||
            code == KeyCatalog::CODE_AA_CTRL_K3 || code == KeyCatalog::CODE_AA_CTRL_DX) {
            mods_.ctrl = active;
            return;
        }
        if (code == 56 || code == 100) {
            mods_.alt = active;
            return;
        }
        if (code == KeyCatalog::CODE_SYM_K3 || code == KeyCatalog::CODE_SYM_DX) {
            mods_.sym = active;
        }
    }

    std::string_view emit(const char* str) noexcept {
        size_t len = std::strlen(str);
        if (len >= sizeof(buf_)) {
            len = sizeof(buf_) - 1;
        }
        std::memcpy(buf_, str, len);
        buf_[len] = '\0';
        return std::string_view(buf_, len);
    }

    std::string_view emit_char(char c) noexcept {
        buf_[0] = c;
        buf_[1] = '\0';
        return std::string_view(buf_, 1);
    }

    std::string_view translate_keycode(uint16_t code) noexcept {
        // Special function keys
        switch (code) {
        case 28: // Enter
            return emit("\r");
        case 57: // Space
            return emit(" ");
        case 14: // Backspace / Del
            return emit("\x7f");
        case 103: // Up (five-way or keypad)
        case 122:
            return emit(mods_.shift ? "\033[5~" : "\033[A");
        case 108: // Down
        case 123:
            return emit(mods_.shift ? "\033[6~" : "\033[B");
        case 105: // Left
            return emit("\033[D");
        case 106: // Right
            return emit("\033[C");
        case 52:  // '.'
            return emit(mods_.shift ? ">" : ".");
        case 53:  // '/'
            return emit(mods_.shift ? "?" : "/");
        default:
            break;
        }

        // Numbers 0..9 (Linux keycodes 2..11)
        if (code >= 2 && code <= 11) {
            int digit = (code == 11) ? 0 : (code - 1);
            if (mods_.shift) {
                const char shift_digits[] = ")!@#$%^&*(";
                return emit_char(shift_digits[digit]);
            }
            return emit_char(static_cast<char>('0' + digit));
        }

        // Alphabet
        char letter = code_to_letter(code);
        if (letter != '\0') {
            if (mods_.ctrl) {
                return emit_char(static_cast<char>(letter - 'a' + 1));
            }
            if (mods_.shift) {
                return emit_char(static_cast<char>(letter - 'a' + 'A'));
            }
            return emit_char(letter);
        }

        return {};
    }

    static char code_to_letter(uint16_t code) noexcept {
        // Row 1: q w e r t y u i o p
        static constexpr uint16_t row1[] = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25};
        for (size_t i = 0; i < 10; ++i) {
            if (code == row1[i]) {
                const char letters[] = "qwertyuiop";
                return letters[i];
            }
        }

        // Row 2: a s d f g h j k l
        static constexpr uint16_t row2[] = {30, 31, 32, 33, 34, 35, 36, 37, 38};
        for (size_t i = 0; i < 9; ++i) {
            if (code == row2[i]) {
                const char letters[] = "asdfghjkl";
                return letters[i];
            }
        }

        // Row 3: z x c v b n m
        static constexpr uint16_t row3[] = {44, 45, 46, 47, 48, 49, 50};
        for (size_t i = 0; i < 7; ++i) {
            if (code == row3[i]) {
                const char letters[] = "zxcvbnm";
                return letters[i];
            }
        }

        return '\0';
    }

    ModifierState mods_{};
    char buf_[16]{};
};

} // namespace input
} // namespace myts

#endif // MYTS_INPUT_INPUT_MANAGER_HPP
