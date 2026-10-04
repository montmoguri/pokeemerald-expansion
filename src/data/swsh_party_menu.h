enum {
    BUTTON_START,
    BUTTON_SELECT,
    BUTTON_L,
    BUTTON_R,
    BUTTON_NONE = 0xFF,
};

static const u32 sPartyMenuBg_Gfx[]             = INCGFX_U32("graphics/party_menu/swsh/tiles.png", ".4bpp.smol");
static const u16 sPartyMenuBg_Pal[]             = INCGFX_U16("graphics/party_menu/swsh/tiles.png", ".gbapal");
static const u32 sPartyMenuBg_Main_Tilemap[]    = INCGFX_U32("graphics/party_menu/swsh/bg_main.bin", ".smolTM");
static const u32 sPartyMenuBg_Scroll_Tilemap[]  = INCGFX_U32("graphics/party_menu/swsh/bg_scroll.bin", ".smolTM");

static const u32 sHeldItem_Gfx[]                = INCGFX_U32("graphics/party_menu/swsh/hold_icons.png", ".4bpp");
static const u32 sMoveTypes_Gfx[]               = INCGFX_U32("graphics/party_menu/swsh/move_types.png", ".4bpp.smol");
static const u32 sMessageWindow_Gfx[]           = INCGFX_U32("graphics/party_menu/swsh/message_window.png", ".4bpp.smol");
static const u32 sSelectFrame_Gfx[]             = INCGFX_U32("graphics/party_menu/swsh/select_frame.png", ".4bpp.smol");

static const u16 sHeldItem_Pal[]                = INCGFX_U16("graphics/party_menu/swsh/hold_icons.png", ".gbapal");

static const u8 sButtons_Gfx[][4 * TILE_SIZE_4BPP] = {
    [BUTTON_START]  = INCGFX_U8("graphics/party_menu/swsh/button_start.png", ".4bpp"),
    [BUTTON_SELECT] = INCGFX_U8("graphics/party_menu/swsh/button_select.png", ".4bpp"),
    [BUTTON_L]      = INCGFX_U8("graphics/party_menu/swsh/button_l.png", ".4bpp"),
    [BUTTON_R]      = INCGFX_U8("graphics/party_menu/swsh/button_r.png", ".4bpp"),
};

// For global use, specifically dexnav
const u32 gHeldItemGfx[]        = INCGFX_U32("graphics/party_menu/hold_icons.png", ".4bpp");
const u16 gHeldItemPalette[]    = INCGFX_U16("graphics/party_menu/hold_icons.png", ".gbapal");

static const u8 sText_EggNickname[POKEMON_NAME_LENGTH + 1]  = _("Egg");
static const u8 sMenuText_Confirm[]                         = _("Confirm");
static const u8 sMenuText_Switch[]                          = _("Switch");
static const u8 sMenuText_Boxes[]                           = _("Boxes");
static const u8 sMenuText_ViewPartnerParty[]                = _("{STR_VAR_1}'s party");
static const u8 sMenuText_ViewPlayerParty[]                 = _("Your party");
static const u8 sText_SendThisMonToPC[]                     = _("Send {STR_VAR_1} to the PC?");

static const struct BgTemplate sPartyMenuBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 0,
        .mapBaseIndex = 28,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
    {
        .bg = 3,
        .charBaseIndex = 0,
        .mapBaseIndex = 27,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    },
};

static const struct
{
    struct PartyBoxRect nickname;
    struct PartyBoxRect level;
    struct PartyBoxRect gender;
    struct PartyBoxRect hp;
    struct PartyBoxRect hpBar;
    struct PartyBoxRect descText;
} sPartySlotLayout =
{
    .nickname = { 32,  0, 48, 13 },
    .level    = { 80, 11, 32,  8 },
    .gender   = { 91,  0,  8,  8 },
    .hp       = { 32, 11, 48,  8 },
    .hpBar    = { 32, 12, 64,  2 },
    .descText = { 32, 11, 64, 12 },
};


// Geometry of a party slot. Every layout is a single column with a fixed vertical
// stride, so slot s sits at y = PARTY_SLOT_TOP + PARTY_SLOT_STRIDE * s. Within a slot the
// held-item and status sprites sit at constant offsets from the mon icon, so a slot is
// fully described by its icon's x and the slot index. Only the icon's x varies between
// layouts: a couple of slots shift one tile left in double/multi battles to clear the
// enemy/partner mons. (Vanilla's 4th "menu Poké Ball" column is unused in SwSh; dropped.)
#define PARTY_SLOT_TOP          18
#define PARTY_SLOT_STRIDE       24
#define PARTY_ITEM_OFFSET_X      6
#define PARTY_ITEM_OFFSET_Y     10
#define PARTY_STATUS_OFFSET_X   73
#define PARTY_STATUS_OFFSET_Y    9
#define PARTY_ICON_X            34
#define PARTY_ICON_X_SHIFTED    26  // one tile left, to clear the enemy/partner mons

#define PARTY_SLOT_Y(s)  (PARTY_SLOT_TOP + PARTY_SLOT_STRIDE * (s))

// HBlank swaps PARTY_ITEM_PAL_COUNT palettes to cover all six item icons,
// parking the rest in unused BG palettes
// Credit: Greenphx9 and aarant/merrp icons-expansion branch
#define PARTY_ITEM_ICON_TOP(s)          (PARTY_SLOT_Y(s) + PARTY_ITEM_OFFSET_Y - 16)
#define PARTY_ITEM_PAL_SWAP_VCOUNT(s)   (PARTY_ITEM_ICON_TOP(s) - 8)

STATIC_ASSERT(PARTY_ITEM_ICON_TOP(0) + 32 <= PARTY_ITEM_ICON_TOP(PARTY_ITEM_PAL_COUNT), PartyItemIconsShareScanlines);

static const u8 sHeldItemIconParkPalNums[PARTY_ITEM_PARK_PAL_COUNT] = { 9, 10, 11 };

// One slot: mon icon (x,y), held item (x,y), status (x,y) — all derived from the icon origin.
#define PARTY_SLOT_COORDS(iconX, s)                                          \
{                                                                            \
    (iconX),                         PARTY_SLOT_Y(s),                        \
    (iconX) + PARTY_ITEM_OFFSET_X,   PARTY_SLOT_Y(s) + PARTY_ITEM_OFFSET_Y,  \
    (iconX) + PARTY_STATUS_OFFSET_X, PARTY_SLOT_Y(s) + PARTY_STATUS_OFFSET_Y \
}

#define PARTY_COORDS_SINGLE                        \
{                                                  \
    PARTY_SLOT_COORDS(PARTY_ICON_X, 0),            \
    PARTY_SLOT_COORDS(PARTY_ICON_X, 1),            \
    PARTY_SLOT_COORDS(PARTY_ICON_X, 2),            \
    PARTY_SLOT_COORDS(PARTY_ICON_X, 3),            \
    PARTY_SLOT_COORDS(PARTY_ICON_X, 4),            \
    PARTY_SLOT_COORDS(PARTY_ICON_X, 5),            \
}

