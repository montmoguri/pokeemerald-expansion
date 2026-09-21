#include "global.h"
#include "bg.h"
#include "comfy_anim.h"
#include "decompress.h"
#include "decoration.h"
#include "decoration_inventory.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "field_weather.h"
#include "fieldmap.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "international_string_util.h"
#include "item.h"
#include "item_icon.h"
#include "list_menu.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "move.h"
#include "money.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "scanline_effect.h"
#include "shop.h"
#include "shop_criteria.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "swsh_shop.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "tv.h"
#include "window.h"
#include "constants/event_object_movement.h"
#include "constants/event_objects.h"
#include "constants/game_stat.h"
#include "constants/items.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "constants/tv.h"

enum {
    MART_TYPE_NORMAL,
    MART_TYPE_DECOR,
    MART_TYPE_DECOR2,
};

static EWRAM_DATA struct {
    const u16 *itemList;
    u16 itemCount;
    u8 martType;
} sMartInfo = {0};

void SetBuyMenuMart_SwSh(u8 martType, const u16 *itemList, u16 itemCount)
{
    sMartInfo.martType = martType;
    sMartInfo.itemList = itemList;
    sMartInfo.itemCount = itemCount;
}

#define SHOP_MENU_PALETTE_ID (gMapHeader.mapLayout->isFrlg ? 11 : 12)

// Mont note: shop menu shares char base 0 with the field tileset, current design only use 9 tiles
#define SHOP_MENU_BASE_TILE 1014

enum {
    WIN_MONEY,
    WIN_ITEM_LIST,
    WIN_ITEM_DESCRIPTION,
    WIN_QUANTITY_IN_BAG,
    WIN_MESSAGE,
};

enum {
    COLORID_NORMAL,
    COLORID_HOVER_NAME,
    COLORID_HOVER_PRICE,
    COLORID_MONEY,
};

enum {
    OBJ_EVENT_ID,
    X_COORD,
    Y_COORD,
    ANIM_NUM,
    VIEWPORT_OBJECT_FIELD_COUNT,
};

enum {
    SPINNER_ARROW_UP,
    SPINNER_ARROW_DOWN,
    SPINNER_ARROW_SPRITES_COUNT,
};

enum {
    SPINNER_ARROW_STATIC,
    SPINNER_ARROW_ANIM,
};

#define LIST_NAME_BUFFER_SIZE           (ITEM_NAME_LENGTH + 15)
#define DESCRIPTION_BUFFER_SIZE         200
#define HOVER_SLOT_SPRITES_COUNT        6
#define SCROLL_THUMB_SPRITES_COUNT      3
#define QUANTITY_FRAME_SPRITES_COUNT    2
#define ITEM_ICON_SLOT_COUNT            2

struct ShopData
{
    u16 menuTilemap[0x400];         // shop menu
    u16 mapTopTilemap[0x400];       // BG1, shared with shop menu
    u16 mapMidTilemap[0x400];       // BG2
    u16 mapBottomTilemap[0x400];    // BG3
    s16 viewportObjects[OBJECT_EVENTS_COUNT][VIEWPORT_OBJECT_FIELD_COUNT];
    u32 tintPalettes;               // obj pals for the map's sprites
    u8 (*itemNames)[LIST_NAME_BUFFER_SIZE];
    u8 descriptionBuffer[DESCRIPTION_BUFFER_SIZE];
    u16 listTotal;
    u16 listShown;
    u16 scrollOffset;
    u16 selectedRow;
    s32 hoveredIndex;
    u32 totalCost;
    u16 maxQuantity;
    u16 shownItemId;
    u8 cursorSpriteId;
    u8 hoverSlotSpriteIds[HOVER_SLOT_SPRITES_COUNT];
    u8 scrollThumbSpriteIds[SCROLL_THUMB_SPRITES_COUNT];
    u8 quantityFrameSpriteIds[QUANTITY_FRAME_SPRITES_COUNT];
    u8 spinnerArrowSpriteIds[SPINNER_ARROW_SPRITES_COUNT];
    u8 itemSpriteIds[ITEM_ICON_SLOT_COUNT];
    u8 iconSlot:1;
    u32 cursorAnimId;
    u32 cursorBobAnimId;
    u32 scrollThumbAnimId;
};

static EWRAM_DATA struct ShopData *sShopData = NULL;

static void CB2_BuyMenu(void);
static void VBlankCB_BuyMenu(void);
static void BuyMenuInitBgs(void);
static void BuyMenuInitWindows(void);
static void BuyMenuDecompressBgGraphics(void);
static void BuyMenuDrawGraphics(void);
static void BuyMenuDrawMapGraphics(void);
static void BuyMenuTintMapView(void);
static void BuyMenuDrawMapBg(void);
static void BuyMenuDrawMapMetatile(s16 x, s16 y, const u16 *src, u8 metatileLayerType);
static void BuyMenuDrawMapMetatileLayer(u16 *dest, s16 offset1, s16 offset2, const u16 *src);
static void BuyMenuCollectObjectEventData(void);
static void BuyMenuDrawObjectEvents(void);
static void BuyMenuCopyMenuBgToBg1TilemapBuffer(void);
static bool8 BuyMenuCheckForOverlapWithMenuBg(int x, int y);
static void BuyMenuFreeMemory(void);
static void BuyMenuPrint(u8 windowId, u8 fontId, const u8 *str, u8 x, u8 y, u8 letterSpacing, u8 lineSpacing, u8 speed, u8 colorId);
static void BuyMenuBuildItemNames(void);
static u16 BuyMenuGetEntryId(u32 index);
static u32 BuyMenuGetItemPrice(u32 itemId);
static bool32 BuyMenuIsSoldOut(u32 itemId);
static void BuyMenuPrintMoney(void);
static void BuyMenuPrintPriceInList(u32 itemId, u8 y, bool32 isHovered);
static void BuyMenuPrintItemDescription(u32 itemId);
static void BuyMenuPrintQuantityInBag(u32 itemId);
static void BuyMenuMoveCursorCallback(u32 index, bool32 onInit);
static void BuyMenuLoadSpriteGfx(void);
static void BuyMenuCreateListSprites(void);
static void BuyMenuAddItemIcon(u32 itemId, u8 iconSlot, s16 spriteY);
static void BuyMenuRemoveItemIcon(u8 iconSlot);
static void SpriteCB_SlideCursorY(struct Sprite *sprite);
static void SpriteCB_ScrollThumb(struct Sprite *sprite);
static void SpriteCB_SpinnerArrow(struct Sprite *sprite);
static void CreateQuantityFrameSprites(u8 y);
static void DestroyQuantityFrameSprites(void);
static void AnimateQuantitySpinner(void);
static void BuyMenuPrintQuantity(s16 quantity);
static void BuyMenuPrintTotalCost(u32 total);
static s16 ShopList_RowSpriteY(u16 row);
static u8 ShopList_RowHeight(void);
static void ShopList_RefreshRow(u8 row);
static void ShopList_RefreshColors(void);
static void ShopList_ScrollRows(bool32 movingDown);
static void ShopList_SeekTo(u16 absPos);
static void ShopList_Step(bool32 movingDown, bool32 allowWrap);
static void ShopList_Refresh(void);
static void ShopList_Move(bool32 movingDown, bool32 allowWrap);
static s32 ShopList_ProcessInput(void);
static void ConvertMoneyToCommaString(u8 *dest, u32 amount);
static void BuyMenuDisplayMessage(u8 taskId, const u8 *text, TaskFunc callback);
static void BuyMenuReturnToItemList(u8 taskId);
static void Task_BuyHowManyDialogueInit(u8 taskId);
static void Task_BuyHowManyDialogueHandleInput(u8 taskId);
static void BuyMenuConfirmPurchase(u8 taskId);
static void BuyMenuTryMakePurchase(u8 taskId);
static void BuyMenuSubtractMoney(u8 taskId);
static void Task_ReturnToItemListAfterItemPurchase(u8 taskId);
static void Task_ReturnToItemListAfterDecorationPurchase(u8 taskId);
static u8 FormatDescriptionByWidth(u8 *result, s32 resultSize, s32 maxWidth, u8 fontId, const u8 *str, s16 letterSpacing);
static void Task_BuyMenu(u8 taskId);
static void ExitBuyMenu(u8 taskId);
static void Task_ExitBuyMenu(u8 taskId);

static const u32 sShopMenu_Gfx[]     = INCGFX_U32("graphics/shop/swsh/menu.png", ".4bpp.smol", "-num_tiles 10 -Wnum_tiles");
static const u16 sShopMenu_Pal[]     = INCGFX_U16("graphics/shop/swsh/menu.png", ".gbapal");
static const u32 sShopMenu_Tilemap[] = INCGFX_U32("graphics/shop/swsh/menu.bin", ".smolTM");
static const u32 sHoverSlot_Gfx[]    = INCGFX_U32("graphics/shop/swsh/hover_slot.png", ".4bpp.smol");
static const u32 sScrollThumb_Gfx[]  = INCGFX_U32("graphics/shop/swsh/scroll_thumb.png", ".4bpp.smol");
static const u16 sShopUI_Pal[]       = INCGFX_U16("graphics/shop/swsh/hover_slot.png", ".gbapal");

#define TAG_SHOP_UI_PAL            200
#define TAG_SHOP_SHARED_PAL        201
#define TAG_CURSOR                 202
#define TAG_HOVER_SLOT             203
#define TAG_SCROLL_THUMB           204
#define TAG_QUANTITY_FRAME         205
#define TAG_SPINNER_ARROW          206
#define TAG_ITEM_ICON_BASE         207 // and 208 for item icon swapping

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
    .tag = TAG_CURSOR,
};

static const struct SpriteTemplate sSpriteTemplate_Cursor =
{
    .tileTag = TAG_CURSOR,
    .paletteTag = TAG_SHOP_SHARED_PAL,
    .oam = &sOamData_Cursor,
    .callback = SpriteCB_SlideCursorY,
};

static const struct OamData sOamData_HoverSlot =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x16),
    .size = SPRITE_SIZE(32x16),
    .priority = 1,
};

#define HOVER_SLOT_FRAME_TILES  ((32 * 16) / (8 * 8))

