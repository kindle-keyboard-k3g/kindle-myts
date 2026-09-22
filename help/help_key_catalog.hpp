#ifndef MYTS_HELP_HELP_KEY_CATALOG_HPP
#define MYTS_HELP_HELP_KEY_CATALOG_HPP

#include "input/key_catalog.hpp"
#include <cstddef>
#include <cstdint>

namespace myts {
namespace help {

struct PhysicalKey {
    uint16_t code;
    char primary;
    char shift;
    const char* label;
};

struct SymEntry {
    char key;
    char symbol;
    const char* desc;
};

struct FnEntry {
    char key;
    const char* f_label;
    const char* desc;
};

class HelpKeyCatalog {
public:
    static constexpr size_t ROW1_COUNT = 10;
    static constexpr size_t ROW2_COUNT = 10;
    static constexpr size_t ROW3_COUNT = 10;
    static constexpr size_t SYM_COUNT = 28;
    static constexpr size_t FN_COUNT = 28;

    static constexpr PhysicalKey ROW1[ROW1_COUNT] = {
        {16, 'Q', '!', "Q"}, {17, 'W', '@', "W"}, {18, 'E', '#', "E"},
        {19, 'R', '$', "R"}, {20, 'T', '%', "T"}, {21, 'Y', '^', "Y"},
        {22, 'U', '&', "U"}, {23, 'I', '*', "I"}, {24, 'O', '(', "O"},
        {25, 'P', ')', "P"}
    };

    static constexpr PhysicalKey ROW2[ROW2_COUNT] = {
        {30, 'A', '\0', "A"}, {31, 'S', '\0', "S"}, {32, 'D', '\0', "D"},
        {33, 'F', '\0', "F"}, {34, 'G', '\0', "G"}, {35, 'H', '\0', "H"},
        {36, 'J', '\0', "J"}, {37, 'K', '\0', "K"}, {38, 'L', '\0', "L"},
        {input::KeyCatalog::CODE_DEL, '<', '\0', "DEL"}
    };

    static constexpr PhysicalKey ROW3[ROW3_COUNT] = {
        {44, 'Z', '\0', "Z"}, {45, 'X', '\0', "X"}, {46, 'C', '\0', "C"},
        {47, 'V', '\0', "V"}, {48, 'B', '\0', "B"}, {49, 'N', '\0', "N"},
        {50, 'M', '\0', "M"}, {52, '.', '>', "."}, {53, '/', '?', "/"},
        {input::KeyCatalog::CODE_ENTER, '\r', '\0', "RET"}
    };

    static constexpr SymEntry SYM_ENTRIES[SYM_COUNT] = {
        {'q', '!', "Exclamation"}, {'w', '@', "At sign"},
        {'e', '#', "Hash / Octothorpe"}, {'r', '$', "Dollar"},
        {'t', '%', "Percent"}, {'y', '^', "Caret"},
        {'u', '&', "Ampersand"}, {'i', '*', "Asterisk"},
        {'o', '(', "Left Paren"}, {'p', ')', "Right Paren"},
        {'a', '*', "Asterisk"}, {'s', '+', "Plus"},
        {'d', '#', "Hash"}, {'f', '-', "Minus / Dash"},
        {'g', '_', "Underscore"}, {'h', '(', "Left Paren"},
        {'j', ')', "Right Paren"}, {'k', '&', "Ampersand"},
        {'l', '!', "Exclamation"}, {'D', '?', "Question mark"},
        {'z', '~', "Tilde"}, {'x', '$', "Dollar"},
        {'c', '|', "Pipe"}, {'v', '/', "Slash"},
        {'b', '\\', "Backslash"}, {'n', '\"', "Double quote"},
        {'m', '\'', "Single quote"}, {'.', ':', "Colon"}
    };

    static constexpr FnEntry FN_ENTRIES[FN_COUNT] = {
        {'q', "F1", "VT Function 1"}, {'w', "F2", "VT Function 2"},
        {'e', "F3", "VT Function 3"}, {'r', "F4", "VT Function 4"},
        {'t', "F5", "VT Function 5"}, {'y', "F6", "VT Function 6"},
        {'u', "F7", "VT Function 7"}, {'i', "F8", "VT Function 8"},
        {'o', "F9", "VT Function 9"}, {'p', "F10", "VT Function 10"},
        {'a', "`", "Backtick"}, {'s', "%", "Percent"},
        {'d', "^", "Caret"}, {'f', "<", "Less Than"},
        {'g', ">", "Greater Than"}, {'h', "[", "Left Bracket"},
        {'j', "]", "Right Bracket"}, {'k', "=", "Equals"},
        {'l', "F11", "VT Function 11"}, {'D', "F12", "VT Function 12"},
        {'z', "\\t", "Tab"}, {'x', ";", "Semicolon"},
        {'c', ",", "Comma"}, {'v', "(", "Left Paren"},
        {'b', ")", "Right Paren"}, {'n', "{", "Left Brace"},
        {'m', "}", "Right Brace"}, {'.', ",", "Comma"}
    };

    [[nodiscard]] static const char* keycode_to_name(uint16_t code) noexcept {
        if (code == input::KeyCatalog::CODE_MENU) return "Menu";
        if (input::KeyCatalog::is_back_key(code)) return "Back";
        if (code == input::KeyCatalog::CODE_PAGE_FORWARD) return "Right<";
        if (code == input::KeyCatalog::CODE_PAGE_TURN_K3 || code == input::KeyCatalog::CODE_PAGE_TURN_DX) return "Right>";
        if (code == input::KeyCatalog::CODE_PAGE_BACK_K3 || code == input::KeyCatalog::CODE_PAGE_BACK_DX) return "Left<";
        if (code == input::KeyCatalog::CODE_PAGE_BACK_DX) return "Left>";
        if (code == input::KeyCatalog::CODE_FIVEWAY_UP) return "Up";
        if (code == input::KeyCatalog::CODE_FIVEWAY_DOWN) return "Down";
        if (code == input::KeyCatalog::CODE_FIVEWAY_LEFT) return "Left";
        if (code == input::KeyCatalog::CODE_FIVEWAY_RIGHT) return "Right";
        if (input::KeyCatalog::is_select_key(code)) return "Select";
        if (code == input::KeyCatalog::CODE_ENTER) return "Enter";
        if (code == input::KeyCatalog::CODE_DEL) return "Del";
        if (code == input::KeyCatalog::CODE_SPACE) return "Space";
        if (code == input::KeyCatalog::CODE_AA_CTRL_K3 || code == input::KeyCatalog::CODE_AA_CTRL_DX) return "aA / Ctrl";
        if (code == input::KeyCatalog::CODE_SYM_K3 || code == input::KeyCatalog::CODE_SYM_DX) return "Sym";
        if (code == input::KeyCatalog::CODE_HOME_K3 || code == input::KeyCatalog::CODE_HOME_DX) return "Home";
        if (code == input::KeyCatalog::CODE_SHIFT_L || code == input::KeyCatalog::CODE_SHIFT_R) return "Shift";
        if (code == input::KeyCatalog::CODE_ALT_L || code == input::KeyCatalog::CODE_ALT_R) return "Alt";
        return nullptr;
    }
};

} // namespace help
} // namespace myts

#endif // MYTS_HELP_HELP_KEY_CATALOG_HPP