#define PARTY_COORDS_DOUBLE                        \
{                                                  \
    PARTY_SLOT_COORDS(PARTY_ICON_X_SHIFTED, 0),    \
    PARTY_SLOT_COORDS(PARTY_ICON_X_SHIFTED, 1),    \
    PARTY_SLOT_COORDS(PARTY_ICON_X, 2),            \
    PARTY_SLOT_COORDS(PARTY_ICON_X, 3),            \
    PARTY_SLOT_COORDS(PARTY_ICON_X, 4),            \
    PARTY_SLOT_COORDS(PARTY_ICON_X, 5),            \
}

#define PARTY_COORDS_MULTI                         \
{                                                  \
    PARTY_SLOT_COORDS(PARTY_ICON_X_SHIFTED, 0),    \
    PARTY_SLOT_COORDS(PARTY_ICON_X, 1),            \
    PARTY_SLOT_COORDS(PARTY_ICON_X, 2),            \
    PARTY_SLOT_COORDS(PARTY_ICON_X_SHIFTED, 3),    \
    PARTY_SLOT_COORDS(PARTY_ICON_X, 4),            \
    PARTY_SLOT_COORDS(PARTY_ICON_X, 5),            \
}

static const u8 sPartyMenuSpriteCoords[PARTY_LAYOUT_COUNT][PARTY_SIZE][3 * 2] =
{
    [PARTY_LAYOUT_SINGLE]                       = PARTY_COORDS_SINGLE,
    [PARTY_LAYOUT_DOUBLE]                       = PARTY_COORDS_DOUBLE,
    [PARTY_LAYOUT_MULTI]                        = PARTY_COORDS_MULTI,
    [PARTY_LAYOUT_MULTI_SHOWCASE]               = PARTY_COORDS_MULTI,
    [PARTY_LAYOUT_MULTI_FULL]                   = PARTY_COORDS_SINGLE,
    [PARTY_LAYOUT_MULTI_FULL_PARTNER]           = PARTY_COORDS_SINGLE,
    [PARTY_LAYOUT_MULTI_FULL_SHOWCASE]          = PARTY_COORDS_SINGLE,
    [PARTY_LAYOUT_MULTI_FULL_SHOWCASE_PARTNER]  = PARTY_COORDS_SINGLE,
};

static const struct
{
    struct PartyBoxRect moveName;
    struct PartyBoxRect pp;
} sPartyMoveBoxLayout =
{
    .moveName = { 10, 1, 62, 12 },
    .pp       = { 86, 1, 10, 12 },
};

// Text colors for BG, FG, and Shadow in that order
static const u8 sFontColorTable[][3] =
{
    {0,  3,  2},  // Default
    {0, 11, 12},  // Gender symbol
    {1,  2,  3},  // Selection actions
    {1,  8,  9},  // Field moves
    {0,  1,  2},  // Button prompt text
    {0,  5,  6},  // PP state 0 (yellow)
    {0,  7,  8},  // PP state 1 (orange)
    {0,  9, 10},  // PP state 2 (red)
    {0,  3,  4},  // PP state 3 (white)
    {0,  1,  2},  // Ability name, move names
};

#define PARTY_CHAR_BASE_TILES       1024

#define PARTY_TILES_BG_GFX          128
#define PARTY_TILES_MSGBOX          14
#define PARTY_TILES_STD_BORDER      9

#define PARTY_BASE_MSGBOX           PARTY_TILES_BG_GFX
#define PARTY_BASE_STD_BORDER       (PARTY_BASE_MSGBOX + PARTY_TILES_MSGBOX)

#define WIN_PARTY_SLOT_W            14
#define WIN_PARTY_SLOT_H            3
#define WIN_PARTY_SLOT_TILES        (WIN_PARTY_SLOT_W * WIN_PARTY_SLOT_H)
#define WIN_PARTY_SLOT_BASE         (PARTY_BASE_STD_BORDER + PARTY_TILES_STD_BORDER)
#define WIN_PARTY_SLOT(s)           (WIN_PARTY_SLOT_BASE + (s) * WIN_PARTY_SLOT_TILES)

#define WIN_MSG_W                   28
#define WIN_MSG_H                   4
#define WIN_MSG_TILES               (WIN_MSG_W * WIN_MSG_H)
#define WIN_MSG_BASE                WIN_PARTY_SLOT(PARTY_SIZE)

#define WIN_PROMPT_W                14
#define WIN_PROMPT_H                2
#define WIN_PROMPT_TILES            (WIN_PROMPT_W * WIN_PROMPT_H)
#define WIN_PROMPT_BASE             (WIN_MSG_BASE + WIN_MSG_TILES)

#define PARTY_SLOT_WIN(s, left)                     \
    {                                               \
        .bg          = 1,                           \
        .tilemapLeft = (left),                      \
        .tilemapTop  = 1 + (s) * WIN_PARTY_SLOT_H,  \
        .width       = WIN_PARTY_SLOT_W,            \
        .height      = WIN_PARTY_SLOT_H,            \
        .paletteNum  = 3 + (s),                     \
        .baseBlock   = WIN_PARTY_SLOT(s),           \
    }

#define PARTY_WIN_MSG                               \
    [WIN_MSG] = {                                   \
        .bg          = 0,                           \
        .tilemapLeft = 1,                           \
        .tilemapTop  = 15,                          \
        .width       = WIN_MSG_W,                   \
        .height      = WIN_MSG_H,                   \
        .paletteNum  = 14,                          \
        .baseBlock   = WIN_MSG_BASE,                \
    }

static const struct WindowTemplate sSinglePartyMenuWindowTemplate_SwSh[] =
{
    PARTY_SLOT_WIN(0, 2),
    PARTY_SLOT_WIN(1, 2),
    PARTY_SLOT_WIN(2, 2),
    PARTY_SLOT_WIN(3, 2),
    PARTY_SLOT_WIN(4, 2),
    PARTY_SLOT_WIN(5, 2),
    PARTY_WIN_MSG,
    [PARTY_LABEL_WINDOW_PROMPT] = {
        .bg = 1,
        .tilemapLeft = 16,
        .tilemapTop = 18,
        .width = WIN_PROMPT_W,
        .height = WIN_PROMPT_H,
        .paletteNum = 0,
        .baseBlock = WIN_PROMPT_BASE,
    },
    DUMMY_WIN_TEMPLATE
};

static const struct WindowTemplate sDoublePartyMenuWindowTemplate_SwSh[] =
{
    PARTY_SLOT_WIN(0, 1),
    PARTY_SLOT_WIN(1, 1),
    PARTY_SLOT_WIN(2, 2),
    PARTY_SLOT_WIN(3, 2),
    PARTY_SLOT_WIN(4, 2),
    PARTY_SLOT_WIN(5, 2),
    PARTY_WIN_MSG,
    DUMMY_WIN_TEMPLATE
};

// party slots 0 and 3 are the leading mons
static const struct WindowTemplate sMultiPartyMenuWindowTemplate_SwSh[] =
{
    PARTY_SLOT_WIN(0, 1),
    PARTY_SLOT_WIN(1, 2),
    PARTY_SLOT_WIN(2, 2),
    PARTY_SLOT_WIN(3, 1),
    PARTY_SLOT_WIN(4, 2),
    PARTY_SLOT_WIN(5, 2),
    PARTY_WIN_MSG,
    DUMMY_WIN_TEMPLATE
};