static const union AnimCmd sSpriteAnim_HoverSlot_0[] = {
    ANIMCMD_FRAME(0 * HOVER_SLOT_FRAME_TILES, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_HoverSlot_1[] = {
    ANIMCMD_FRAME(1 * HOVER_SLOT_FRAME_TILES, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_HoverSlot_2[] = {
    ANIMCMD_FRAME(2 * HOVER_SLOT_FRAME_TILES, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_HoverSlot_3[] = {
    ANIMCMD_FRAME(3 * HOVER_SLOT_FRAME_TILES, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_HoverSlot_4[] = {
    ANIMCMD_FRAME(4 * HOVER_SLOT_FRAME_TILES, 0, FALSE, FALSE),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_HoverSlot[] = {
    sSpriteAnim_HoverSlot_0,
    sSpriteAnim_HoverSlot_1,
    sSpriteAnim_HoverSlot_2,
    sSpriteAnim_HoverSlot_3,
    sSpriteAnim_HoverSlot_4,
};

static const u8 sHoverSlotAnims[HOVER_SLOT_SPRITES_COUNT] = {0, 1, 1, 2, 3, 4};

static const struct CompressedSpriteSheet sSpriteSheet_HoverSlot =
{
    .data = sHoverSlot_Gfx,
    .size = (32 * 16 * 5) / 2,
    .tag = TAG_HOVER_SLOT,
};

static const struct SpriteTemplate sSpriteTemplate_HoverSlot =
{
    .tileTag = TAG_HOVER_SLOT,
    .paletteTag = TAG_SHOP_UI_PAL,
    .oam = &sOamData_HoverSlot,
    .anims = sSpriteAnimTable_HoverSlot,
};

static const struct OamData sOamData_ScrollThumb =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(8x32),
    .size = SPRITE_SIZE(8x32),
    .priority = 1,
};

static const struct CompressedSpriteSheet sSpriteSheet_ScrollThumb =
{
    .data = sScrollThumb_Gfx,
    .size = (8 * 32) / 2,
    .tag = TAG_SCROLL_THUMB,
};

static const struct SpriteTemplate sSpriteTemplate_ScrollThumb =
{
    .tileTag = TAG_SCROLL_THUMB,
    .paletteTag = TAG_SHOP_UI_PAL,
    .oam = &sOamData_ScrollThumb,
    .callback = SpriteCB_ScrollThumb,
};

static const struct OamData sOamData_QuantityFrame =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x32),
    .size = SPRITE_SIZE(64x32),
    .priority = 0,
};

#define QUANTITY_FRAME_TILES    ((64 * 32) / (8 * 8))

static const union AnimCmd sSpriteAnim_QuantityFrame_0[] = {
    ANIMCMD_FRAME(0 * QUANTITY_FRAME_TILES, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_QuantityFrame_1[] = {
    ANIMCMD_FRAME(1 * QUANTITY_FRAME_TILES, 0, FALSE, FALSE),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_QuantityFrame[] = {
    sSpriteAnim_QuantityFrame_0,
    sSpriteAnim_QuantityFrame_1,
};

static const struct CompressedSpriteSheet sSpriteSheet_QuantityFrame =
{
    .data = gQuantityFrameSwSh_Gfx,
    .size = (64 * 64) / 2,
    .tag = TAG_QUANTITY_FRAME,
};

static const struct SpriteTemplate sSpriteTemplate_QuantityFrame =
{
    .tileTag = TAG_QUANTITY_FRAME,
    .paletteTag = TAG_SHOP_SHARED_PAL,
    .oam = &sOamData_QuantityFrame,
    .anims = sSpriteAnimTable_QuantityFrame,
};

static const struct OamData sOamData_SpinnerArrow =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x8),
    .size = SPRITE_SIZE(16x8),
    .priority = 0,
};

static const union AnimCmd sSpriteAnim_SpinnerArrowUp[] = {
    ANIMCMD_FRAME(0, 0, FALSE, FALSE),
    ANIMCMD_END
};
static const union AnimCmd sSpriteAnim_SpinnerArrowDown[] = {
    ANIMCMD_FRAME(0, 0, FALSE, TRUE),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_SpinnerArrow[] = {
    [SPINNER_ARROW_UP]   = sSpriteAnim_SpinnerArrowUp,
    [SPINNER_ARROW_DOWN] = sSpriteAnim_SpinnerArrowDown,
};

static const struct CompressedSpriteSheet sSpriteSheet_SpinnerArrow =
{
    .data = gSpinnerArrowSwSh_Gfx,
    .size = (16 * 8) / 2,
    .tag = TAG_SPINNER_ARROW,
};

static const struct SpriteTemplate sSpriteTemplate_SpinnerArrow =
{
    .tileTag = TAG_SPINNER_ARROW,
    .paletteTag = TAG_SHOP_SHARED_PAL,
    .oam = &sOamData_SpinnerArrow,
    .anims = sSpriteAnimTable_SpinnerArrow,
    .callback = SpriteCB_SpinnerArrow,
};

static const struct SpritePalette sShopSpritePalettes[] =
{
    { sShopUI_Pal,          TAG_SHOP_UI_PAL },
    { gStatusIconsSwSh_Pal, TAG_SHOP_SHARED_PAL },
    {},
};

static const union AffineAnimCmd sAffineAnim_ItemIcon_Appear[] =
{
    AFFINEANIMCMD_FRAME(192, 192, 0, 0),
    AFFINEANIMCMD_FRAME(8, 8, 0, 8),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd *const sAffineAnims_ItemIcon[] =
{
    sAffineAnim_ItemIcon_Appear,
};

static const struct BgTemplate sShopBuyMenuBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 2,
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
        .mapBaseIndex = 29,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
    {
        .bg = 3,
        .charBaseIndex = 0,
        .mapBaseIndex = 28,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    }
};

#define SHOP_CHAR_BASE_TILES        ((BG_SCREEN_ADDR(28) - BG_CHAR_ADDR(2)) / TILE_SIZE_4BPP)

#define SHOP_TILES_STD_BORDER       9
#define SHOP_TILES_MSGBOX           14

#define SHOP_BASE_STD_BORDER        1
#define SHOP_BASE_MSGBOX            (SHOP_BASE_STD_BORDER + SHOP_TILES_STD_BORDER)

#define WIN_MONEY_W                 10
#define WIN_MONEY_H                 3
#define WIN_MONEY_TILES             (WIN_MONEY_W * WIN_MONEY_H)
#define WIN_MONEY_BASE              (SHOP_BASE_MSGBOX + SHOP_TILES_MSGBOX)

#define WIN_ITEM_LIST_W             17
#define WIN_ITEM_LIST_H             12
#define WIN_ITEM_LIST_TILES         (WIN_ITEM_LIST_W * WIN_ITEM_LIST_H)
#define WIN_ITEM_LIST_BASE          (WIN_MONEY_BASE + WIN_MONEY_TILES)

#define WIN_ITEM_DESCRIPTION_W      17
#define WIN_ITEM_DESCRIPTION_H      4
#define WIN_ITEM_DESCRIPTION_TILES  (WIN_ITEM_DESCRIPTION_W * WIN_ITEM_DESCRIPTION_H)
#define WIN_ITEM_DESCRIPTION_BASE   (WIN_ITEM_LIST_BASE + WIN_ITEM_LIST_TILES)

#define WIN_QUANTITY_IN_BAG_W       3
#define WIN_QUANTITY_IN_BAG_H       2
#define WIN_QUANTITY_IN_BAG_TILES   (WIN_QUANTITY_IN_BAG_W * WIN_QUANTITY_IN_BAG_H)
#define WIN_QUANTITY_IN_BAG_BASE    (WIN_ITEM_DESCRIPTION_BASE + WIN_ITEM_DESCRIPTION_TILES)

#define WIN_MESSAGE_W               27
#define WIN_MESSAGE_H               4
#define WIN_MESSAGE_TILES           (WIN_MESSAGE_W * WIN_MESSAGE_H)
#define WIN_MESSAGE_BASE            (WIN_QUANTITY_IN_BAG_BASE + WIN_QUANTITY_IN_BAG_TILES)

#define WIN_YESNO_W                 5
#define WIN_YESNO_H                 4
#define WIN_YESNO_TILES             (WIN_YESNO_W * WIN_YESNO_H)
#define WIN_YESNO_BASE              (WIN_MESSAGE_BASE + WIN_MESSAGE_TILES)

#define SHOP_TILES_END              (WIN_YESNO_BASE + WIN_YESNO_TILES)

STATIC_ASSERT(SHOP_TILES_END <= SHOP_CHAR_BASE_TILES, ShopMenuCharBaseOverflow);

static const struct WindowTemplate sShopBuyMenuWindowTemplates[] =
{
    [WIN_MONEY] = {
        .bg = 0,
        .tilemapLeft = 18,
        .tilemapTop = 0,
        .width = WIN_MONEY_W,
        .height = WIN_MONEY_H,
        .paletteNum = 15,
        .baseBlock = WIN_MONEY_BASE,
    },
    [WIN_ITEM_LIST] = {
        .bg = 0,
        .tilemapLeft = 11,
        .tilemapTop = 3,
        .width = WIN_ITEM_LIST_W,
        .height = WIN_ITEM_LIST_H,
        .paletteNum = 15,
        .baseBlock = WIN_ITEM_LIST_BASE,
    },
    [WIN_ITEM_DESCRIPTION] = {
        .bg = 0,
        .tilemapLeft = 7,
        .tilemapTop = 16,
        .width = WIN_ITEM_DESCRIPTION_W,
        .height = WIN_ITEM_DESCRIPTION_H,
        .paletteNum = 15,
        .baseBlock = WIN_ITEM_DESCRIPTION_BASE,
    },
    [WIN_QUANTITY_IN_BAG] = {
        .bg = 0,
        .tilemapLeft = 26,
        .tilemapTop = 17,
        .width = WIN_QUANTITY_IN_BAG_W,
        .height = WIN_QUANTITY_IN_BAG_H,
        .paletteNum = 15,
        .baseBlock = WIN_QUANTITY_IN_BAG_BASE,
    },
    [WIN_MESSAGE] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = WIN_MESSAGE_W,
        .height = WIN_MESSAGE_H,
        .paletteNum = 15,
        .baseBlock = WIN_MESSAGE_BASE,
    },
    DUMMY_WIN_TEMPLATE
};

static const struct WindowTemplate sShopBuyMenuYesNoWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 21,
    .tilemapTop = 9,
    .width = WIN_YESNO_W,
    .height = WIN_YESNO_H,
    .paletteNum = 15,
    .baseBlock = WIN_YESNO_BASE,
};

static const u8 sFontColorTable[][3] =
{
                            // bgColor, textColor, shadowColor
    [COLORID_NORMAL]      = {0,  5,  6},
    [COLORID_HOVER_NAME]  = {0,  7,  8},
    [COLORID_HOVER_PRICE] = {0,  7,  9},
    [COLORID_MONEY]       = {0,  7, 10},
};

static const struct YesNoFuncTable sShopPurchaseYesNoFuncs =
{
    BuyMenuTryMakePurchase,
    BuyMenuReturnToItemList
};

void CB2_InitBuyMenu_SwSh(void)
{
    switch (gMain.state)
    {
    case 0:
        SetVBlankHBlankCallbacksToNull();
        CpuFastFill(0, (void *)OAM, OAM_SIZE);
        ScanlineEffect_Stop();
        ResetTempTileDataBuffers();
        FreeAllSpritePalettes();
        ResetPaletteFade();
        ResetSpriteData();
        ResetTasks();
        ClearScheduledBgCopiesToVram();
        sShopData = AllocZeroed(sizeof(*sShopData));
        sShopData->cursorSpriteId = SPRITE_NONE;
        memset(sShopData->hoverSlotSpriteIds, SPRITE_NONE, sizeof(sShopData->hoverSlotSpriteIds));
        memset(sShopData->scrollThumbSpriteIds, SPRITE_NONE, sizeof(sShopData->scrollThumbSpriteIds));
        memset(sShopData->quantityFrameSpriteIds, SPRITE_NONE, sizeof(sShopData->quantityFrameSpriteIds));
        memset(sShopData->spinnerArrowSpriteIds, SPRITE_NONE, sizeof(sShopData->spinnerArrowSpriteIds));
        memset(sShopData->itemSpriteIds, SPRITE_NONE, sizeof(sShopData->itemSpriteIds));
        sShopData->cursorAnimId = INVALID_COMFY_ANIM;
        sShopData->cursorBobAnimId = INVALID_COMFY_ANIM;
        sShopData->scrollThumbAnimId = INVALID_COMFY_ANIM;
        if (sMartInfo.martType == MART_TYPE_NORMAL)
            TryBuildDynamicShopItemList(&sMartInfo.itemList, &sMartInfo.itemCount);
        BuyMenuBuildItemNames();
        BuyMenuInitBgs();
        BuyMenuInitWindows();
        BuyMenuDecompressBgGraphics();
        gMain.state++;
        break;
    case 1:
        if (!FreeTempTileDataBuffersIfPossible())
            gMain.state++;
        break;
    default:
        BuyMenuDrawGraphics();
        CreateTask(Task_BuyMenu, 8);
        BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB_BuyMenu);
        SetMainCallback2(CB2_BuyMenu);
        break;
    }
}

static void CB2_BuyMenu(void)
{
    RunTasks();
    AdvanceComfyAnimations();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void VBlankCB_BuyMenu(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void BuyMenuInitBgs(void)
{
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sShopBuyMenuBgTemplates, ARRAY_COUNT(sShopBuyMenuBgTemplates));
    SetBgTilemapBuffer(1, sShopData->mapTopTilemap);
    SetBgTilemapBuffer(2, sShopData->mapMidTilemap);
    SetBgTilemapBuffer(3, sShopData->mapBottomTilemap);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BG2HOFS, 0);
    SetGpuReg(REG_OFFSET_BG2VOFS, 0);
    SetGpuReg(REG_OFFSET_BG3HOFS, 0);
    SetGpuReg(REG_OFFSET_BG3VOFS, 0);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    ShowBg(3);
}

static void BuyMenuInitWindows(void)
{
    u32 windowId;

    InitWindows(sShopBuyMenuWindowTemplates);
    DeactivateAllTextPrinters();
    LoadUserWindowBorderGfx(WIN_MESSAGE, SHOP_BASE_STD_BORDER, BG_PLTT_ID(13));
    LoadMessageBoxGfx(WIN_MESSAGE, SHOP_BASE_MSGBOX, BG_PLTT_ID(14));

    for (windowId = WIN_MONEY; windowId <= WIN_QUANTITY_IN_BAG; windowId++)
    {
        SetWindowAttribute(windowId, WINDOW_PALETTE_NUM, SHOP_MENU_PALETTE_ID);
        FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
        PutWindowTilemap(windowId);
    }
}

static void BuyMenuDecompressBgGraphics(void)
{
    DecompressAndCopyTileDataToVram(1, sShopMenu_Gfx, 0, SHOP_MENU_BASE_TILE, 0);
    DecompressDataWithHeaderWram(sShopMenu_Tilemap, sShopData->menuTilemap);
    LoadPalette(sShopMenu_Pal, BG_PLTT_ID(SHOP_MENU_PALETTE_ID), PLTT_SIZE_4BPP);
    BuyMenuLoadSpriteGfx();
}

static void BuyMenuDrawGraphics(void)
{
    BuyMenuDrawMapGraphics();
    BuyMenuCopyMenuBgToBg1TilemapBuffer();
    BuyMenuTintMapView();
    BuyMenuPrintMoney();
    BuyMenuCreateListSprites();
    ShopList_Refresh();
    ScheduleBgCopyTilemapToVram(0);
    ScheduleBgCopyTilemapToVram(1);
    ScheduleBgCopyTilemapToVram(2);
    ScheduleBgCopyTilemapToVram(3);
}

static void BuyMenuDrawMapGraphics(void)
{
    BuyMenuCollectObjectEventData();
    BuyMenuDrawObjectEvents();
    BuyMenuDrawMapBg();
}

// tinting overworld view area
#define MAP_VIEW_TINT_COLOR        RGB(30, 29, 30)
#define MAP_VIEW_TINT_COEFF        10

// apply tint to ow view, both map tileset and obj sprites
static void BuyMenuTintMapView(void)
{
    u32 palettes = ((1 << SHOP_MENU_PALETTE_ID) - 1) | sShopData->tintPalettes;

    BlendPalettesFine(palettes, gPlttBufferUnfaded, gPlttBufferUnfaded, MAP_VIEW_TINT_COEFF, MAP_VIEW_TINT_COLOR);
}

// in metatiles unit
#define VIEW_WIDTH                 15
#define VIEW_HEIGHT                10
#define VIEW_X_OFFSET              1
#define VIEW_Y_OFFSET              3
#define VIEW_OBJECT_SCAN_WIDTH     5
#define VIEW_OBJECT_SCAN_HEIGHT    10

static void BuyMenuDrawMapBg(void)
{
    const struct MapLayout *mapLayout = gMapHeader.mapLayout;
    u16 numMetatilesInPrimary = GetNumMetatilesInPrimary(mapLayout);
    s16 i, j, x, y;

    GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
    x -= VIEW_X_OFFSET;
    y -= VIEW_Y_OFFSET;

    for (j = 0; j < VIEW_HEIGHT; j++)
    {
        for (i = 0; i < VIEW_WIDTH; i++)
        {
            u16 metatile = MapGridGetMetatileIdAt(x + i, y + j);
            u8 metatileLayerType;

            if (BuyMenuCheckForOverlapWithMenuBg(i, j) == TRUE)
                metatileLayerType = MapGridGetMetatileLayerTypeAt(x + i, y + j);
            else
                metatileLayerType = METATILE_LAYER_TYPE_COVERED;

            if (metatile < numMetatilesInPrimary)
                BuyMenuDrawMapMetatile(i, j, mapLayout->primaryTileset->metatiles + metatile * NUM_TILES_PER_METATILE, metatileLayerType);
            else
                BuyMenuDrawMapMetatile(i, j, mapLayout->secondaryTileset->metatiles + ((metatile - numMetatilesInPrimary) * NUM_TILES_PER_METATILE), metatileLayerType);
        }
    }
}

static void BuyMenuDrawMapMetatile(s16 x, s16 y, const u16 *src, u8 metatileLayerType)
{
    u16 offset1 = x * 2;
    u16 offset2 = y * 64;

    switch (metatileLayerType)
    {
    case METATILE_LAYER_TYPE_NORMAL:
        BuyMenuDrawMapMetatileLayer(sShopData->mapMidTilemap, offset1, offset2, src);
        BuyMenuDrawMapMetatileLayer(sShopData->mapTopTilemap, offset1, offset2, src + 4);
        break;
    case METATILE_LAYER_TYPE_COVERED:
        BuyMenuDrawMapMetatileLayer(sShopData->mapBottomTilemap, offset1, offset2, src);
        BuyMenuDrawMapMetatileLayer(sShopData->mapMidTilemap, offset1, offset2, src + 4);
        break;
    case METATILE_LAYER_TYPE_SPLIT:
        BuyMenuDrawMapMetatileLayer(sShopData->mapBottomTilemap, offset1, offset2, src);
        BuyMenuDrawMapMetatileLayer(sShopData->mapTopTilemap, offset1, offset2, src + 4);
        break;
    }
}

static void BuyMenuDrawMapMetatileLayer(u16 *dest, s16 offset1, s16 offset2, const u16 *src)
{
    dest[offset1 + offset2] = src[0];
    dest[offset1 + offset2 + 1] = src[1];
    dest[offset1 + offset2 + 32] = src[2];
    dest[offset1 + offset2 + 33] = src[3];
}

static void BuyMenuCollectObjectEventData(void)
{
    s16 originX, originY;
    u8 x, y;
    u8 numObjects = 0;

    GetXYCoordsOneStepInFrontOfPlayer(&originX, &originY);
    originX -= VIEW_X_OFFSET;
    originY -= VIEW_Y_OFFSET;

    for (y = 0; y < OBJECT_EVENTS_COUNT; y++)
        sShopData->viewportObjects[y][OBJ_EVENT_ID] = OBJECT_EVENTS_COUNT;

    for (y = 0; y < VIEW_OBJECT_SCAN_HEIGHT; y++)
    {
        for (x = 0; x < VIEW_OBJECT_SCAN_WIDTH; x++)
        {
            u8 objEventId = GetObjectEventIdByXY(originX + x, originY + y);

            // skip if invalid or an overworld Pokémon that is not following the player
            if (objEventId != OBJECT_EVENTS_COUNT && !(gObjectEvents[objEventId].active && gObjectEvents[objEventId].graphicsId & OBJ_EVENT_MON && gObjectEvents[objEventId].localId != OBJ_EVENT_ID_FOLLOWER))
            {
                sShopData->viewportObjects[numObjects][OBJ_EVENT_ID] = objEventId;
                sShopData->viewportObjects[numObjects][X_COORD] = x;
                sShopData->viewportObjects[numObjects][Y_COORD] = y;

                switch (gObjectEvents[objEventId].facingDirection)
                {
                case DIR_SOUTH:
                    sShopData->viewportObjects[numObjects][ANIM_NUM] = ANIM_STD_FACE_SOUTH;
                    break;
                case DIR_NORTH:
                    sShopData->viewportObjects[numObjects][ANIM_NUM] = ANIM_STD_FACE_NORTH;
                    break;
                case DIR_WEST:
                    sShopData->viewportObjects[numObjects][ANIM_NUM] = ANIM_STD_FACE_WEST;
                    break;
                case DIR_EAST:
                default:
                    sShopData->viewportObjects[numObjects][ANIM_NUM] = ANIM_STD_FACE_EAST;
                    break;
                }
                numObjects++;
            }
        }
    }
}

static void BuyMenuDrawObjectEvents(void)
{
    u8 i;
    u8 spriteId;
    const struct ObjectEventGraphicsInfo *graphicsInfo;
    u8 weatherTemp = gWeatherPtr->palProcessingState;

    // This function runs during fadeout, so the weather palette processing state must be temporarily changed,
    // so that time-blending will work properly
    if (weatherTemp == WEATHER_PAL_STATE_SCREEN_FADING_OUT)
        gWeatherPtr->palProcessingState = WEATHER_PAL_STATE_IDLE;
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (sShopData->viewportObjects[i][OBJ_EVENT_ID] == OBJECT_EVENTS_COUNT)
            continue;

        graphicsInfo = GetObjectEventGraphicsInfo(gObjectEvents[sShopData->viewportObjects[i][OBJ_EVENT_ID]].graphicsId);

        spriteId = CreateObjectGraphicsSprite(
            gObjectEvents[sShopData->viewportObjects[i][OBJ_EVENT_ID]].graphicsId,
            SpriteCallbackDummy,
            (u16)sShopData->viewportObjects[i][X_COORD] * 16 + 8,
            (u16)sShopData->viewportObjects[i][Y_COORD] * 16 + 16 - graphicsInfo->height / 2,
            2);

        // only tint these sprites/pals from ow redrawing
        sShopData->tintPalettes |= 1 << (16 + gSprites[spriteId].oam.paletteNum);

        StartSpriteAnim(&gSprites[spriteId], sShopData->viewportObjects[i][ANIM_NUM]);
    }

    gWeatherPtr->palProcessingState = weatherTemp; // restore weather state
    CpuFastCopy(gPlttBufferFaded + 16*16, gPlttBufferUnfaded + 16*16, PLTT_BUFFER_SIZE);
}

static void BuyMenuCopyMenuBgToBg1TilemapBuffer(void)
{
    u32 i;
    u16 *dest = sShopData->mapTopTilemap;
    const u16 *src = sShopData->menuTilemap;

    for (i = 0; i < ARRAY_COUNT(sShopData->menuTilemap); i++)
    {
        if (src[i] != 0)
            dest[i] = src[i] + ((SHOP_MENU_PALETTE_ID << 12) | SHOP_MENU_BASE_TILE);
    }
}

static bool8 BuyMenuCheckForOverlapWithMenuBg(int x, int y)
{
    const u16 *metatile = sShopData->menuTilemap;
    int offset1 = x * 2;
    int offset2 = y * 64;

    if (metatile[offset2 + offset1] == 0 &&
        metatile[offset2 + offset1 + 32] == 0 &&
        metatile[offset2 + offset1 + 1] == 0 &&
        metatile[offset2 + offset1 + 33] == 0)
        return TRUE;

    return FALSE;
}

static void BuyMenuPrint(u8 windowId, u8 fontId, const u8 *str, u8 x, u8 y, u8 letterSpacing, u8 lineSpacing, u8 speed, u8 colorId)
{
    AddTextPrinterParameterized4(windowId, fontId, x, y, letterSpacing, lineSpacing, sFontColorTable[colorId], speed, str);
}

static u16 BuyMenuGetEntryId(u32 index)
{
    return sMartInfo.itemList[index];
}

#define MAX_ITEMS_SHOWN            6
#define LIST_FONT                  FONT_NARROW
#define LIST_TOP_Y                 0
#define LIST_ROW_PADDING           0
#define LIST_ROW_NAME_X            2
#define LIST_ROW_NAME_MAX_WIDTH    86

static void BuyMenuBuildItemNames(void)
{
    u32 i;

    sShopData->itemNames = Alloc(sMartInfo.itemCount * sizeof(*sShopData->itemNames));
    for (i = 0; i < sMartInfo.itemCount; i++)
    {
        u8 *end;

        if (sMartInfo.martType == MART_TYPE_NORMAL)
            end = CopyItemName(BuyMenuGetEntryId(i), sShopData->itemNames[i]);
        else
            end = StringCopy(sShopData->itemNames[i], gDecorations[BuyMenuGetEntryId(i)].name);

        PrependFontIdToFit(sShopData->itemNames[i], end, LIST_FONT, LIST_ROW_NAME_MAX_WIDTH);
    }

    sShopData->listTotal = sMartInfo.itemCount;
    sShopData->listShown = min(sMartInfo.itemCount, MAX_ITEMS_SHOWN);
}

static u32 BuyMenuGetItemPrice(u32 itemId)
{
    if (sMartInfo.martType != MART_TYPE_NORMAL)
        return gDecorations[itemId].price;

    return GetItemPrice(itemId) >> IsPokeNewsActive(POKENEWS_SLATEPORT);
}

static bool32 BuyMenuIsSoldOut(u32 itemId)
{
    if (sMartInfo.martType != MART_TYPE_NORMAL)
        return FALSE;

    return GetItemImportance(itemId) && (CheckBagHasItem(itemId, 1) || CheckPCHasItem(itemId, 1));
}

#define MONEY_TEXT_Y               3

static void BuyMenuPrintMoney(void)
{
    u8 windowWidth = sShopBuyMenuWindowTemplates[WIN_MONEY].width * 8;

    FillWindowPixelBuffer(WIN_MONEY, PIXEL_FILL(0));
    BuyMenuPrint(WIN_MONEY, LIST_FONT, COMPOUND_STRING("Money"), 0, MONEY_TEXT_Y, 0, 0, TEXT_SKIP_DRAW, COLORID_MONEY);
    ConvertMoneyToCommaString(gStringVar1, GetMoney(&gSaveBlock1Ptr->money));
    StringExpandPlaceholders(gStringVar4, gText_PokedollarVar1);
    BuyMenuPrint(WIN_MONEY, LIST_FONT, gStringVar4,
                 GetStringRightAlignXOffset(LIST_FONT, gStringVar4, windowWidth), MONEY_TEXT_Y, 0, 0,
                 TEXT_SKIP_DRAW, COLORID_MONEY);
    CopyWindowToVram(WIN_MONEY, COPYWIN_GFX);
}

static void BuyMenuPrintPriceInList(u32 itemId, u8 y, bool32 isHovered)
{
    u8 windowWidth = sShopBuyMenuWindowTemplates[WIN_ITEM_LIST].width * 8;

    if (BuyMenuIsSoldOut(itemId))
    {
        StringCopy(gStringVar4, gText_SoldOut);
    }
    else
    {
        ConvertMoneyToCommaString(gStringVar1, BuyMenuGetItemPrice(itemId));
        StringExpandPlaceholders(gStringVar4, gText_PokedollarVar1);
    }

    BuyMenuPrint(WIN_ITEM_LIST, LIST_FONT, gStringVar4,
                 GetStringRightAlignXOffset(LIST_FONT, gStringVar4, windowWidth), y, 0, 0,
                 TEXT_SKIP_DRAW, isHovered ? COLORID_HOVER_PRICE : COLORID_NORMAL);
}

#define DESCRIPTION_TOP_Y          1

static void BuyMenuPrintItemDescription(u32 itemId)
{
    s32 maxWidth = sShopBuyMenuWindowTemplates[WIN_ITEM_DESCRIPTION].width * 8;
    const u8 *str;
    u8 fontId;

    if (sMartInfo.martType == MART_TYPE_NORMAL)
        str = GetItemDescription(itemId);
    else
        str = gDecorations[itemId].description;

    fontId = FormatDescriptionByWidth(sShopData->descriptionBuffer, DESCRIPTION_BUFFER_SIZE, maxWidth,
                                      FONT_SHORT_NARROW, str,
                                      GetFontAttribute(FONT_SHORT_NARROW, FONTATTR_LETTER_SPACING));
    FillWindowPixelBuffer(WIN_ITEM_DESCRIPTION, PIXEL_FILL(0));
    BuyMenuPrint(WIN_ITEM_DESCRIPTION, fontId, sShopData->descriptionBuffer, 0, DESCRIPTION_TOP_Y, 0, 1,
                 TEXT_SKIP_DRAW, COLORID_NORMAL);
    CopyWindowToVram(WIN_ITEM_DESCRIPTION, COPYWIN_GFX);
}

static void BuyMenuPrintQuantityInBag(u32 itemId)
{
    u8 windowWidth = sShopBuyMenuWindowTemplates[WIN_QUANTITY_IN_BAG].width * 8;

    FillWindowPixelBuffer(WIN_QUANTITY_IN_BAG, PIXEL_FILL(0));

    if (sMartInfo.martType == MART_TYPE_NORMAL)
    {
        ConvertIntToDecimalStringN(gStringVar1, CountTotalItemQuantityInBag(itemId),
                                   STR_CONV_MODE_RIGHT_ALIGN, MAX_ITEM_DIGITS);
        StringExpandPlaceholders(gStringVar4, gText_xVar1);
        BuyMenuPrint(WIN_QUANTITY_IN_BAG, LIST_FONT, gStringVar4,
                     GetStringRightAlignXOffset(LIST_FONT, gStringVar4, windowWidth), 0, 0, 0,
                     TEXT_SKIP_DRAW, COLORID_NORMAL);
    }

    CopyWindowToVram(WIN_QUANTITY_IN_BAG, COPYWIN_GFX);
}

static u8 ShopList_RowHeight(void)
{
    return GetFontAttribute(LIST_FONT, FONTATTR_MAX_LETTER_HEIGHT) + LIST_ROW_PADDING;
}

static void ShopList_RefreshRow(u8 row)
{
    u8 windowWidth = sShopBuyMenuWindowTemplates[WIN_ITEM_LIST].width * 8;
    u32 index = sShopData->scrollOffset + row;
    bool32 isHovered;
    u8 rowHeight;
    u8 rowY;

    if (index >= sShopData->listTotal)
        return;

    isHovered = ((s32)index == sShopData->hoveredIndex);
    rowHeight = ShopList_RowHeight();
    rowY = LIST_TOP_Y + row * rowHeight;
    FillWindowPixelRect(WIN_ITEM_LIST, PIXEL_FILL(0), 0, rowY, windowWidth, rowHeight);
    BuyMenuPrint(WIN_ITEM_LIST, LIST_FONT, sShopData->itemNames[index], LIST_ROW_NAME_X, rowY, 0, 0,
                 TEXT_SKIP_DRAW, isHovered ? COLORID_HOVER_NAME : COLORID_NORMAL);
    BuyMenuPrintPriceInList(BuyMenuGetEntryId(index), rowY, isHovered);
}

static void ShopList_RefreshColors(void)
{
    u8 row;

    for (row = 0; row < sShopData->listShown; row++)
    {
        if (sShopData->scrollOffset + row >= sShopData->listTotal)
            break;
        ShopList_RefreshRow(row);
    }
}

static void ShopList_ScrollRows(bool32 movingDown)
{
    u8 rowHeight = ShopList_RowHeight();
    u16 windowWidth = sShopBuyMenuWindowTemplates[WIN_ITEM_LIST].width * 8;
    u16 windowHeight = sShopBuyMenuWindowTemplates[WIN_ITEM_LIST].height * 8;

    if (movingDown)
    {
        ScrollWindow(WIN_ITEM_LIST, 0, rowHeight, PIXEL_FILL(0));
        ShopList_RefreshRow(sShopData->listShown - 1);
    }
    else
    {
        u16 y = sShopData->listShown * rowHeight + LIST_TOP_Y;

        ScrollWindow(WIN_ITEM_LIST, 1, rowHeight, PIXEL_FILL(0));
        ShopList_RefreshRow(0);
        if (y < windowHeight)
            FillWindowPixelRect(WIN_ITEM_LIST, PIXEL_FILL(0), 0, y, windowWidth, windowHeight - y);
    }
}

static void ShopList_SeekTo(u16 absPos)
{
    u16 total = sShopData->listTotal;
    u16 shown = sShopData->listShown;
    u16 top = 0;

    if (total > shown)
    {
        u16 halfScreen = shown / 2;
        u16 maxTop = total - shown;

        if (absPos > halfScreen)
            top = min(absPos - halfScreen, maxTop);
    }

    sShopData->scrollOffset = top;
    sShopData->selectedRow = absPos - top;
}

static void ShopList_Step(bool32 movingDown, bool32 allowWrap)
{
    u16 total = sShopData->listTotal;
    u16 abs = sShopData->scrollOffset + sShopData->selectedRow;

    if (total == 0)
        return;

    if (movingDown)
    {
        if (abs < total - 1u)
            abs++;
        else if (allowWrap)
            abs = 0;
    }
    else
    {
        if (abs != 0)
            abs--;
        else if (allowWrap)
            abs = total - 1u;
    }

    ShopList_SeekTo(abs);
}

static void ShopList_Refresh(void)
{
    PutWindowTilemap(WIN_ITEM_LIST);
    FillWindowPixelBuffer(WIN_ITEM_LIST, PIXEL_FILL(0));
    BuyMenuMoveCursorCallback(sShopData->scrollOffset + sShopData->selectedRow, TRUE);
    CopyWindowToVram(WIN_ITEM_LIST, COPYWIN_GFX);
}

static void ShopList_Move(bool32 movingDown, bool32 allowWrap)
{
    u16 oldScroll = sShopData->scrollOffset;
    u16 oldAbs = oldScroll + sShopData->selectedRow;

    ShopList_Step(movingDown, allowWrap);

    if (sShopData->scrollOffset + sShopData->selectedRow == oldAbs)
        return;

    if (sShopData->scrollOffset != oldScroll)
        ShopList_ScrollRows(sShopData->scrollOffset > oldScroll);

    BuyMenuMoveCursorCallback(sShopData->scrollOffset + sShopData->selectedRow, FALSE);
    CopyWindowToVram(WIN_ITEM_LIST, COPYWIN_GFX);
}

static s32 ShopList_ProcessInput(void)
{
    if (JOY_NEW(A_BUTTON))
        return BuyMenuGetEntryId(sShopData->scrollOffset + sShopData->selectedRow);

    if (JOY_NEW(B_BUTTON))
        return LIST_CANCEL;

    if (JOY_REPEAT(DPAD_UP))
        ShopList_Move(FALSE, JOY_NEW(DPAD_UP));
    else if (JOY_REPEAT(DPAD_DOWN))
        ShopList_Move(TRUE, JOY_NEW(DPAD_DOWN));

    return LIST_NOTHING_CHOSEN;
}

#define LIST_SPRITE_ROW_Y_OFFSET     8
#define ITEM_ICON_X                  80
#define ITEM_ICON_TO_CURSOR_X        22
#define ITEM_ICON_Y_OFFSET           4
#define LIST_CURSOR_X                (ITEM_ICON_X - ITEM_ICON_TO_CURSOR_X)
#define HOVER_SLOT_X                 80
#define HOVER_SLOT_SPACING           32

#define SUBPRIORITY_SPINNER_ARROW    1
#define SUBPRIORITY_QUANTITY_FRAME   2
#define SUBPRIORITY_ITEM_ICON        3
#define SUBPRIORITY_CURSOR           4
#define SUBPRIORITY_HOVER_SLOT       5
#define SUBPRIORITY_SCROLL_THUMB     6

static s16 ShopList_RowSpriteY(u16 row)
{
    return sShopBuyMenuWindowTemplates[WIN_ITEM_LIST].tilemapTop * 8
         + LIST_TOP_Y + row * ShopList_RowHeight() + LIST_SPRITE_ROW_Y_OFFSET;
}

#define SCROLL_THUMB_X          236
#define SCROLL_TRACK_LEN        (WIN_ITEM_LIST_H * 8)
#define SCROLL_THUMB_MIN_LEN    8

static u8 ScrollThumb_Length(u16 total)
{
    if (total <= MAX_ITEMS_SHOWN)
        return 0;
    return max(SCROLL_TRACK_LEN * MAX_ITEMS_SHOWN / total, SCROLL_THUMB_MIN_LEN);
}

static s16 ScrollThumb_Offset(u16 total, u16 absIdx)
{
    u8 len = ScrollThumb_Length(total);

    if (len == 0)
        return 0;
    return absIdx * (SCROLL_TRACK_LEN - len) / (total - 1);
}

static u8 ScrollThumb_SegmentSize(u8 len)
{
    if (len >= 32)
        return 32;
    if (len >= 16)
        return 16;
    return 8;
}

static u8 ScrollThumb_SegmentCount(u8 len)
{
    u8 size = ScrollThumb_SegmentSize(len);

    return (len + size - 1) / size;
}

static u8 ScrollThumb_SegmentOffset(u8 len, u8 index)
{
    u8 size = ScrollThumb_SegmentSize(len);

    return (index == ScrollThumb_SegmentCount(len) - 1) ? len - size : index * size;
}

static void ScrollThumb_SetSegmentOam(struct Sprite *sprite, u8 size)
{
    switch (size)
    {
    case 32:
        sprite->oam.shape = SPRITE_SHAPE(8x32);
        sprite->oam.size = SPRITE_SIZE(8x32);
        break;
    case 16:
        sprite->oam.shape = SPRITE_SHAPE(8x16);
        sprite->oam.size = SPRITE_SIZE(8x16);
        break;
    default:
        sprite->oam.shape = SPRITE_SHAPE(8x8);
        sprite->oam.size = SPRITE_SIZE(8x8);
        break;
    }
    CalcCenterToCornerVec(sprite, sprite->oam.shape, sprite->oam.size, sprite->oam.affineMode);
}

static void SpriteCB_ScrollThumb(struct Sprite *sprite)
{
    u16 total = sShopData->listTotal;
    u8 len = ScrollThumb_Length(total);
    u8 index = sprite->data[0];
    u8 size;

    if (len == 0 || index >= ScrollThumb_SegmentCount(len))
    {
        sprite->invisible = TRUE;
        return;
    }

    size = ScrollThumb_SegmentSize(len);
    ScrollThumb_SetSegmentOam(sprite, size);
    sprite->y = sShopBuyMenuWindowTemplates[WIN_ITEM_LIST].tilemapTop * 8 + size / 2
              + ScrollThumb_SegmentOffset(len, index);
    sprite->invisible = FALSE;
    if (sShopData->scrollThumbAnimId != INVALID_COMFY_ANIM)
        sprite->y2 = ReadComfyAnimValueSmooth(&gComfyAnims[sShopData->scrollThumbAnimId]);
}

#define CURSOR_BOB_RANGE    3
#define CURSOR_BOB_FRAMES   20
#define sBobTarget data[0]

static void SpriteCB_CursorBob(struct Sprite *sprite)
{
    struct ComfyAnim *bob;

    if (sShopData->cursorBobAnimId == INVALID_COMFY_ANIM)
        return;

    bob = &gComfyAnims[sShopData->cursorBobAnimId];

    if (bob->completed && sprite->x2 == sprite->sBobTarget)
    {
        sprite->sBobTarget = (sprite->sBobTarget == 0) ? CURSOR_BOB_RANGE : 0;
        InitComfyAnim_Easing(&(struct ComfyAnimEasingConfig){
            .from = Q_24_8(sprite->x2),
            .to = Q_24_8(sprite->sBobTarget),
            .durationFrames = CURSOR_BOB_FRAMES,
            .easingFunc = ComfyAnimEasing_EaseInOutQuad,
        }, bob);
        TryAdvanceComfyAnim(bob);
    }

    sprite->x2 = ReadComfyAnimValueSmooth(bob);
}

static void StartCursorBob(u8 spriteId)
{
    struct ComfyAnimEasingConfig config = {
        .from = Q_24_8(0),
        .to = Q_24_8(CURSOR_BOB_RANGE),
        .durationFrames = CURSOR_BOB_FRAMES,
        .easingFunc = ComfyAnimEasing_EaseInOutQuad,
    };

    if (sShopData->cursorBobAnimId == INVALID_COMFY_ANIM)
        sShopData->cursorBobAnimId = CreateComfyAnim_Easing(&config);
    else
        InitComfyAnim_Easing(&config, &gComfyAnims[sShopData->cursorBobAnimId]);

    gSprites[spriteId].x2 = 0;
    gSprites[spriteId].sBobTarget = CURSOR_BOB_RANGE;
}

static void SpriteCB_SlideCursorY(struct Sprite *sprite)
{
    s16 y;
    u8 i;

    SpriteCB_CursorBob(sprite);

    if (sShopData->cursorAnimId == INVALID_COMFY_ANIM)
        return;

    y = ReadComfyAnimValueSmooth(&gComfyAnims[sShopData->cursorAnimId]);
    sprite->y = y;

    for (i = 0; i < HOVER_SLOT_SPRITES_COUNT; i++)
    {
        if (sShopData->hoverSlotSpriteIds[i] != SPRITE_NONE)
            gSprites[sShopData->hoverSlotSpriteIds[i]].y = y;
    }

    for (i = 0; i < ITEM_ICON_SLOT_COUNT; i++)
    {
        if (sShopData->itemSpriteIds[i] != SPRITE_NONE)
            gSprites[sShopData->itemSpriteIds[i]].y2 = y + ITEM_ICON_Y_OFFSET;
    }
}

#undef CURSOR_BOB_RANGE
#undef CURSOR_BOB_FRAMES
#undef sBobTarget

static void BuyMenuLoadSpriteGfx(void)
{
    LoadCompressedSpriteSheet(&sSpriteSheet_Cursor);
    LoadCompressedSpriteSheet(&sSpriteSheet_HoverSlot);
    LoadCompressedSpriteSheet(&sSpriteSheet_ScrollThumb);
    LoadCompressedSpriteSheet(&sSpriteSheet_QuantityFrame);
    LoadCompressedSpriteSheet(&sSpriteSheet_SpinnerArrow);
    LoadSpritePalettes(sShopSpritePalettes);
}

static void BuyMenuCreateListSprites(void)
{
    s16 initialY = ShopList_RowSpriteY(sShopData->selectedRow);
    s16 initialThumbY2 = ScrollThumb_Offset(sShopData->listTotal, sShopData->scrollOffset + sShopData->selectedRow);
    u8 i;

    sShopData->cursorAnimId = CreateComfyAnim_Easing(&(struct ComfyAnimEasingConfig){
        .from = Q_24_8(initialY),
        .to = Q_24_8(initialY),
        .durationFrames = 1,
        .easingFunc = ComfyAnimEasing_EaseOutCubic,
    });
    sShopData->scrollThumbAnimId = CreateComfyAnim_Easing(&(struct ComfyAnimEasingConfig){
        .from = Q_24_8(initialThumbY2),
        .to = Q_24_8(initialThumbY2),
        .durationFrames = 1,
        .easingFunc = ComfyAnimEasing_EaseOutCubic,
    });

    for (i = 0; i < HOVER_SLOT_SPRITES_COUNT; i++)
    {
        u8 spriteId = CreateSprite(&sSpriteTemplate_HoverSlot, HOVER_SLOT_X + i * HOVER_SLOT_SPACING,
                                   initialY, SUBPRIORITY_HOVER_SLOT);

        sShopData->hoverSlotSpriteIds[i] = spriteId;
        StartSpriteAnim(&gSprites[spriteId], sHoverSlotAnims[i]);
    }

    for (i = 0; i < SCROLL_THUMB_SPRITES_COUNT; i++)
    {
        u8 spriteId = CreateSprite(&sSpriteTemplate_ScrollThumb, SCROLL_THUMB_X, 0, SUBPRIORITY_SCROLL_THUMB);

        sShopData->scrollThumbSpriteIds[i] = spriteId;
        gSprites[spriteId].data[0] = i;
        gSprites[spriteId].invisible = TRUE;
    }

    sShopData->cursorSpriteId = CreateSprite(&sSpriteTemplate_Cursor, LIST_CURSOR_X, initialY,
                                             SUBPRIORITY_CURSOR);
    StartCursorBob(sShopData->cursorSpriteId);
}

static void BuyMenuAddItemIcon(u32 itemId, u8 iconSlot, s16 spriteY)
{
    u8 *spriteIdPtr = &sShopData->itemSpriteIds[iconSlot];
    u8 spriteId;

    if (*spriteIdPtr != SPRITE_NONE)
        return;

    if (sMartInfo.martType == MART_TYPE_NORMAL || itemId == ITEM_LIST_END)
    {
        struct Sprite *sprite;

        spriteId = AddItemIconSprite(iconSlot + TAG_ITEM_ICON_BASE, iconSlot + TAG_ITEM_ICON_BASE, itemId);
        if (spriteId == MAX_SPRITES)
            return;

        sprite = &gSprites[spriteId];
        sprite->x2 = ITEM_ICON_X;
        sprite->y2 = spriteY + ITEM_ICON_Y_OFFSET;
        sprite->subpriority = SUBPRIORITY_ITEM_ICON;
        sprite->oam.affineMode = ST_OAM_AFFINE_NORMAL;
        sprite->affineAnims = sAffineAnims_ItemIcon;
        InitSpriteAffineAnim(sprite);
        StartSpriteAffineAnim(sprite, 0);
    }
    else
    {
        spriteId = AddDecorationIconObject(itemId, ITEM_ICON_X - 4, spriteY + ITEM_ICON_Y_OFFSET - 4, 1,
                                           iconSlot + TAG_ITEM_ICON_BASE, iconSlot + TAG_ITEM_ICON_BASE);
        if (spriteId == MAX_SPRITES)
            return;

        gSprites[spriteId].subpriority = SUBPRIORITY_ITEM_ICON;
    }

    *spriteIdPtr = spriteId;
}

static void BuyMenuRemoveItemIcon(u8 iconSlot)
{
    u8 *spriteIdPtr = &sShopData->itemSpriteIds[iconSlot];

    if (*spriteIdPtr == SPRITE_NONE)
        return;

    FreeSpriteOamMatrix(&gSprites[*spriteIdPtr]);
    FreeSpriteTilesByTag(iconSlot + TAG_ITEM_ICON_BASE);
    FreeSpritePaletteByTag(iconSlot + TAG_ITEM_ICON_BASE);
    DestroySprite(&gSprites[*spriteIdPtr]);
    *spriteIdPtr = SPRITE_NONE;
}

static void BuyMenuMoveCursorCallback(u32 index, bool32 onInit)
{
    s32 oldHovered = sShopData->hoveredIndex;
    s16 spriteY = ShopList_RowSpriteY(sShopData->selectedRow);
    u32 durationFrames = 8;

    sShopData->hoveredIndex = index;

    if (sShopData->cursorAnimId != INVALID_COMFY_ANIM)
    {
        struct ComfyAnim *cursorAnim = &gComfyAnims[sShopData->cursorAnimId];

        if (!onInit && !cursorAnim->completed)
            durationFrames = (gMain.heldKeys & (DPAD_UP | DPAD_DOWN)) ? 1 : 2;

        InitComfyAnim_Easing(&(struct ComfyAnimEasingConfig){
            .from = cursorAnim->position,
            .to = Q_24_8(spriteY),
            .durationFrames = durationFrames,
            .easingFunc = ComfyAnimEasing_EaseOutCubic,
        }, cursorAnim);
    }

    if (sShopData->listTotal > MAX_ITEMS_SHOWN && sShopData->scrollThumbAnimId != INVALID_COMFY_ANIM)
    {
        struct ComfyAnim *thumbAnim = &gComfyAnims[sShopData->scrollThumbAnimId];
        s32 maxOffset = Q_24_8(SCROLL_TRACK_LEN - ScrollThumb_Length(sShopData->listTotal));

        InitComfyAnim_Easing(&(struct ComfyAnimEasingConfig){
            .from = min(thumbAnim->position, maxOffset),
            .to = Q_24_8(ScrollThumb_Offset(sShopData->listTotal, index)),
            .durationFrames = durationFrames,
            .easingFunc = ComfyAnimEasing_EaseOutCubic,
        }, thumbAnim);
    }

    if (!onInit && (oldHovered - (s32)index == 1 || (s32)index - oldHovered == 1))
    {
        s32 oldRow = oldHovered - (s32)sShopData->scrollOffset;

        if (oldRow >= 0 && oldRow < sShopData->listShown)
            ShopList_RefreshRow(oldRow);
        ShopList_RefreshRow(sShopData->selectedRow);
    }
    else
    {
        ShopList_RefreshColors();
    }

    BuyMenuPrintItemDescription(BuyMenuGetEntryId(index));
    BuyMenuPrintQuantityInBag(BuyMenuGetEntryId(index));

    {
        u32 iconItemId = BuyMenuGetEntryId(index);

        if (iconItemId != sShopData->shownItemId
         || sShopData->itemSpriteIds[sShopData->iconSlot ^ 1] == SPRITE_NONE)
        {
            BuyMenuAddItemIcon(iconItemId, sShopData->iconSlot, spriteY);
            BuyMenuRemoveItemIcon(sShopData->iconSlot ^ 1);
            sShopData->iconSlot ^= 1;
            sShopData->shownItemId = iconItemId;
        }
    }

    if (!onInit)
        PlaySE(SE_SELECT);
}

// add a comma every three digits, e.g. 123456 -> "123,456".
static void ConvertMoneyToCommaString(u8 *dest, u32 amount)
{
    u8 digits[MAX_MONEY_DIGITS];
    u8 count = 0;
    s8 i;

    do
    {
        digits[count++] = amount % 10;
        amount /= 10;
    } while (amount != 0 && count < MAX_MONEY_DIGITS);

    for (i = count - 1; i >= 0; i--)
    {
        *dest++ = CHAR_0 + digits[i];
        if (i != 0 && i % 3 == 0)
            *dest++ = CHAR_COMMA;
    }
    *dest = EOS;
}

static const struct {
    const char *before;
    const char *after;
} sHyphenRemovalPatterns[] = {
    {"Incine", "roar"},
    {"La",     "riat"},
    {"Marsha", "dow"},
    {"Thi",    "ef"},
    {"Elec",   "tric"},
    {"Fight",  "ing"},
    {"pro",    "motes"},
    {"Decidu", "eye"},
    {"Sha",    "ckle"},
    {"invigor","ating"},
    {"Thunder","bolt"},
    {"inde",   "scribable"},
};

static u8 AsciiToGbaChar(char c)
{
    if (c >= 'A' && c <= 'Z') return CHAR_A + (c - 'A');
    if (c >= 'a' && c <= 'z') return CHAR_a + (c - 'a');
    if (c >= '0' && c <= '9') return CHAR_0 + (c - '0');
    return c;
}

static bool32 ShouldRemoveHyphen(const u8 *p, const u8 *start, const u8 *end)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sHyphenRemovalPatterns); i++)
    {
        const char *before = sHyphenRemovalPatterns[i].before;
        const char *after  = sHyphenRemovalPatterns[i].after;
        u32 beforeLen = 0, afterLen = 0;
        u32 j;
        bool32 matches;

        while (before[beforeLen]) beforeLen++;
        while (after[afterLen])  afterLen++;

        if (p < start + beforeLen)
            continue;

        matches = TRUE;
        for (j = 0; j < beforeLen; j++)
        {
            if (p[-(s32)beforeLen + j] != AsciiToGbaChar(before[j]))
            {
                matches = FALSE;
                break;
            }
        }
        if (!matches)
            continue;

        for (j = 0; j < afterLen; j++)
        {
            if (p[1 + j] != AsciiToGbaChar(after[j]))
            {
                matches = FALSE;
                break;
            }
        }
        if (matches)
            return TRUE;
    }

    // Special case: Poké-mon
    if (p >= start + 4 &&
        p[-4] == CHAR_P && p[-3] == CHAR_o && p[-2] == CHAR_k && p[-1] == CHAR_e_ACUTE &&
        p[1]  == CHAR_m && p[2]  == CHAR_o && p[3]  == CHAR_n)
        return TRUE;

    return FALSE;
}

static bool32 PerformTextFormatting(u8 *result, s32 resultSize, s32 maxWidth, u8 fontId, const u8 *str, s16 letterSpacing, u32 *outLineCount)
{
    u8 *end, *ptr, *curLine, *lastSpace;
    u8 *limit = result + resultSize - 1;

    end = result;
    while (*str != EOS && end < limit)
    {
        if (*str == CHAR_SPACE || *str == CHAR_NEWLINE)
        {
            if (!(*str == CHAR_NEWLINE && end > result && *(end - 1) == CHAR_HYPHEN))
            {
                *end = EOS;
                end++;
            }
        }
        else
        {
            *end = *str;
            end++;
        }
        str++;
    }
    *end = EOS;

    {
        u8 *p = result;
        while (p < end)
        {
            if (*p == CHAR_HYPHEN && ShouldRemoveHyphen(p, result, end))
            {
                u8 *dst = p;
                u8 *src = p + 1;
                while (src <= end)
                    *dst++ = *src++;
                end--;
            }
            else
            {
                p++;
            }
        }
    }

    ptr = result;
    curLine = ptr;
    *outLineCount = 1;

    while (*ptr != EOS) ptr++;

    while (ptr != end)
    {
        lastSpace = ptr++;
        *lastSpace = CHAR_SPACE;
        if (GetStringWidth(fontId, curLine, letterSpacing) > maxWidth)
        {
            *lastSpace = CHAR_NEWLINE;
            (*outLineCount)++;
            curLine = ptr;
        }
        while (*ptr != EOS) ptr++;
    }

    return (GetStringWidth(fontId, curLine, letterSpacing) <= maxWidth);
}

static u8 FormatDescriptionByWidth(u8 *result, s32 resultSize, s32 maxWidth, u8 fontId, const u8 *str, s16 letterSpacing)
{
    u32 lineCount;
    bool32 lastLineFits;

    while (TRUE)
    {
        lastLineFits = PerformTextFormatting(result, resultSize, maxWidth, fontId, str, letterSpacing, &lineCount);

        if (lineCount < 3 && lastLineFits)
            break;

        if (fontId == FONT_SHORT_NARROW)
        {
            fontId = FONT_SHORT_NARROWER;
            letterSpacing = GetFontAttribute(fontId, FONTATTR_LETTER_SPACING);
        }
        else
        {
            break;
        }
    }

    return fontId;
}
#define tItemCount  data[1]
#define tItemId     data[5]

static void BuyMenuDisplayMessage(u8 taskId, const u8 *text, TaskFunc callback)
{
    ClearWindowTilemap(WIN_ITEM_DESCRIPTION);
    ClearWindowTilemap(WIN_QUANTITY_IN_BAG);
    DisplayMessageAndContinueTask(taskId, WIN_MESSAGE, SHOP_BASE_MSGBOX, 14, FONT_NORMAL, GetPlayerTextSpeedDelay(), text, callback);
    ScheduleBgCopyTilemapToVram(0);
}

static void BuyMenuReturnToItemList(u8 taskId)
{
    ClearDialogWindowAndFrameToTransparent(WIN_MESSAGE, FALSE);
    PutWindowTilemap(WIN_ITEM_DESCRIPTION);
    PutWindowTilemap(WIN_QUANTITY_IN_BAG);
    ShopList_Refresh();
    ScheduleBgCopyTilemapToVram(0);
    gTasks[taskId].func = Task_BuyMenu;
}

#define QUANTITY_FRAME_X            144
#define QUANTITY_FRAME_SPACING      64
#define QUANTITY_FRAME_Y            96
#define QUANTITY_SPINNER_X          152

#define SPINNER_ARROW_Y_OFFSET      10
#define SPINNER_ARROW_ANIM_FRAMES   3

#define QUANTITY_FILL_INDEX         13
#define QUANTITY_COUNT_LEFT         16
#define QUANTITY_COUNT_RIGHT        48
#define QUANTITY_COUNT_TOP          8
#define QUANTITY_TOTAL_RIGHT        48

#define sDir    data[0]  // -1 = up, +1 = down
#define sMode   data[1]
#define sTimer  data[2]
#define sStep   data[3]

static const u8 sSpinnerArrowOffsets[] = {0, 1, 2, 1};

static void SpriteCB_SpinnerArrow(struct Sprite *sprite)
{
    if (sprite->sMode == SPINNER_ARROW_STATIC)
        return;

    if (++sprite->sTimer < SPINNER_ARROW_ANIM_FRAMES)
        return;

    sprite->sTimer = 0;
    if (++sprite->sStep >= (s16)ARRAY_COUNT(sSpinnerArrowOffsets))
    {
        sprite->sStep = 0;
        sprite->sMode = SPINNER_ARROW_STATIC;
    }
    sprite->y2 = sprite->sDir * sSpinnerArrowOffsets[sprite->sStep];
}

static void CreateSpinnerArrowSprites(s16 x, s16 y)
{
    u8 i;

    for (i = 0; i < SPINNER_ARROW_SPRITES_COUNT; i++)
    {
        s8 dir = (i == SPINNER_ARROW_UP) ? -1 : 1;
        u8 spriteId = CreateSprite(&sSpriteTemplate_SpinnerArrow, x, y + dir * SPINNER_ARROW_Y_OFFSET,
                                   SUBPRIORITY_SPINNER_ARROW);

        StartSpriteAnim(&gSprites[spriteId], i);
        gSprites[spriteId].sDir = dir;
        gSprites[spriteId].sMode = SPINNER_ARROW_STATIC;
        sShopData->spinnerArrowSpriteIds[i] = spriteId;
    }
}

static void DestroySpinnerArrowSprites(void)
{
    u8 i;

    for (i = 0; i < SPINNER_ARROW_SPRITES_COUNT; i++)
    {
        if (sShopData->spinnerArrowSpriteIds[i] != SPRITE_NONE)
        {
            DestroySprite(&gSprites[sShopData->spinnerArrowSpriteIds[i]]);
            sShopData->spinnerArrowSpriteIds[i] = SPRITE_NONE;
        }
    }
}

static void AnimateQuantitySpinner(void)
{
    u16 dpad = JOY_REPEAT(DPAD_ANY);
    u8 arrowIdx, spriteId;

    if (dpad == DPAD_UP || dpad == DPAD_RIGHT)
        arrowIdx = SPINNER_ARROW_UP;
    else if (dpad == DPAD_DOWN || dpad == DPAD_LEFT)
        arrowIdx = SPINNER_ARROW_DOWN;
    else
        return;

    spriteId = sShopData->spinnerArrowSpriteIds[arrowIdx];
    if (spriteId == SPRITE_NONE || gSprites[spriteId].sMode != SPINNER_ARROW_STATIC)
        return;

    gSprites[spriteId].sMode = SPINNER_ARROW_ANIM;
    gSprites[spriteId].sTimer = 0;
    gSprites[spriteId].sStep = 0;
}

#undef sDir
#undef sMode
#undef sTimer
#undef sStep

static void CreateQuantityFrameSprites(u8 y)
{
    u8 i;

    for (i = 0; i < QUANTITY_FRAME_SPRITES_COUNT; i++)
    {
        u8 spriteId = CreateSprite(&sSpriteTemplate_QuantityFrame, QUANTITY_FRAME_X + i * QUANTITY_FRAME_SPACING,
                                   y, SUBPRIORITY_QUANTITY_FRAME);

        sShopData->quantityFrameSpriteIds[i] = spriteId;
        StartSpriteAnim(&gSprites[spriteId], i);
        SetSpriteSheetFrameTileNum(&gSprites[spriteId]);
    }
    CreateSpinnerArrowSprites(QUANTITY_SPINNER_X, y);
}

static void DestroyQuantityFrameSprites(void)
{
    u8 i;

    for (i = 0; i < QUANTITY_FRAME_SPRITES_COUNT; i++)
    {
        if (sShopData->quantityFrameSpriteIds[i] != SPRITE_NONE)
        {
            DestroySprite(&gSprites[sShopData->quantityFrameSpriteIds[i]]);
            sShopData->quantityFrameSpriteIds[i] = SPRITE_NONE;
        }
    }
    DestroySpinnerArrowSprites();
}

static const union TextColor sQuantityTextColor =
{
    .background = 0,
    .foreground = 12,
    .shadow = 14,
};

static void BuyMenuPrintQuantity(s16 quantity)
{
    u8 spriteId = sShopData->quantityFrameSpriteIds[0];

    if (spriteId == SPRITE_NONE)
        return;

    ConvertIntToDecimalStringN(gStringVar1, quantity, STR_CONV_MODE_LEADING_ZEROS, MAX_ITEM_DIGITS);
    StringExpandPlaceholders(gStringVar4, gText_xVar1);
    FillSpriteRectColor(spriteId, QUANTITY_COUNT_LEFT, QUANTITY_COUNT_TOP,
                        QUANTITY_COUNT_RIGHT - QUANTITY_COUNT_LEFT,
                        GetFontAttribute(FONT_NARROW, FONTATTR_MAX_LETTER_HEIGHT), QUANTITY_FILL_INDEX);
    AddSpriteTextPrinterParameterized6(spriteId, FONT_NARROW,
                                       GetStringRightAlignXOffset(FONT_NARROW, gStringVar4, QUANTITY_COUNT_RIGHT),
                                       QUANTITY_COUNT_TOP, 0, 0, sQuantityTextColor, 0, gStringVar4);
}

static void BuyMenuPrintTotalCost(u32 total)
{
    u8 spriteId = sShopData->quantityFrameSpriteIds[1];

    if (spriteId == SPRITE_NONE)
        return;

    ConvertMoneyToCommaString(gStringVar1, total);
    StringExpandPlaceholders(gStringVar4, gText_PokedollarVar1);
    FillSpriteRectColor(spriteId, 0, QUANTITY_COUNT_TOP, QUANTITY_TOTAL_RIGHT,
                        GetFontAttribute(FONT_NARROW, FONTATTR_MAX_LETTER_HEIGHT), QUANTITY_FILL_INDEX);
    AddSpriteTextPrinterParameterized6(spriteId, FONT_NARROW,
                                       GetStringRightAlignXOffset(FONT_NARROW, gStringVar4, QUANTITY_TOTAL_RIGHT),
                                       QUANTITY_COUNT_TOP, 0, 0, sQuantityTextColor, 0, gStringVar4);
}

static void Task_BuyHowManyDialogueInit(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u32 maxQuantity;

    tItemCount = 1;
    CreateQuantityFrameSprites(QUANTITY_FRAME_Y);
    BuyMenuPrintQuantity(tItemCount);
    BuyMenuPrintTotalCost(sShopData->totalCost);

    // Avoid division by zero in-case something costs 0 pokedollars.
    if (sShopData->totalCost == 0)
        maxQuantity = MAX_BAG_ITEM_CAPACITY;
    else
        maxQuantity = GetMoney(&gSaveBlock1Ptr->money) / sShopData->totalCost;

    if (maxQuantity > MAX_BAG_ITEM_CAPACITY)
        sShopData->maxQuantity = MAX_BAG_ITEM_CAPACITY;
    else
        sShopData->maxQuantity = maxQuantity;

    gTasks[taskId].func = Task_BuyHowManyDialogueHandleInput;
}

static void Task_BuyHowManyDialogueHandleInput(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (AdjustQuantityAccordingToDPadInput(&tItemCount, sShopData->maxQuantity) == TRUE)
    {
        sShopData->totalCost = (GetItemPrice(tItemId) >> IsPokeNewsActive(POKENEWS_SLATEPORT)) * tItemCount;
        BuyMenuPrintQuantity(tItemCount);
        BuyMenuPrintTotalCost(sShopData->totalCost);
        AnimateQuantitySpinner();
    }
    else
    {
        if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            DestroyQuantityFrameSprites();
            CopyItemName(tItemId, gStringVar1);
            ConvertIntToDecimalStringN(gStringVar2, tItemCount, STR_CONV_MODE_LEFT_ALIGN, MAX_ITEM_DIGITS);
            ConvertMoneyToCommaString(gStringVar3, sShopData->totalCost);
            BuyMenuDisplayMessage(taskId, gText_Var1AndYouWantedVar2, BuyMenuConfirmPurchase);
        }
        else if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            DestroyQuantityFrameSprites();
            BuyMenuReturnToItemList(taskId);
        }
    }
}

