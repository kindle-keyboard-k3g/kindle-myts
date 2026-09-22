#ifndef MYTS_HELP_HELP_TYPES_HPP
#define MYTS_HELP_HELP_TYPES_HPP

#include <cstdint>

namespace myts {
namespace help {

/**
 * @brief Identifies the active tab page in the help system.
 */
enum class HelpPage : uint8_t {
    Overview = 0,
    Keypad = 1,
    Sym = 2,
    Fn = 3
};

/**
 * @brief Routing decision emitted by the help controller for input events.
 */
enum class HelpRoute : uint8_t {
    Pass,           ///< Forward to active terminal / PTY
    Consume,        ///< Intercept and discard without PTY or UI update
    OpenAfterWrite, ///< Forward current sequence to PTY, then open help
    Redraw,         ///< Help screen updated (e.g. key pressed or tab switched)
    Exit            ///< Close help screen and restore terminal
};

/**
 * @brief Immutable snapshot of a recent physical or logical key press.
 */
struct KeySnapshot {
    uint16_t code{0};
    char character{'\0'};
    uint8_t modifiers{0};
};

/**
 * @brief Encapsulates current navigation state (active page and recent key).
 */
struct HelpNavigationState {
    HelpPage page{HelpPage::Overview};
    KeySnapshot last_key{};
};

} // namespace help
} // namespace myts

#endif // MYTS_HELP_HELP_TYPES_HPP
