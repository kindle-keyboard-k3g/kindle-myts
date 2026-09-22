#include "tests/test_framework.h"
#include "input/key_catalog.hpp"
#include "help/help_types.hpp"
#include "help/help_key_catalog.hpp"
#include "help/help_screen.hpp"
#include "help/help_controller.hpp"
#include <linux/input.h>

using namespace myts;
using namespace myts::input;
using namespace myts::help;

static struct input_event make_ev(uint16_t type, uint16_t code, int32_t val) {
    struct input_event ev{};
    ev.type = type;
    ev.code = code;
    ev.value = val;
    return ev;
}

static int test_key_catalog_constants_and_helpers() {
    ASSERT_EQ(KeyCatalog::CODE_MENU, 139);
    ASSERT_EQ(KeyCatalog::CODE_BACK_K3, 158);
    ASSERT_EQ(KeyCatalog::CODE_BACK_DX, 91);
    ASSERT_EQ(KeyCatalog::CODE_PAGE_FORWARD, 109);
    ASSERT_EQ(KeyCatalog::CODE_PAGE_TURN_K3, 191);
    ASSERT_EQ(KeyCatalog::CODE_PAGE_TURN_DX, 124);
    ASSERT_EQ(KeyCatalog::CODE_SELECT_K3, 194);
    ASSERT_EQ(KeyCatalog::CODE_SELECT_DX, 92);

    ASSERT_TRUE(KeyCatalog::is_back_key(158));
    ASSERT_TRUE(KeyCatalog::is_back_key(91));
    ASSERT_FALSE(KeyCatalog::is_back_key(100));

    ASSERT_TRUE(KeyCatalog::is_menu_key(139));
    ASSERT_FALSE(KeyCatalog::is_menu_key(140));

    ASSERT_TRUE(KeyCatalog::is_select_key(194));
    ASSERT_TRUE(KeyCatalog::is_select_key(92));
    ASSERT_FALSE(KeyCatalog::is_select_key(28)); // Enter is not select

    ASSERT_TRUE(KeyCatalog::is_exit_trigger(KeyCatalog::CODE_BACK_K3));
    ASSERT_TRUE(KeyCatalog::is_exit_trigger(KeyCatalog::CODE_BACK_DX));
    ASSERT_TRUE(KeyCatalog::is_exit_trigger(KeyCatalog::CODE_MENU));
    ASSERT_TRUE(KeyCatalog::is_exit_trigger(KeyCatalog::CODE_PAGE_TURN_K3));

    return 0;
}

static int test_help_types_and_state() {
    ASSERT_EQ(static_cast<uint8_t>(HelpPage::Overview), 0);
    ASSERT_EQ(static_cast<uint8_t>(HelpPage::Keypad), 1);
    ASSERT_EQ(static_cast<uint8_t>(HelpPage::Sym), 2);
    ASSERT_EQ(static_cast<uint8_t>(HelpPage::Fn), 3);

    HelpNavigationState state{};
    ASSERT_EQ(static_cast<uint8_t>(state.page), static_cast<uint8_t>(HelpPage::Overview));
    ASSERT_EQ(state.last_key.code, 0);
    ASSERT_EQ(state.last_key.character, '\0');
    ASSERT_EQ(state.last_key.modifiers, 0);

    return 0;
}