static const struct WindowTemplate sShowcaseMultiPartyMenuWindowTemplate_SwSh[] =
{
    PARTY_SLOT_WIN(0, 1),
    PARTY_SLOT_WIN(1, 2),
    PARTY_SLOT_WIN(2, 2),
    PARTY_SLOT_WIN(3, 1),
    PARTY_SLOT_WIN(4, 2),
    PARTY_SLOT_WIN(5, 2),
    DUMMY_WIN_TEMPLATE
};

// mont note: only one of these show at a time
#define WIN_POPUP_BASE              (WIN_PROMPT_BASE + WIN_PROMPT_TILES)
#define WIN_POPUP_TILES             168

// WIN_ACTIONS_W/_H/_TILES handled in swsh_party_menu.c with MAX_PARTY_MENU_ACTIONS
#define WIN_ITEM_GIVE_TAKE_W        6
#define WIN_ITEM_GIVE_TAKE_H        8
#define WIN_ITEM_GIVE_TAKE_TILES    (WIN_ITEM_GIVE_TAKE_W * WIN_ITEM_GIVE_TAKE_H)

#define WIN_MAIL_READ_TAKE_W        8
#define WIN_MAIL_READ_TAKE_H        6
#define WIN_MAIL_READ_TAKE_TILES    (WIN_MAIL_READ_TAKE_W * WIN_MAIL_READ_TAKE_H)

#define WIN_MOVE_SELECT_W           10
#define WIN_MOVE_SELECT_H           8
#define WIN_MOVE_SELECT_TILES       (WIN_MOVE_SELECT_W * WIN_MOVE_SELECT_H)

#define WIN_CATALOG_W               12
#define WIN_CATALOG_H               14
#define WIN_CATALOG_TILES           (WIN_CATALOG_W * WIN_CATALOG_H)

#define WIN_ZYGARDE_CUBE_W          11
#define WIN_ZYGARDE_CUBE_H          6
#define WIN_ZYGARDE_CUBE_TILES      (WIN_ZYGARDE_CUBE_W * WIN_ZYGARDE_CUBE_H)

#define WIN_LEVEL_UP_STATS_W        10
#define WIN_LEVEL_UP_STATS_H        11
#define WIN_LEVEL_UP_STATS_TILES    (WIN_LEVEL_UP_STATS_W * WIN_LEVEL_UP_STATS_H)

#define WIN_OAK_W                   27
#define WIN_OAK_H                   4
#define WIN_OAK_TILES               (WIN_OAK_W * WIN_OAK_H)

#define WIN_YESNO_W                 5
#define WIN_YESNO_H                 4
#define WIN_YESNO_TILES             (WIN_YESNO_W * WIN_YESNO_H)
#define WIN_YESNO_BASE              (WIN_POPUP_BASE + WIN_POPUP_TILES)

// mont note: the commented names are for tracking equivalent vanilla party_menu.c win templates
static const struct WindowTemplate sPartyMenuWindowTemplates[] =
{
    [PARTYWIN_ACTIONS] = { // see DisplaySelectionWindow
        .bg = 0,
        .tilemapLeft = 19,
        .tilemapTop = PARTY_ACTIONS_BOTTOM_ROW - WIN_ACTIONS_H,
        .width = WIN_ACTIONS_W,
        .height = WIN_ACTIONS_H,
        .paletteNum = 14,
        .baseBlock = WIN_POPUP_BASE,
    },
    [PARTYWIN_ITEM] = { // sItemGiveTakeWindowTemplate
        .bg = 0,
        .tilemapLeft = 23,
        .tilemapTop = 11,
        .width = WIN_ITEM_GIVE_TAKE_W,
        .height = WIN_ITEM_GIVE_TAKE_H,
        .paletteNum = 14,
        .baseBlock = WIN_POPUP_BASE,
    },
    [PARTYWIN_MAIL] = { // sMailReadTakeWindowTemplate
        .bg = 0,
        .tilemapLeft = 21,
        .tilemapTop = 13,
        .width = WIN_MAIL_READ_TAKE_W,
        .height = WIN_MAIL_READ_TAKE_H,
        .paletteNum = 14,
        .baseBlock = WIN_POPUP_BASE,
    },
    [PARTYWIN_MOVES] = { // sMoveSelectWindowTemplate
        .bg = 0,
        .tilemapLeft = 19,
        .tilemapTop = 11,
        .width = WIN_MOVE_SELECT_W,
        .height = WIN_MOVE_SELECT_H,
        .paletteNum = 14,
        .baseBlock = WIN_POPUP_BASE,
    },
    [PARTYWIN_CATALOG] = { // sCatalogSelectWindowTemplate
        .bg = 0,
        .tilemapLeft = 17,
        .tilemapTop = 5,
        .width = WIN_CATALOG_W,
        .height = WIN_CATALOG_H,
        .paletteNum = 14,
        .baseBlock = WIN_POPUP_BASE,
    },
    [PARTYWIN_ZYGARDE_CUBE] = { // sZygardeCubeSelectWindowTemplate
        .bg = 0,
        .tilemapLeft = 18,
        .tilemapTop = 13,
        .width = WIN_ZYGARDE_CUBE_W,
        .height = WIN_ZYGARDE_CUBE_H,
        .paletteNum = 14,
        .baseBlock = WIN_POPUP_BASE,
    },
    [PARTYWIN_LEVEL_UP_STATS] = { // sLevelUpStatsWindowTemplate
        .bg = 0,
        .tilemapLeft = 19,
        .tilemapTop = 1,
        .width = WIN_LEVEL_UP_STATS_W,
        .height = WIN_LEVEL_UP_STATS_H,
        .paletteNum = 14,
        .baseBlock = WIN_POPUP_BASE,
    },
    [PARTYWIN_OAK_VOICEOVER] = { // sWindowTemplate_FirstBattleOakVoiceover
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = WIN_OAK_W,
        .height = WIN_OAK_H,
        .paletteNum = 14,
        .baseBlock = WIN_POPUP_BASE,
    },
    [PARTYWIN_YESNO] = { // sPartyMenuYesNoWindowTemplate
        .bg = 0,
        .tilemapLeft = 21,
        .tilemapTop = 9,
        .width = WIN_YESNO_W,
        .height = WIN_YESNO_H,
        .paletteNum = 14,
        .baseBlock = WIN_YESNO_BASE,
    },
};

STATIC_ASSERT(ARRAY_COUNT(sPartyMenuWindowTemplates) == PARTYWIN_COUNT, PartyWindowTemplateCount);
STATIC_ASSERT(WIN_ACTIONS_TILES <= WIN_POPUP_TILES, PartyPopupActions);
STATIC_ASSERT(WIN_ITEM_GIVE_TAKE_TILES <= WIN_POPUP_TILES, PartyPopupItemGiveTake);
STATIC_ASSERT(WIN_MAIL_READ_TAKE_TILES <= WIN_POPUP_TILES, PartyPopupMailReadTake);
STATIC_ASSERT(WIN_MOVE_SELECT_TILES <= WIN_POPUP_TILES, PartyPopupMoveSelect);
STATIC_ASSERT(WIN_CATALOG_TILES <= WIN_POPUP_TILES, PartyPopupCatalog);
STATIC_ASSERT(WIN_ZYGARDE_CUBE_TILES <= WIN_POPUP_TILES, PartyPopupZygardeCube);
STATIC_ASSERT(WIN_LEVEL_UP_STATS_TILES <= WIN_POPUP_TILES, PartyPopupLevelUpStats);
STATIC_ASSERT(WIN_OAK_TILES <= WIN_POPUP_TILES, PartyPopupOak);

