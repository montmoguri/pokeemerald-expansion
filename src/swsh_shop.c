#include "global.h"
#include "bg.h"
#include "decompress.h"
#include "decoration.h"
#include "decoration_inventory.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "field_weather.h"
#include "fieldmap.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "item.h"
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
    WIN_QUANTITY_PRICE,
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

#define LIST_NAME_BUFFER_SIZE      (ITEM_NAME_LENGTH + 15)
#define DESCRIPTION_BUFFER_SIZE    200

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
static void BuyMenuPrintItemQuantityAndPrice(u8 taskId);
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

#define WIN_QUANTITY_PRICE_W        10
#define WIN_QUANTITY_PRICE_H        2
#define WIN_QUANTITY_PRICE_TILES    (WIN_QUANTITY_PRICE_W * WIN_QUANTITY_PRICE_H)
#define WIN_QUANTITY_PRICE_BASE     (WIN_MESSAGE_BASE + WIN_MESSAGE_TILES)

#define WIN_YESNO_W                 5
#define WIN_YESNO_H                 4
#define WIN_YESNO_TILES             (WIN_YESNO_W * WIN_YESNO_H)
#define WIN_YESNO_BASE              (WIN_QUANTITY_PRICE_BASE + WIN_QUANTITY_PRICE_TILES)

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
    [WIN_QUANTITY_PRICE] = {
        .bg = 0,
        .tilemapLeft = 18,
        .tilemapTop = 11,
        .width = WIN_QUANTITY_PRICE_W,
        .height = WIN_QUANTITY_PRICE_H,
        .paletteNum = 15,
        .baseBlock = WIN_QUANTITY_PRICE_BASE,
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

static const u8 sVanillaTextColors[3] = {1, 2, 3};

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
}

static void BuyMenuDrawGraphics(void)
{
    BuyMenuDrawMapGraphics();
    BuyMenuCopyMenuBgToBg1TilemapBuffer();
    BuyMenuTintMapView();
    BuyMenuPrintMoney();
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
#define LIST_ROW_NAME_X            4
#define LIST_ROW_NAME_MAX_WIDTH    88

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

static void BuyMenuMoveCursorCallback(u32 index, bool32 onInit)
{
    s32 oldHovered = sShopData->hoveredIndex;

    sShopData->hoveredIndex = index;

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

static void BuyMenuPrintItemQuantityAndPrice(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u8 windowWidth = sShopBuyMenuWindowTemplates[WIN_QUANTITY_PRICE].width * 8;

    FillWindowPixelBuffer(WIN_QUANTITY_PRICE, PIXEL_FILL(1));
    ConvertIntToDecimalStringN(gStringVar1, tItemCount, STR_CONV_MODE_LEADING_ZEROS, MAX_ITEM_DIGITS);
    StringExpandPlaceholders(gStringVar4, gText_xVar1);
    AddTextPrinterParameterized4(WIN_QUANTITY_PRICE, FONT_NORMAL, 0, 1, 0, 0, sVanillaTextColors, TEXT_SKIP_DRAW, gStringVar4);
    ConvertMoneyToCommaString(gStringVar1, sShopData->totalCost);
    StringExpandPlaceholders(gStringVar4, gText_PokedollarVar1);
    AddTextPrinterParameterized4(WIN_QUANTITY_PRICE, FONT_NORMAL,
                                 GetStringRightAlignXOffset(FONT_NORMAL, gStringVar4, windowWidth), 1, 0, 0,
                                 sVanillaTextColors, TEXT_SKIP_DRAW, gStringVar4);
    CopyWindowToVram(WIN_QUANTITY_PRICE, COPYWIN_GFX);
}

static void Task_BuyHowManyDialogueInit(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u32 maxQuantity;

    tItemCount = 1;
    DrawStdFrameWithCustomTileAndPalette(WIN_QUANTITY_PRICE, FALSE, SHOP_BASE_STD_BORDER, 13);
    BuyMenuPrintItemQuantityAndPrice(taskId);
    ScheduleBgCopyTilemapToVram(0);

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
        BuyMenuPrintItemQuantityAndPrice(taskId);
    }
    else
    {
        if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            ClearStdWindowAndFrameToTransparent(WIN_QUANTITY_PRICE, FALSE);
            ClearWindowTilemap(WIN_QUANTITY_PRICE);
            PutWindowTilemap(WIN_ITEM_LIST);
            CopyItemName(tItemId, gStringVar1);
            ConvertIntToDecimalStringN(gStringVar2, tItemCount, STR_CONV_MODE_LEFT_ALIGN, MAX_ITEM_DIGITS);
            ConvertMoneyToCommaString(gStringVar3, sShopData->totalCost);
            BuyMenuDisplayMessage(taskId, gText_Var1AndYouWantedVar2, BuyMenuConfirmPurchase);
        }
        else if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            ClearStdWindowAndFrameToTransparent(WIN_QUANTITY_PRICE, FALSE);
            ClearWindowTilemap(WIN_QUANTITY_PRICE);
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

static void BuyMenuFreeMemory(void)
{
    if (sMartInfo.martType == MART_TYPE_NORMAL)
        TryFreeDynamicShopItemList(&sMartInfo.itemList);

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
