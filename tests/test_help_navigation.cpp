#include "tests/test_framework.h"
#include "input/key_catalog.hpp"
#include "help/help_types.hpp"
#include "help/help_key_catalog.hpp"

using namespace myts;
using namespace myts::input;
using namespace myts::help;

static int test_key_catalog_constants_and_helpers() {
    ASSERT_EQ(KeyCatalog::KEY_MENU, 139);
    ASSERT_EQ(KeyCatalog::KEY_BACK_K3, 158);
    ASSERT_EQ(KeyCatalog::KEY_BACK_DX, 91);
    ASSERT_EQ(KeyCatalog::KEY_PAGE_FORWARD, 109);
    ASSERT_EQ(KeyCatalog::KEY_PAGE_TURN_K3, 191);
    ASSERT_EQ(KeyCatalog::KEY_PAGE_TURN_DX, 124);
    ASSERT_EQ(KeyCatalog::KEY_SELECT_K3, 194);
    ASSERT_EQ(KeyCatalog::KEY_SELECT_DX, 92);

    ASSERT_TRUE(KeyCatalog::is_back_key(158));
    ASSERT_TRUE(KeyCatalog::is_back_key(91));
    ASSERT_FALSE(KeyCatalog::is_back_key(100));

    ASSERT_TRUE(KeyCatalog::is_menu_key(139));
    ASSERT_FALSE(KeyCatalog::is_menu_key(140));

    ASSERT_TRUE(KeyCatalog::is_select_key(194));
    ASSERT_TRUE(KeyCatalog::is_select_key(92));
    ASSERT_FALSE(KeyCatalog::is_select_key(28)); // Enter is not select

    ASSERT_TRUE(KeyCatalog::is_exit_trigger(KeyCatalog::KEY_BACK_K3));
    ASSERT_TRUE(KeyCatalog::is_exit_trigger(KeyCatalog::KEY_BACK_DX));
    ASSERT_TRUE(KeyCatalog::is_exit_trigger(KeyCatalog::KEY_MENU));
    ASSERT_TRUE(KeyCatalog::is_exit_trigger(KeyCatalog::KEY_PAGE_TURN_K3));

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
    // Check row dimensions
    ASSERT_EQ(HelpKeyCatalog::ROW1_COUNT, 10);
    ASSERT_EQ(HelpKeyCatalog::ROW2_COUNT, 10);
    ASSERT_EQ(HelpKeyCatalog::ROW3_COUNT, 10);

    // Check Row 1 keys
    ASSERT_EQ(HelpKeyCatalog::ROW1[0].primary, 'Q');
    ASSERT_EQ(HelpKeyCatalog::ROW1[0].shift, '!');
    ASSERT_EQ(HelpKeyCatalog::ROW1[9].primary, 'P');
    ASSERT_EQ(HelpKeyCatalog::ROW1[9].shift, ')');

    // Check Row 2 keys
    ASSERT_EQ(HelpKeyCatalog::ROW2[0].primary, 'A');
    ASSERT_EQ(HelpKeyCatalog::ROW2[9].primary, '<'); // Del / Backspace

    // Check Row 3 keys
    ASSERT_EQ(HelpKeyCatalog::ROW3[0].primary, 'Z');
    ASSERT_EQ(HelpKeyCatalog::ROW3[9].primary, '\r'); // Enter

    // Check Sym layer symbols
    ASSERT_TRUE(HelpKeyCatalog::SYM_COUNT >= 20);
    ASSERT_EQ(HelpKeyCatalog::SYM_ENTRIES[0].key, 'Q');
    ASSERT_EQ(HelpKeyCatalog::SYM_ENTRIES[0].symbol, '!');

    // Check Fn layer mappings
    ASSERT_TRUE(HelpKeyCatalog::FN_COUNT >= 12);
    ASSERT_EQ(HelpKeyCatalog::FN_ENTRIES[0].key, 'Q');
    ASSERT_STR_EQ(HelpKeyCatalog::FN_ENTRIES[0].f_label, "F1");
    ASSERT_EQ(HelpKeyCatalog::FN_ENTRIES[11].key, 'S');
    ASSERT_STR_EQ(HelpKeyCatalog::FN_ENTRIES[11].f_label, "F12");

    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_key_catalog_constants_and_helpers);
    RUN_TEST(test_help_types_and_state);
    RUN_TEST(test_help_key_catalog_layouts);
TEST_MAIN_END()