static void BuyMenuConfirmPurchase(u8 taskId)
{
    CreateYesNoMenuWithCallbacks(taskId, &sShopBuyMenuYesNoWindowTemplate, 1, 0, 0, SHOP_BASE_STD_BORDER, 13, &sShopPurchaseYesNoFuncs);
}

static void BuyMenuSubtractMoney(u8 taskId)
{
    IncrementGameStat(GAME_STAT_SHOPPED);
    RemoveMoney(&gSaveBlock1Ptr->money, sShopData->totalCost);
    PlaySE(SE_SHOP);
    BuyMenuPrintMoney();

    if (sMartInfo.martType == MART_TYPE_NORMAL)
        gTasks[taskId].func = Task_ReturnToItemListAfterItemPurchase;
    else
        gTasks[taskId].func = Task_ReturnToItemListAfterDecorationPurchase;
}

static void BuyMenuTryMakePurchase(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    PutWindowTilemap(WIN_ITEM_LIST);

    if (sMartInfo.martType == MART_TYPE_NORMAL)
    {
        if (AddBagItem(tItemId, tItemCount) == TRUE)
        {
            GetSetItemObtained(tItemId, FLAG_SET_ITEM_OBTAINED);
            RecordItemPurchase(taskId);
            BuyMenuDisplayMessage(taskId, gText_HereYouGoThankYou, BuyMenuSubtractMoney);
        }
        else
        {
            BuyMenuDisplayMessage(taskId, gText_NoMoreRoomForThis, BuyMenuReturnToItemList);
        }
    }
    else
    {
        if (DecorationAdd(tItemId))
        {
            if (sMartInfo.martType == MART_TYPE_DECOR)
                BuyMenuDisplayMessage(taskId, gText_ThankYouIllSendItHome, BuyMenuSubtractMoney);
            else // MART_TYPE_DECOR2
                BuyMenuDisplayMessage(taskId, gText_ThanksIllSendItHome, BuyMenuSubtractMoney);
        }
        else
        {
            BuyMenuDisplayMessage(taskId, gText_SpaceForVar1Full, BuyMenuReturnToItemList);
        }
    }
}