// static const struct WindowTemplate sDoWhatWithMonMsgWindowTemplate =
// {
//     .bg = 2,
//     .tilemapLeft = 1,
//     .tilemapTop = 17,
//     .width = 16,
//     .height = 2,
//     .paletteNum = 15,
//     .baseBlock = 0x285,
// };

// static const struct WindowTemplate sDoWhatWithItemMsgWindowTemplate =
// {
//     .bg = 2,
//     .tilemapLeft = 1,
//     .tilemapTop = 17,
//     .width = 20,
//     .height = 2,
//     .paletteNum = 15,
//     .baseBlock = 0x299,
// };

// static const struct WindowTemplate sDoWhatWithMailMsgWindowTemplate =
// {
//     .bg = 2,
//     .tilemapLeft = 1,
//     .tilemapTop = 17,
//     .width = 18,
//     .height = 2,
//     .paletteNum = 15,
//     .baseBlock = 0x299,
// };

// static const struct WindowTemplate sWhichMoveMsgWindowTemplate =
// {
//     .bg = 2,
//     .tilemapLeft = 1,
//     .tilemapTop = 17,
//     .width = 16,
//     .height = 2,
//     .paletteNum = 15,
//     .baseBlock = 0x299,
// };

#define WIN_IN_BATTLE_MOVE_W        14
#define WIN_IN_BATTLE_MOVE_H        2
#define WIN_IN_BATTLE_MOVE_TILES    (WIN_IN_BATTLE_MOVE_W * WIN_IN_BATTLE_MOVE_H)
#define WIN_IN_BATTLE_MOVE_BASE     (WIN_YESNO_BASE + WIN_YESNO_TILES)
#define WIN_IN_BATTLE_MOVE(m)       (WIN_IN_BATTLE_MOVE_BASE + (m) * WIN_IN_BATTLE_MOVE_TILES)

#define PARTY_IN_BATTLE_MOVE_WIN(m)                     \
    {                                                   \
        .bg          = 1,                               \
        .tilemapLeft = 16,                              \
        .tilemapTop  = 2 + (m) * WIN_IN_BATTLE_MOVE_H,  \
        .width       = WIN_IN_BATTLE_MOVE_W,            \
        .height      = WIN_IN_BATTLE_MOVE_H,            \
        .paletteNum  = 1,                               \
        .baseBlock   = WIN_IN_BATTLE_MOVE(m),           \
    }

static const struct WindowTemplate sMoveInfoWindowTemplate_SwSh[] =
{
    PARTY_IN_BATTLE_MOVE_WIN(0),
    PARTY_IN_BATTLE_MOVE_WIN(1),
    PARTY_IN_BATTLE_MOVE_WIN(2),
    PARTY_IN_BATTLE_MOVE_WIN(3),
};

STATIC_ASSERT(ARRAY_COUNT(sMoveInfoWindowTemplate_SwSh) == MAX_MON_MOVES, PartyMoveInfoWindowCount);

#define WIN_IN_BATTLE_ABILITY_W         13
#define WIN_IN_BATTLE_ABILITY_H         4
#define WIN_IN_BATTLE_ABILITY_TILES     (WIN_IN_BATTLE_ABILITY_W * WIN_IN_BATTLE_ABILITY_H)
#define WIN_IN_BATTLE_ABILITY_BASE      WIN_IN_BATTLE_MOVE(MAX_MON_MOVES)

static const struct WindowTemplate sAbilityInfoWindowTemplate =
{
    .bg = 1,
    .tilemapLeft = 17,
    .tilemapTop = 11,
    .width = WIN_IN_BATTLE_ABILITY_W,
    .height = WIN_IN_BATTLE_ABILITY_H,
    .paletteNum = 1,
    .baseBlock = WIN_IN_BATTLE_ABILITY_BASE,
};

#define PARTY_TILES_END                 (WIN_IN_BATTLE_ABILITY_BASE + WIN_IN_BATTLE_ABILITY_TILES)

STATIC_ASSERT(PARTY_TILES_END <= PARTY_CHAR_BASE_TILES, PartyMenuCharBaseOverflow);

// Plain tilemaps for party menu slots.
static const u8 sSlotTilemap_Main_SwSh[]  = INCBIN_U8("graphics/party_menu/swsh/slot.bin");
static const u8 sSlotTilemap_Empty_SwSh[] = INCBIN_U8("graphics/party_menu/swsh/slot_empty.bin");

// Plain tilemaps for move slots
static const u8 sMoveTilemap_Main_SwSh[]  = INCBIN_U8("graphics/party_menu/swsh/move.bin");
static const u8 sMoveTilemap_Empty_SwSh[] = INCBIN_U8("graphics/party_menu/swsh/move_empty.bin");
static const u8 sAbilityTilemap_SwSh[]    = INCBIN_U8("graphics/party_menu/swsh/ability_box.bin");

// Palette offsets
static const u8 sGenderPalOffsets[]     = {11, 12};
static const u8 sHPBarPalOffset         = 9;
static const u8 sPartyBoxPalOffset1     = 1;
static const u8 sPartyBoxPalOffsets3[]  = {2, 3};

// Palette ids
static const u8 sGenderMalePalIds[]               = {11, 12};
static const u8 sGenderFemalePalIds[]             = {13, 14};
static const u8 sHPBarPalIds[]                    = {7, 8, 9};
static const u8 sHPBarEmptyPalId                  = 10;

// party slot state coloring lives in sPartyMenuBg_Pal as one 16-color pal per state
// the frame color is the pal index 1, the two text colors follow
#define PARTY_BOX_PAL_BANK_SIZE 16
#define PARTY_BOX_PAL_BASE(n)   (PARTY_BOX_PAL_BANK_SIZE * ((n) + 1) + 1)

enum PartyBoxPalState
{
    PARTY_BOX_PAL_NO_MON,
    PARTY_BOX_PAL_NORMAL,
    PARTY_BOX_PAL_MULTI,
    PARTY_BOX_PAL_SELECTED_FOR_ACTION,
    PARTY_BOX_PAL_CURR_SELECTION,
    PARTY_BOX_PAL_CURR_SELECTION_MULTI,
    PARTY_BOX_PAL_STATE_COUNT,
};

struct PartyBoxPalIds
{
    u8 frame;
    u8 text[2];
};