static int test_help_key_catalog_layouts() {
    ASSERT_EQ(HelpKeyCatalog::ROW1_COUNT, 10);
    ASSERT_EQ(HelpKeyCatalog::ROW2_COUNT, 10);
    ASSERT_EQ(HelpKeyCatalog::ROW3_COUNT, 10);

    ASSERT_EQ(HelpKeyCatalog::ROW1[0].primary, 'Q');
    ASSERT_EQ(HelpKeyCatalog::ROW1[0].shift, '!');
    ASSERT_EQ(HelpKeyCatalog::ROW1[9].primary, 'P');
    ASSERT_EQ(HelpKeyCatalog::ROW1[9].shift, ')');

    ASSERT_EQ(HelpKeyCatalog::ROW2[0].primary, 'A');
    ASSERT_EQ(HelpKeyCatalog::ROW2[9].primary, '<');

    ASSERT_EQ(HelpKeyCatalog::ROW3[0].primary, 'Z');
    ASSERT_EQ(HelpKeyCatalog::ROW3[9].primary, '\r');

    ASSERT_TRUE(HelpKeyCatalog::SYM_COUNT >= 20);
    ASSERT_EQ(HelpKeyCatalog::SYM_ENTRIES[0].key, 'Q');
    ASSERT_EQ(HelpKeyCatalog::SYM_ENTRIES[0].symbol, '!');

    ASSERT_TRUE(HelpKeyCatalog::FN_COUNT >= 12);
    ASSERT_EQ(HelpKeyCatalog::FN_ENTRIES[0].key, 'Q');
    ASSERT_STR_EQ(HelpKeyCatalog::FN_ENTRIES[0].f_label, "F1");
    ASSERT_EQ(HelpKeyCatalog::FN_ENTRIES[11].key, 'S');
    ASSERT_STR_EQ(HelpKeyCatalog::FN_ENTRIES[11].f_label, "F12");

    return 0;
}

static int test_help_screen_state_and_navigation() {
    HelpScreen screen;
    ASSERT_FALSE(screen.active());

    screen.open();
    ASSERT_TRUE(screen.active());
    ASSERT_EQ(static_cast<uint8_t>(screen.state().page), static_cast<uint8_t>(HelpPage::Overview));

    screen.set_page(HelpPage::Keypad);
    ASSERT_EQ(static_cast<uint8_t>(screen.state().page), static_cast<uint8_t>(HelpPage::Keypad));

    screen.next_page();
    ASSERT_EQ(static_cast<uint8_t>(screen.state().page), static_cast<uint8_t>(HelpPage::Sym));

    screen.next_page();
    ASSERT_EQ(static_cast<uint8_t>(screen.state().page), static_cast<uint8_t>(HelpPage::Fn));

    screen.next_page(); // Wrap to 0
    ASSERT_EQ(static_cast<uint8_t>(screen.state().page), static_cast<uint8_t>(HelpPage::Overview));

    screen.prev_page(); // Wrap to 3
    ASSERT_EQ(static_cast<uint8_t>(screen.state().page), static_cast<uint8_t>(HelpPage::Fn));

    KeySnapshot snap{30, 'a', 0};
    screen.record_key(snap);
    ASSERT_EQ(screen.state().last_key.code, 30);
    ASSERT_EQ(screen.state().last_key.character, 'a');

    screen.close();
    ASSERT_FALSE(screen.active());

    return 0;
}

static int test_help_controller_routing_inactive() {
    HelpController ctrl;
    ModifierState mods{};

    ASSERT_FALSE(ctrl.active());

    // Regular key press passes through
    auto r1 = ctrl.before_terminal_write(make_ev(EV_KEY, 30, 1), mods, "a");
    ASSERT_EQ(static_cast<uint8_t>(r1), static_cast<uint8_t>(HelpRoute::Pass));
    ASSERT_FALSE(ctrl.active());

    // Menu key opens help immediately
    auto r_menu = ctrl.before_terminal_write(make_ev(EV_KEY, KeyCatalog::CODE_MENU, 1), mods, "");
    ASSERT_EQ(static_cast<uint8_t>(r_menu), static_cast<uint8_t>(HelpRoute::Redraw));
    ASSERT_TRUE(ctrl.active());

    ctrl.close();
    ASSERT_FALSE(ctrl.active());

    // Shift+H opens help
    mods.shift = true;
    auto r_sh = ctrl.before_terminal_write(make_ev(EV_KEY, KeyCatalog::CODE_H, 1), mods, "H");
    ASSERT_EQ(static_cast<uint8_t>(r_sh), static_cast<uint8_t>(HelpRoute::Redraw));
    ASSERT_TRUE(ctrl.active());

    ctrl.close();
    mods.shift = false;

    // Typing "help\r" in terminal opens help via after_terminal_write
    auto r_help = ctrl.after_terminal_write("help");
    ASSERT_EQ(static_cast<uint8_t>(r_help), static_cast<uint8_t>(HelpRoute::Pass));
    ASSERT_FALSE(ctrl.active());

    auto r_enter = ctrl.after_terminal_write("\r");
    ASSERT_EQ(static_cast<uint8_t>(r_enter), static_cast<uint8_t>(HelpRoute::Redraw));
    ASSERT_TRUE(ctrl.active());

    return 0;
}

