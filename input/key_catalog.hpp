#ifndef MYTS_INPUT_KEY_CATALOG_HPP
#define MYTS_INPUT_KEY_CATALOG_HPP

#include <cstdint>

namespace myts {
namespace input {

/**
 * @brief Canonical Kindle hardware keycodes and modifier bitmasks.
 *
 * Prefixed with CODE_* to avoid macro collisions with Linux <linux/input-event-codes.h>.
 */
struct KeyCatalog {
    static constexpr uint16_t CODE_MENU = 139;
    static constexpr uint16_t CODE_BACK_K3 = 158;
    static constexpr uint16_t CODE_BACK_DX = 91;
    static constexpr uint16_t CODE_PAGE_FORWARD = 109; // Right<
    static constexpr uint16_t CODE_PAGE_TURN_K3 = 191; // Right>
    static constexpr uint16_t CODE_PAGE_TURN_DX = 124; // Right> (DX)
    static constexpr uint16_t CODE_PAGE_BACK_K3 = 193; // Left<
    static constexpr uint16_t CODE_PAGE_BACK_DX = 104; // Left>
    static constexpr uint16_t CODE_AA_CTRL_K3 = 190;
    static constexpr uint16_t CODE_AA_CTRL_DX = 90;
    static constexpr uint16_t CODE_SYM_K3 = 126;
    static constexpr uint16_t CODE_SYM_DX = 94;
    static constexpr uint16_t CODE_FIVEWAY_UP = 103;
    static constexpr uint16_t CODE_FIVEWAY_DOWN = 108;
    static constexpr uint16_t CODE_FIVEWAY_LEFT = 105;
    static constexpr uint16_t CODE_FIVEWAY_RIGHT = 106;
    static constexpr uint16_t CODE_SELECT_K3 = 194;
    static constexpr uint16_t CODE_SELECT_DX = 92;
    static constexpr uint16_t CODE_DEL = 14;
    static constexpr uint16_t CODE_ENTER = 28;
    static constexpr uint16_t CODE_SPACE = 57;
    static constexpr uint16_t CODE_Q = 16;
    static constexpr uint16_t CODE_H = 35;

    static constexpr uint8_t MOD_NONE = 0;
    static constexpr uint8_t MOD_SHIFT = 1 << 0;
    static constexpr uint8_t MOD_CTRL = 1 << 1;
    static constexpr uint8_t MOD_ALT = 1 << 2;
    static constexpr uint8_t MOD_SYM = 1 << 3;

    [[nodiscard]] static constexpr bool is_back_key(uint16_t code) noexcept {
        return code == CODE_BACK_K3 || code == CODE_BACK_DX;
    }

    [[nodiscard]] static constexpr bool is_menu_key(uint16_t code) noexcept {
        return code == CODE_MENU;
    }

    [[nodiscard]] static constexpr bool is_select_key(uint16_t code) noexcept {
        return code == CODE_SELECT_K3 || code == CODE_SELECT_DX;
    }

    [[nodiscard]] static constexpr bool is_exit_trigger(uint16_t code) noexcept {
        return is_back_key(code) || is_menu_key(code) ||
               code == CODE_PAGE_TURN_K3 || code == CODE_PAGE_TURN_DX;
    }

    [[nodiscard]] static constexpr bool is_arrow_key(uint16_t code) noexcept {
        return code == CODE_FIVEWAY_UP || code == CODE_FIVEWAY_DOWN ||
               code == CODE_FIVEWAY_LEFT || code == CODE_FIVEWAY_RIGHT;
    }
};

} // namespace input
} // namespace myts

#endif // MYTS_INPUT_KEY_CATALOG_HPP