static const struct PartyBoxPalIds sPartyBoxPalIds[PARTY_BOX_PAL_STATE_COUNT] =
{
    [PARTY_BOX_PAL_NO_MON]               = { PARTY_BOX_PAL_BASE(0) },
    [PARTY_BOX_PAL_NORMAL]               = { PARTY_BOX_PAL_BASE(1), { PARTY_BOX_PAL_BASE(1) + 1, PARTY_BOX_PAL_BASE(1) + 2 } },
    [PARTY_BOX_PAL_MULTI]                = { PARTY_BOX_PAL_BASE(2), { PARTY_BOX_PAL_BASE(2) + 1, PARTY_BOX_PAL_BASE(2) + 2 } },
    [PARTY_BOX_PAL_SELECTED_FOR_ACTION]  = { PARTY_BOX_PAL_BASE(3), { PARTY_BOX_PAL_BASE(3) + 1, PARTY_BOX_PAL_BASE(3) + 2 } },
    [PARTY_BOX_PAL_CURR_SELECTION]       = { PARTY_BOX_PAL_BASE(4), { PARTY_BOX_PAL_BASE(4) + 1, PARTY_BOX_PAL_BASE(4) + 2 } },
    [PARTY_BOX_PAL_CURR_SELECTION_MULTI] = { PARTY_BOX_PAL_BASE(5), { PARTY_BOX_PAL_BASE(5) + 1, PARTY_BOX_PAL_BASE(5) + 2 } },
};

static const u8 *const sActionStringTable[] =
{
    [PARTY_MSG_CHOOSE_MON]             = gText_ChoosePokemon,
    [PARTY_MSG_CHOOSE_MON_OR_CANCEL]   = gText_ChoosePokemonCancel,
    [PARTY_MSG_CHOOSE_MON_AND_CONFIRM] = gText_ChoosePokemonConfirm,
    [PARTY_MSG_MOVE_TO_WHERE]          = gText_MoveToWhere,
    [PARTY_MSG_TEACH_WHICH_MON]        = gText_TeachWhichPokemon,
    [PARTY_MSG_USE_ON_WHICH_MON]       = gText_UseOnWhichPokemon,
    [PARTY_MSG_GIVE_TO_WHICH_MON]      = gText_GiveToWhichPokemon,
    [PARTY_MSG_NOTHING_TO_CUT]         = gText_NothingToCut,
    [PARTY_MSG_CANT_SURF_HERE]         = gText_CantSurfHere,
    [PARTY_MSG_ALREADY_SURFING]        = gText_AlreadySurfing,
    [PARTY_MSG_CURRENT_TOO_FAST]       = gText_CurrentIsTooFast,
    [PARTY_MSG_ENJOY_CYCLING]          = gText_EnjoyCycling,
    [PARTY_MSG_ALREADY_IN_USE]         = gText_InUseAlready_PM,
    [PARTY_MSG_CANT_USE_HERE]          = gText_CantUseHere,
    [PARTY_MSG_NO_MON_FOR_BATTLE]      = gText_NoPokemonForBattle,
    [PARTY_MSG_CHOOSE_MON_2]           = gText_ChoosePokemon2,
    [PARTY_MSG_NOT_ENOUGH_HP]          = gText_NotEnoughHp,
    [PARTY_MSG_X_MONS_ARE_NEEDED]      = gText_PokemonAreNeeded,
    [PARTY_MSG_MONS_CANT_BE_SAME]      = gText_PokemonCantBeSame, //TODO: regular dialog
    [PARTY_MSG_NO_SAME_HOLD_ITEMS]     = gText_NoIdenticalHoldItems, //TODO: regular dialog
    [PARTY_MSG_UNUSED]                 = gText_EmptyString2,
    [PARTY_MSG_DO_WHAT_WITH_MON]       = gText_DoWhatWithPokemon,
    [PARTY_MSG_RESTORE_WHICH_MOVE]     = gText_RestoreWhichMove, //TODO: set cursor on move slots
    [PARTY_MSG_BOOST_PP_WHICH_MOVE]    = gText_BoostPP,
    [PARTY_MSG_DO_WHAT_WITH_ITEM]      = gText_DoWhatWithItem,
    [PARTY_MSG_DO_WHAT_WITH_MAIL]      = gText_DoWhatWithMail,
    [PARTY_MSG_ALREADY_HOLDING_ONE]    = gText_AlreadyHoldingOne,
    [PARTY_MSG_WHICH_APPLIANCE]        = gText_WhichAppliance,
    [PARTY_MSG_CHOOSE_SECOND_FUSION]   = gText_NextFusionMon,
    [PARTY_MSG_NO_POKEMON]             = COMPOUND_STRING("You have no POKéMON."),
    [PARTY_MSG_CHOOSE_MON_FOR_BOX]     = gText_SendWhichMonToPC,
    [PARTY_MSG_SEND_MON_TO_BOX]        = sText_SendThisMonToPC,
    [PARTY_MSG_MOVE_ITEM_WHERE]        = gText_MoveItemWhere,
};

static const u8 sText_NoUse[] = _("NO USE");

static const u8 *const sDescriptionStringTable[] =
{
    [PARTYBOX_DESC_NO_USE]     = sText_NoUse,
    [PARTYBOX_DESC_ABLE_3]     = gText_Able,
    [PARTYBOX_DESC_FIRST]      = gText_First_PM,
    [PARTYBOX_DESC_SECOND]     = gText_Second_PM,
    [PARTYBOX_DESC_THIRD]      = gText_Third_PM,
    [PARTYBOX_DESC_FOURTH]     = gText_Fourth,
    [PARTYBOX_DESC_ABLE]       = gText_Able2,
    [PARTYBOX_DESC_NOT_ABLE]   = gText_NotAble,
    [PARTYBOX_DESC_ABLE_2]     = gText_Able3,
    [PARTYBOX_DESC_NOT_ABLE_2] = gText_NotAble2,
    [PARTYBOX_DESC_LEARNED]    = gText_Learned,
    [PARTYBOX_DESC_HAVE]       = gText_Have,
    [PARTYBOX_DESC_DONT_HAVE]  = gText_DontHave,
};

static const u16 sUnusedData[] =
{
    0x0108, 0x0151, 0x0160, 0x015b, 0x002e, 0x005c, 0x0102, 0x0153, 0x014b, 0x00ed, 0x00f1, 0x010d, 0x003a, 0x003b, 0x003f, 0x0071,
    0x00b6, 0x00f0, 0x00ca, 0x00db, 0x00da, 0x004c, 0x00e7, 0x0055, 0x0057, 0x0059, 0x00d8, 0x005b, 0x005e, 0x00f7, 0x0118, 0x0068,
    0x0073, 0x015f, 0x0035, 0x00bc, 0x00c9, 0x007e, 0x013d, 0x014c, 0x0103, 0x0107, 0x0122, 0x009c, 0x00d5, 0x00a8, 0x00d3, 0x011d,
    0x0121, 0x013b, 0x000f, 0x0013, 0x0039, 0x0046, 0x0094, 0x00f9, 0x007f, 0x0123,
};

static const u8 sText_Trade4[] = _("TRADE");