static void Task_ReturnToItemListAfterItemPurchase(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        u16 premierBallsToAdd = tItemCount / 10;

        if (premierBallsToAdd >= 1
         && ((I_PREMIER_BALL_BONUS <= GEN_7 && tItemId == ITEM_POKE_BALL)
          || (I_PREMIER_BALL_BONUS >= GEN_8 && (GetItemPocket(tItemId) == POCKET_POKE_BALLS))))
        {
            u32 spaceAvailable = GetFreeSpaceForItemInBag(ITEM_PREMIER_BALL);

            if (spaceAvailable < premierBallsToAdd)
                premierBallsToAdd = spaceAvailable;
        }
        else
        {
            premierBallsToAdd = 0;
        }

        PlaySE(SE_SELECT);
        AddBagItem(ITEM_PREMIER_BALL, premierBallsToAdd);
        if (premierBallsToAdd > 0)
        {
            ConvertIntToDecimalStringN(gStringVar1, premierBallsToAdd, STR_CONV_MODE_LEFT_ALIGN, MAX_ITEM_DIGITS);
            BuyMenuDisplayMessage(taskId, (premierBallsToAdd >= 2 ? gText_ThrowInPremierBalls : gText_ThrowInPremierBall), BuyMenuReturnToItemList);
        }
        else
        {
            BuyMenuReturnToItemList(taskId);
        }
    }
}

