#ifndef MYTS_HELP_HELP_SCREEN_HPP
#define MYTS_HELP_HELP_SCREEN_HPP

#include "help/help_types.hpp"

namespace myts {
namespace help {

/**
 * @brief State container for active help modal and page navigation.
 */
class HelpScreen {
public:
    HelpScreen() noexcept = default;

    [[nodiscard]] bool active() const noexcept {
        return active_;
    }

    void open() noexcept {
        active_ = true;
        nav_.page = HelpPage::Overview;
        nav_.last_key = KeySnapshot{};
    }

    void close() noexcept {
        active_ = false;
    }

    [[nodiscard]] const HelpNavigationState& state() const noexcept {
        return nav_;
    }

    void set_page(HelpPage page) noexcept {
        nav_.page = page;
    }

    void next_page() noexcept {
        uint8_t next = static_cast<uint8_t>(nav_.page) + 1;
        if (next > static_cast<uint8_t>(HelpPage::Fn)) {
            next = static_cast<uint8_t>(HelpPage::Overview);
        }
        nav_.page = static_cast<HelpPage>(next);
    }

    void prev_page() noexcept {
        if (nav_.page == HelpPage::Overview) {
            nav_.page = HelpPage::Fn;
            return;
        }
        uint8_t prev = static_cast<uint8_t>(nav_.page) - 1;
        nav_.page = static_cast<HelpPage>(prev);
    }

    void record_key(KeySnapshot snap) noexcept {
        nav_.last_key = snap;
    }

private:
    bool active_{false};
    HelpNavigationState nav_{};
};

} // namespace help
} // namespace myts

#endif // MYTS_HELP_HELP_SCREEN_HPP