struct
{
    const u8 *text;
    TaskFunc func;
} static const sCursorOptions[MENU_FIELD_MOVES] =
{
    [MENU_SUMMARY]         = {COMPOUND_STRING("Summary"),         CursorCb_Summary},
    [MENU_SWITCH]          = {COMPOUND_STRING("Switch"),          CursorCb_Switch},
    [MENU_CANCEL1]         = {COMPOUND_STRING("Cancel"),          CursorCb_Cancel1},
    [MENU_ITEM]            = {COMPOUND_STRING("Item"),            CursorCb_Item},
    [MENU_GIVE]            = {COMPOUND_STRING("Give"),            CursorCb_Give},
    [MENU_TAKE_ITEM]       = {COMPOUND_STRING("Take"),            CursorCb_TakeItem},
    [MENU_MOVE_ITEM]       = {COMPOUND_STRING("Move"),            CursorCb_MoveItem},
    [MENU_MAIL]            = {COMPOUND_STRING("Mail"),            CursorCb_Mail},
    [MENU_TAKE_MAIL]       = {COMPOUND_STRING("Take"),            CursorCb_TakeMail},
    [MENU_READ]            = {COMPOUND_STRING("Read"),            CursorCb_Read},
    [MENU_CANCEL2]         = {COMPOUND_STRING("Cancel"),          CursorCb_Cancel2},
    [MENU_SHIFT]           = {COMPOUND_STRING("Shift"),           CursorCb_SendMon},
    [MENU_SEND_OUT]        = {COMPOUND_STRING("Send Out"),        CursorCb_SendMon},
    [MENU_ENTER]           = {COMPOUND_STRING("Enter"),           CursorCb_Enter},
    [MENU_NO_ENTRY]        = {COMPOUND_STRING("No Entry"),        CursorCb_NoEntry},
    [MENU_STORE]           = {COMPOUND_STRING("Store"),           CursorCb_Store},
    [MENU_REGISTER]        = {COMPOUND_STRING("Register"),        CursorCb_Register},
    [MENU_TRADE1]          = {COMPOUND_STRING("Trade"),           CursorCb_Trade1},
    [MENU_TRADE2]          = {COMPOUND_STRING("Trade"),           CursorCb_Trade2},
    [MENU_TOSS]            = {COMPOUND_STRING("Toss"),            CursorCb_Toss},
    [MENU_CATALOG_BULB]    = {COMPOUND_STRING("Light bulb"),      CursorCb_CatalogBulb},
    [MENU_CATALOG_OVEN]    = {COMPOUND_STRING("Microwave oven"),  CursorCb_CatalogOven},
    [MENU_CATALOG_WASHING] = {COMPOUND_STRING("Washing machine"), CursorCb_CatalogWashing},
    [MENU_CATALOG_FRIDGE]  = {COMPOUND_STRING("Refrigerator"),    CursorCb_CatalogFridge},
    [MENU_CATALOG_FAN]     = {COMPOUND_STRING("Electric fan"),    CursorCb_CatalogFan},
    [MENU_CATALOG_MOWER]   = {COMPOUND_STRING("Lawn mower"),      CursorCb_CatalogMower},
    [MENU_CHANGE_FORM]     = {COMPOUND_STRING("Change form"),     CursorCb_ChangeForm},
    [MENU_CHANGE_ABILITY]  = {COMPOUND_STRING("Change Ability"),  CursorCb_ChangeAbility},
};

static const u8 sPartyMenuAction_SummarySwitchCancel[] = {MENU_SUMMARY, MENU_SWITCH, MENU_CANCEL1};
static const u8 sPartyMenuAction_ShiftSummaryCancel[] = {MENU_SHIFT, MENU_SUMMARY, MENU_CANCEL1};
static const u8 sPartyMenuAction_SendOutSummaryCancel[] = {MENU_SEND_OUT, MENU_SUMMARY, MENU_CANCEL1};
static const u8 sPartyMenuAction_SummaryCancel[] = {MENU_SUMMARY, MENU_CANCEL1};
static const u8 sPartyMenuAction_EnterSummaryCancel[] = {MENU_ENTER, MENU_SUMMARY, MENU_CANCEL1};
static const u8 sPartyMenuAction_NoEntrySummaryCancel[] = {MENU_NO_ENTRY, MENU_SUMMARY, MENU_CANCEL1};
static const u8 sPartyMenuAction_StoreSummaryCancel[] = {MENU_STORE, MENU_SUMMARY, MENU_CANCEL1};
static const u8 sPartyMenuAction_GiveTakeItemCancel[] = {MENU_GIVE, MENU_TAKE_ITEM, MENU_MOVE_ITEM, MENU_CANCEL2};
static const u8 sPartyMenuAction_ReadTakeMailCancel[] = {MENU_READ, MENU_TAKE_MAIL, MENU_CANCEL2};
static const u8 sPartyMenuAction_RegisterSummaryCancel[] = {MENU_REGISTER, MENU_SUMMARY, MENU_CANCEL1};
static const u8 sPartyMenuAction_TradeSummaryCancel1[] = {MENU_TRADE1, MENU_SUMMARY, MENU_CANCEL1};
static const u8 sPartyMenuAction_TradeSummaryCancel2[] = {MENU_TRADE2, MENU_SUMMARY, MENU_CANCEL1};
static const u8 sPartyMenuAction_TakeItemTossCancel[] = {MENU_TAKE_ITEM, MENU_TOSS, MENU_CANCEL1};
static const u8 sPartyMenuAction_RotomCatalog[] = {MENU_CATALOG_BULB, MENU_CATALOG_OVEN, MENU_CATALOG_WASHING, MENU_CATALOG_FRIDGE, MENU_CATALOG_FAN, MENU_CATALOG_MOWER, MENU_CANCEL1};
static const u8 sPartyMenuAction_ZygardeCube[] = {MENU_CHANGE_FORM, MENU_CHANGE_ABILITY, MENU_CANCEL1};



static const u8 *const sPartyMenuActions[] =
{
    [ACTIONS_NONE]          = NULL,
    [ACTIONS_SWITCH]        = sPartyMenuAction_SummarySwitchCancel,
    [ACTIONS_SHIFT]         = sPartyMenuAction_ShiftSummaryCancel,
    [ACTIONS_SEND_OUT]      = sPartyMenuAction_SendOutSummaryCancel,
    [ACTIONS_ENTER]         = sPartyMenuAction_EnterSummaryCancel,
    [ACTIONS_NO_ENTRY]      = sPartyMenuAction_NoEntrySummaryCancel,
    [ACTIONS_STORE]         = sPartyMenuAction_StoreSummaryCancel,
    [ACTIONS_SUMMARY_ONLY]  = sPartyMenuAction_SummaryCancel,
    [ACTIONS_ITEM]          = sPartyMenuAction_GiveTakeItemCancel,
    [ACTIONS_MAIL]          = sPartyMenuAction_ReadTakeMailCancel,
    [ACTIONS_REGISTER]      = sPartyMenuAction_RegisterSummaryCancel,
    [ACTIONS_TRADE]         = sPartyMenuAction_TradeSummaryCancel1,
    [ACTIONS_SPIN_TRADE]    = sPartyMenuAction_TradeSummaryCancel2,
    [ACTIONS_TAKEITEM_TOSS] = sPartyMenuAction_TakeItemTossCancel,
    [ACTIONS_ROTOM_CATALOG] = sPartyMenuAction_RotomCatalog,
    [ACTIONS_ZYGARDE_CUBE]  = sPartyMenuAction_ZygardeCube,
};