static void Task_ReturnToItemListAfterDecorationPurchase(u8 taskId)
{
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        PlaySE(SE_SELECT);
        BuyMenuReturnToItemList(taskId);
    }
}

static void BuyMenuFreeSprites(void)
{
    u8 i;

    for (i = 0; i < ITEM_ICON_SLOT_COUNT; i++)
        BuyMenuRemoveItemIcon(i);

    FreeSpriteTilesByTag(TAG_CURSOR);
    FreeSpriteTilesByTag(TAG_HOVER_SLOT);
    FreeSpriteTilesByTag(TAG_SCROLL_THUMB);
    FreeSpriteTilesByTag(TAG_QUANTITY_FRAME);
    FreeSpriteTilesByTag(TAG_SPINNER_ARROW);
    FreeSpritePaletteByTag(TAG_SHOP_UI_PAL);
    FreeSpritePaletteByTag(TAG_SHOP_SHARED_PAL);
    ReleaseComfyAnims();
}

static void BuyMenuFreeMemory(void)
{
    if (sMartInfo.martType == MART_TYPE_NORMAL)
        TryFreeDynamicShopItemList(&sMartInfo.itemList);

    BuyMenuFreeSprites();
    Free(sShopData->itemNames);
    Free(sShopData);
    sShopData = NULL;
    FreeAllWindowBuffers();
}