static int test_help_controller_routing_active() {
    HelpController ctrl;
    ModifierState mods{};
    ctrl.open();
    ASSERT_TRUE(ctrl.active());

    // Tab navigation: '2' (Keypad)
    auto r2 = ctrl.before_terminal_write(make_ev(EV_KEY, 3, 1), mods, "2");
    ASSERT_EQ(static_cast<uint8_t>(r2), static_cast<uint8_t>(HelpRoute::Redraw));
    ASSERT_EQ(static_cast<uint8_t>(ctrl.screen().state().page), static_cast<uint8_t>(HelpPage::Keypad));

    // Tab navigation: '3' (Sym)
    auto r3 = ctrl.before_terminal_write(make_ev(EV_KEY, 4, 1), mods, "3");
    ASSERT_EQ(static_cast<uint8_t>(r3), static_cast<uint8_t>(HelpRoute::Redraw));
    ASSERT_EQ(static_cast<uint8_t>(ctrl.screen().state().page), static_cast<uint8_t>(HelpPage::Sym));

    // D-Pad Left -> Keypad
    auto r_left = ctrl.before_terminal_write(make_ev(EV_KEY, KeyCatalog::CODE_FIVEWAY_LEFT, 1), mods, "\033[D");
    ASSERT_EQ(static_cast<uint8_t>(r_left), static_cast<uint8_t>(HelpRoute::Redraw));
    ASSERT_EQ(static_cast<uint8_t>(ctrl.screen().state().page), static_cast<uint8_t>(HelpPage::Keypad));

    // D-Pad Right -> Sym
    auto r_right = ctrl.before_terminal_write(make_ev(EV_KEY, KeyCatalog::CODE_FIVEWAY_RIGHT, 1), mods, "\033[C");
    ASSERT_EQ(static_cast<uint8_t>(r_right), static_cast<uint8_t>(HelpRoute::Redraw));
    ASSERT_EQ(static_cast<uint8_t>(ctrl.screen().state().page), static_cast<uint8_t>(HelpPage::Sym));

    // Regular key press records key and triggers redraw
    auto r_key = ctrl.before_terminal_write(make_ev(EV_KEY, 30, 1), mods, "a");
    ASSERT_EQ(static_cast<uint8_t>(r_key), static_cast<uint8_t>(HelpRoute::Redraw));
    ASSERT_EQ(ctrl.screen().state().last_key.code, 30);
    ASSERT_EQ(ctrl.screen().state().last_key.character, 'a');

    // Exit triggers: 'q'
    auto r_q = ctrl.before_terminal_write(make_ev(EV_KEY, 16, 1), mods, "q");
    ASSERT_EQ(static_cast<uint8_t>(r_q), static_cast<uint8_t>(HelpRoute::Exit));
    ASSERT_FALSE(ctrl.active());

    // Reopen & exit via Back key
    ctrl.open();
    auto r_back = ctrl.before_terminal_write(make_ev(EV_KEY, KeyCatalog::CODE_BACK_K3, 1), mods, "");
    ASSERT_EQ(static_cast<uint8_t>(r_back), static_cast<uint8_t>(HelpRoute::Exit));
    ASSERT_FALSE(ctrl.active());

    // Reopen & exit via Enter key
    ctrl.open();
    auto r_ent = ctrl.before_terminal_write(make_ev(EV_KEY, KeyCatalog::CODE_ENTER, 1), mods, "\r");
    ASSERT_EQ(static_cast<uint8_t>(r_ent), static_cast<uint8_t>(HelpRoute::Exit));
    ASSERT_FALSE(ctrl.active());

    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_key_catalog_constants_and_helpers);
    RUN_TEST(test_help_types_and_state);
    RUN_TEST(test_help_key_catalog_layouts);
    RUN_TEST(test_help_screen_state_and_navigation);
    RUN_TEST(test_help_controller_routing_inactive);
    RUN_TEST(test_help_controller_routing_active);
TEST_MAIN_END()

