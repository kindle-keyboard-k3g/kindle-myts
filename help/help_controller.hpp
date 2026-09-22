#ifndef MYTS_HELP_HELP_CONTROLLER_HPP
#define MYTS_HELP_HELP_CONTROLLER_HPP

#include "help/help_command_tracker.hpp"
#include "help/help_screen.hpp"
#include "input/input_manager.hpp"
#include "input/key_catalog.hpp"
#include <linux/input.h>
#include <string_view>

namespace myts {
namespace help {

/**
 * @brief Coordinates help screen state and input routing for Application.
 */
class HelpController {
public:
    HelpController() noexcept = default;

    [[nodiscard]] bool active() const noexcept {
        return screen_.active();
    }

    [[nodiscard]] const HelpScreen& screen() const noexcept {
        return screen_;
    }

    void open() noexcept {
        screen_.open();
    }

    void close() noexcept {
        screen_.close();
    }

    HelpRoute before_terminal_write(const struct input_event& ev,
                                   const input::ModifierState& mods,
                                   std::string_view seq) noexcept {
        if (!screen_.active()) {
            return handle_inactive_event(ev, mods);
        }
        if (ev.value == 0) {
            return HelpRoute::Consume;
        }
        if (is_exit_key(ev, seq)) {
            screen_.close();
            return HelpRoute::Exit;
        }
        if (handle_navigation_key(ev, seq)) {
            return HelpRoute::Redraw;
        }
        record_and_redraw(ev, mods, seq);
        return HelpRoute::Redraw;
    }

    HelpRoute after_terminal_write(std::string_view seq) noexcept {
        if (tracker_.feed(seq)) {
            screen_.open();
            return HelpRoute::Redraw;
        }
        return HelpRoute::Pass;
    }

private:
    HelpRoute handle_inactive_event(const struct input_event& ev,
                                   const input::ModifierState& mods) noexcept {
        if (ev.type != EV_KEY || ev.value != 1) {
            return HelpRoute::Pass;
        }
        if (input::KeyCatalog::is_menu_key(ev.code)) {
            screen_.open();
            return HelpRoute::Redraw;
        }
        if (mods.shift && ev.code == input::KeyCatalog::CODE_H) {
            screen_.open();
            return HelpRoute::Redraw;
        }
        return HelpRoute::Pass;
    }

    static bool is_exit_key(const struct input_event& ev, std::string_view seq) noexcept {
        if (input::KeyCatalog::is_exit_trigger(ev.code)) return true;
        if (ev.code == input::KeyCatalog::CODE_ENTER || seq == "\r") return true;
        if (seq == "q" || ev.code == input::KeyCatalog::CODE_Q) return true;
        return false;
    }

    bool handle_navigation_key(const struct input_event& ev, std::string_view seq) noexcept {
        if (ev.code == 2 || seq == "1") { screen_.set_page(HelpPage::Overview); return true; }
        if (ev.code == 3 || seq == "2") { screen_.set_page(HelpPage::Keypad); return true; }
        if (ev.code == 4 || seq == "3") { screen_.set_page(HelpPage::Sym); return true; }
        if (ev.code == 5 || seq == "4") { screen_.set_page(HelpPage::Fn); return true; }
        if (is_prev_key(ev, seq)) { screen_.prev_page(); return true; }
        if (is_next_key(ev, seq)) { screen_.next_page(); return true; }
        return false;
    }

    static bool is_prev_key(const struct input_event& ev, std::string_view seq) noexcept {
        return ev.code == input::KeyCatalog::CODE_FIVEWAY_LEFT ||
               ev.code == input::KeyCatalog::CODE_PAGE_BACK_K3 ||
               ev.code == input::KeyCatalog::CODE_PAGE_BACK_DX ||
               seq == "\033[D";
    }

    static bool is_next_key(const struct input_event& ev, std::string_view seq) noexcept {
        return ev.code == input::KeyCatalog::CODE_FIVEWAY_RIGHT ||
               ev.code == input::KeyCatalog::CODE_PAGE_FORWARD ||
               seq == "\033[C";
    }

    void record_and_redraw(const struct input_event& ev,
                          const input::ModifierState& mods,
                          std::string_view seq) noexcept {
        KeySnapshot snap{};
        snap.code = ev.code;
        snap.character = (seq.size() == 1) ? seq[0] : '\0';
        snap.modifiers = (mods.shift ? input::KeyCatalog::MOD_SHIFT : 0) |
                         (mods.ctrl ? input::KeyCatalog::MOD_CTRL : 0) |
                         (mods.alt ? input::KeyCatalog::MOD_ALT : 0) |
                         (mods.sym ? input::KeyCatalog::MOD_SYM : 0);
        screen_.record_key(snap);
    }

    HelpScreen screen_{};
    HelpCommandTracker tracker_{};
};

} // namespace help
} // namespace myts

#endif // MYTS_HELP_HELP_CONTROLLER_HPP