static const u8 sPartyMenuActionCounts[] =
{
    [ACTIONS_NONE]          = 0,
    [ACTIONS_SWITCH]        = ARRAY_COUNT(sPartyMenuAction_SummarySwitchCancel),
    [ACTIONS_SHIFT]         = ARRAY_COUNT(sPartyMenuAction_ShiftSummaryCancel),
    [ACTIONS_SEND_OUT]      = ARRAY_COUNT(sPartyMenuAction_SendOutSummaryCancel),
    [ACTIONS_ENTER]         = ARRAY_COUNT(sPartyMenuAction_EnterSummaryCancel),
    [ACTIONS_NO_ENTRY]      = ARRAY_COUNT(sPartyMenuAction_NoEntrySummaryCancel),
    [ACTIONS_STORE]         = ARRAY_COUNT(sPartyMenuAction_StoreSummaryCancel),
    [ACTIONS_SUMMARY_ONLY]  = ARRAY_COUNT(sPartyMenuAction_SummaryCancel),
    [ACTIONS_ITEM]          = ARRAY_COUNT(sPartyMenuAction_GiveTakeItemCancel),
    [ACTIONS_MAIL]          = ARRAY_COUNT(sPartyMenuAction_ReadTakeMailCancel),
    [ACTIONS_REGISTER]      = ARRAY_COUNT(sPartyMenuAction_RegisterSummaryCancel),
    [ACTIONS_TRADE]         = ARRAY_COUNT(sPartyMenuAction_TradeSummaryCancel1),
    [ACTIONS_SPIN_TRADE]    = ARRAY_COUNT(sPartyMenuAction_TradeSummaryCancel2),
    [ACTIONS_TAKEITEM_TOSS] = ARRAY_COUNT(sPartyMenuAction_TakeItemTossCancel),
    [ACTIONS_ROTOM_CATALOG] = ARRAY_COUNT(sPartyMenuAction_RotomCatalog),
    [ACTIONS_ZYGARDE_CUBE]  = ARRAY_COUNT(sPartyMenuAction_ZygardeCube),
};

STATIC_ASSERT(ARRAY_COUNT(sPartyMenuAction_RotomCatalog) <= MAX_PARTY_MENU_ACTIONS, PartyStaticActionsOverflow);

static const u8 *const sUnionRoomTradeMessages[] =
{
    [UR_TRADE_MSG_NOT_MON_PARTNER_WANTS - 1]       = gText_NotPkmnOtherTrainerWants,
    [UR_TRADE_MSG_NOT_EGG - 1]                     = gText_ThatIsntAnEgg,
    [UR_TRADE_MSG_MON_CANT_BE_TRADED_NOW - 1]      = gText_PkmnCantBeTradedNow,
    [UR_TRADE_MSG_MON_CANT_BE_TRADED - 1]          = gText_PkmnCantBeTraded,
    [UR_TRADE_MSG_PARTNERS_MON_CANT_BE_TRADED - 1] = gText_OtherTrainersPkmnCantBeTraded,
    [UR_TRADE_MSG_EGG_CANT_BE_TRADED -1]           = gText_EggCantBeTradedNow,
    [UR_TRADE_MSG_PARTNER_CANT_ACCEPT_MON - 1]     = gText_OtherTrainerCantAcceptPkmn,
    [UR_TRADE_MSG_CANT_TRADE_WITH_PARTNER_1 - 1]   = gText_CantTradeWithTrainer,
    [UR_TRADE_MSG_CANT_TRADE_WITH_PARTNER_2 - 1]   = gText_CantTradeWithTrainer,
};