static void Task_BuyMenu(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (!gPaletteFade.active)
    {
        s32 itemId = ShopList_ProcessInput();

        switch (itemId)
        {
        case LIST_NOTHING_CHOSEN:
            break;
        case LIST_CANCEL:
            PlaySE(SE_SELECT);
            ExitBuyMenu(taskId);
            break;
        default:
            PlaySE(SE_SELECT);
            tItemId = itemId;
            sShopData->totalCost = BuyMenuGetItemPrice(itemId);

            if (BuyMenuIsSoldOut(itemId))
                BuyMenuDisplayMessage(taskId, gText_ThatItemIsSoldOut, BuyMenuReturnToItemList);
            else if (!IsEnoughMoney(&gSaveBlock1Ptr->money, sShopData->totalCost))
            {
                BuyMenuDisplayMessage(taskId, gText_YouDontHaveMoney, BuyMenuReturnToItemList);
            }
            else
            {
                if (sMartInfo.martType == MART_TYPE_NORMAL)
                {
                    CopyItemName(tItemId, gStringVar1);
                    if (GetItemImportance(tItemId))
                    {
                        ConvertMoneyToCommaString(gStringVar2, sShopData->totalCost);
                        StringExpandPlaceholders(gStringVar4, gText_YouWantedVar1ThatllBeVar2);
                        tItemCount = 1;
                        sShopData->totalCost = (GetItemPrice(tItemId) >> IsPokeNewsActive(POKENEWS_SLATEPORT)) * tItemCount;
                        BuyMenuDisplayMessage(taskId, gStringVar4, BuyMenuConfirmPurchase);
                    }
                    else if (GetItemPocket(tItemId) == POCKET_TM_HM)
                    {
                        StringCopy(gStringVar2, GetMoveName(ItemIdToBattleMoveId(tItemId)));
                        BuyMenuDisplayMessage(taskId, gText_Var1CertainlyHowMany2, Task_BuyHowManyDialogueInit);
                    }
                    else
                    {
                        BuyMenuDisplayMessage(taskId, gText_Var1CertainlyHowMany, Task_BuyHowManyDialogueInit);
                    }
                }
                else
                {
                    StringCopy(gStringVar1, gDecorations[tItemId].name);
                    ConvertMoneyToCommaString(gStringVar2, sShopData->totalCost);

                    if (sMartInfo.martType == MART_TYPE_DECOR)
                        StringExpandPlaceholders(gStringVar4, gText_Var1IsItThatllBeVar2);
                    else // MART_TYPE_DECOR2
                        StringExpandPlaceholders(gStringVar4, gText_YouWantedVar1ThatllBeVar2);

                    BuyMenuDisplayMessage(taskId, gStringVar4, BuyMenuConfirmPurchase);
                }
            }
            break;
        }
    }
}
#undef tItemCount
#undef tItemId

static void ExitBuyMenu(u8 taskId)
{
    gFieldCallback = MapPostLoadHook_ReturnToShopMenu;
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_ExitBuyMenu;
}

static void Task_ExitBuyMenu(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        BuyMenuFreeMemory();
        SetMainCallback2(CB2_ReturnToField);
        DestroyTask(taskId);
    }
}