static const struct OamData sOamData_HeldItem =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(8x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(8x8),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AnimCmd sSpriteAnim_HeldItem[] =
{
    ANIMCMD_FRAME(0, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_HeldMail[] =
{
    ANIMCMD_FRAME(1, 1),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_HeldItem[] =
{
    sSpriteAnim_HeldItem,
    sSpriteAnim_HeldMail,
};

static const struct SpriteSheet sSpriteSheet_HeldItem =
{
    .data = sHeldItem_Gfx, .size = sizeof(sHeldItem_Gfx), .tag = TAG_HELD_ITEM
};

const struct SpriteSheet gSpriteSheet_HeldItem =
{
    .data = gHeldItemGfx, .size = sizeof(gHeldItemGfx), .tag = TAG_HELD_ITEM
};

static const struct SpritePalette sSpritePalette_HeldItem =
{
    .data = sHeldItem_Pal, .tag = TAG_HELD_ITEM
};

static const struct SpriteTemplate sSpriteTemplate_HeldItem =
{
    .tileTag = TAG_HELD_ITEM,
    .paletteTag = TAG_HELD_ITEM,
    .oam = &sOamData_HeldItem,
    .anims = sSpriteAnimTable_HeldItem,
};

static const struct OamData sOamData_Cursor =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .size = SPRITE_SIZE(16x16),
    .priority = 1,
};

static const struct CompressedSpriteSheet sSpriteSheet_Cursor =
{
    .data = gCursorSwSh_Gfx,
    .size = (16 * 16) / 2,
    .tag = TAG_CURSOR
};

static const struct SpriteTemplate sSpriteTemplate_Cursor =
{
    .tileTag = TAG_CURSOR,
    .paletteTag = TAG_SWSH_UI_PAL,
    .oam = &sOamData_Cursor,
};

static const struct OamData sOamData_SelectFrame =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .size = SPRITE_SIZE(16x32),
    .x = 0,
    .matrixNum = 0,
    .shape = SPRITE_SHAPE(16x32),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AnimCmd sSpriteAnim_SelectFrameLeft[] = {
    ANIMCMD_FRAME(0, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_SelectFrameRight[] = {
    ANIMCMD_FRAME(0, 0, TRUE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_SelectFrameMiddle[] = {
    ANIMCMD_FRAME(8, 0, FALSE, FALSE),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_SelectFrame[] = {
    sSpriteAnim_SelectFrameLeft,
    sSpriteAnim_SelectFrameRight,
    sSpriteAnim_SelectFrameMiddle,
};

static const struct CompressedSpriteSheet sSpriteSheet_SelectFrame =
{
    .data = sSelectFrame_Gfx,
    .size = (16 * 32 * 2) / 2,
    .tag = TAG_SELECT_FRAME,
};

static const struct SpritePalette sSpritePal_SelectFrame =
{
    .data = sHeldItem_Pal,
    .tag = TAG_HELD_ITEM,
};

static const struct SpriteTemplate sSpriteTemplate_SelectFrame =
{
    .tileTag = TAG_SELECT_FRAME ,
    .paletteTag = TAG_HELD_ITEM,
    .oam = &sOamData_SelectFrame,
    .anims = sSpriteAnimTable_SelectFrame,
};

static const struct OamData sOamData_MessageWindow =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .size = SPRITE_SIZE(32x64),
    .x = 0,
    .matrixNum = 0,
    .shape = SPRITE_SHAPE(32x64),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AnimCmd sSpriteAnim_MessageWindow_0[] = {
    ANIMCMD_FRAME(0, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_MessageWindow_1[] = {
    ANIMCMD_FRAME(32, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_MessageWindow_2[] = {
    ANIMCMD_FRAME(0, 0, TRUE, TRUE),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_MessageWindow[] = {
    sSpriteAnim_MessageWindow_0,
    sSpriteAnim_MessageWindow_1,
    sSpriteAnim_MessageWindow_2,
};

static const u8 sMessageWindowAnims[MESSAGE_WINDOW_SPRITES_COUNT] = {0, 1, 1, 1, 1, 1, 1, 2};

static const struct CompressedSpriteSheet sSpriteSheet_MessageWindow =
{
    .data = sMessageWindow_Gfx,
    .size = (32 * 64 * 2) / 2,
    .tag = TAG_MESSAGE_WINDOW,
};

static const struct SpriteTemplate sSpriteTemplate_MessageWindow =
{
    .tileTag = TAG_MESSAGE_WINDOW,
    .paletteTag = TAG_HELD_ITEM,
    .oam = &sOamData_MessageWindow,
    .anims = sSpriteAnimTable_MessageWindow,
};

static const struct SpritePalette sSpritePal_PartyMonShadow =
{
    .data = gMonShadowSwSh_Pal,
    .tag = TAG_MON_SHADOW
};

const struct SpriteTemplate gSpriteTemplate_StatusIcons =
{
    .tileTag = TAG_SWSH_STATUS_ICONS,
    .paletteTag = TAG_SWSH_UI_PAL,
    .oam = &gOamData_StatusIconsSwSh,
    .anims = gSpriteAnimTable_StatusIconsSwSh,
};

static const struct OamData sOamData_MoveTypes =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AnimCmd sSpriteAnim_TypeNone[] = {
    ANIMCMD_FRAME(TYPE_NONE * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeNormal[] = {
    ANIMCMD_FRAME(TYPE_NORMAL * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeFighting[] = {
    ANIMCMD_FRAME(TYPE_FIGHTING * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeFlying[] = {
    ANIMCMD_FRAME(TYPE_FLYING * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypePoison[] = {
    ANIMCMD_FRAME(TYPE_POISON * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeGround[] = {
    ANIMCMD_FRAME(TYPE_GROUND * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeRock[] = {
    ANIMCMD_FRAME(TYPE_ROCK * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeBug[] = {
    ANIMCMD_FRAME(TYPE_BUG * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeGhost[] = {
    ANIMCMD_FRAME(TYPE_GHOST * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeSteel[] = {
    ANIMCMD_FRAME(TYPE_STEEL * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeMystery[] = {
    ANIMCMD_FRAME(TYPE_MYSTERY * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeFire[] = {
    ANIMCMD_FRAME(TYPE_FIRE * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeWater[] = {
    ANIMCMD_FRAME(TYPE_WATER * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeGrass[] = {
    ANIMCMD_FRAME(TYPE_GRASS * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeElectric[] = {
    ANIMCMD_FRAME(TYPE_ELECTRIC * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypePsychic[] = {
    ANIMCMD_FRAME(TYPE_PSYCHIC * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeIce[] = {
    ANIMCMD_FRAME(TYPE_ICE * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeDragon[] = {
    ANIMCMD_FRAME(TYPE_DRAGON * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeDark[] = {
    ANIMCMD_FRAME(TYPE_DARK * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeFairy[] = {
    ANIMCMD_FRAME(TYPE_FAIRY * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_TypeStellar[] = {
    ANIMCMD_FRAME(TYPE_STELLAR * 4, 0, FALSE, FALSE),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_MoveTypes[NUMBER_OF_MON_TYPES] = {
    [TYPE_NONE]     = sSpriteAnim_TypeNone,
    [TYPE_NORMAL]   = sSpriteAnim_TypeNormal,
    [TYPE_FIGHTING] = sSpriteAnim_TypeFighting,
    [TYPE_FLYING]   = sSpriteAnim_TypeFlying,
    [TYPE_POISON]   = sSpriteAnim_TypePoison,
    [TYPE_GROUND]   = sSpriteAnim_TypeGround,
    [TYPE_ROCK]     = sSpriteAnim_TypeRock,
    [TYPE_BUG]      = sSpriteAnim_TypeBug,
    [TYPE_GHOST]    = sSpriteAnim_TypeGhost,
    [TYPE_STEEL]    = sSpriteAnim_TypeSteel,
    [TYPE_MYSTERY]  = sSpriteAnim_TypeMystery,
    [TYPE_FIRE]     = sSpriteAnim_TypeFire,
    [TYPE_WATER]    = sSpriteAnim_TypeWater,
    [TYPE_GRASS]    = sSpriteAnim_TypeGrass,
    [TYPE_ELECTRIC] = sSpriteAnim_TypeElectric,
    [TYPE_PSYCHIC]  = sSpriteAnim_TypePsychic,
    [TYPE_ICE]      = sSpriteAnim_TypeIce,
    [TYPE_DRAGON]   = sSpriteAnim_TypeDragon,
    [TYPE_DARK]     = sSpriteAnim_TypeDark,
    [TYPE_FAIRY]    = sSpriteAnim_TypeFairy,
    [TYPE_STELLAR]  = sSpriteAnim_TypeStellar,
};

static const u8 sMoveTypeToPalOffset[NUMBER_OF_MON_TYPES] =
{
    [TYPE_ELECTRIC] = 1,
    [TYPE_FAIRY]    = 1,
    [TYPE_BUG]      = 1,
    [TYPE_GRASS]    = 1,
    [TYPE_GROUND]   = 2,
    [TYPE_DARK]     = 2,
    [TYPE_FLYING]   = 2,
    [TYPE_GHOST]    = 2,
    [TYPE_WATER]    = 3,
    [TYPE_DRAGON]   = 3,
    [TYPE_STEEL]    = 4,
};

static const struct CompressedSpriteSheet sSpriteSheet_MoveTypes =
{
    .data = sMoveTypes_Gfx,
    .size = (NUMBER_OF_MON_TYPES) * 0x80,
    .tag = TAG_MOVE_TYPES
};

static const struct SpriteTemplate sSpriteTemplate_MoveTypes =
{
    .tileTag = TAG_MOVE_TYPES,
    .paletteTag = POKE_ICON_BASE_PAL_TAG,
    .oam = &sOamData_MoveTypes,
    .anims = sSpriteAnimTable_MoveTypes,
};

static const struct OamData sOamData_Button = {
    .size = SPRITE_SIZE(32x8),
    .shape = SPRITE_SHAPE(32x8),
    .priority = 0,
};

static const u8 *const sUnused_StatStrings[] =
{
    gText_HP4,
    gText_Attack3,
    gText_Defense3,
    gText_SpAtk4,
    gText_SpDef4,
    gText_Speed2
};

#define ROTOM_BASE_MOVE  MOVE_THUNDER_SHOCK
#define ROTOM_HEAT_MOVE  MOVE_OVERHEAT
#define ROTOM_WASH_MOVE  MOVE_HYDRO_PUMP
#define ROTOM_FROST_MOVE MOVE_BLIZZARD
#define ROTOM_FAN_MOVE   MOVE_AIR_SLASH
#define ROTOM_MOW_MOVE   MOVE_LEAF_STORM

static const u16 sRotomFormChangeMoves[5] =
{
    ROTOM_HEAT_MOVE,
    ROTOM_WASH_MOVE,
    ROTOM_FROST_MOVE,
    ROTOM_FAN_MOVE,
    ROTOM_MOW_MOVE,
};
