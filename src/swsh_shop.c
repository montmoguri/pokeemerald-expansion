#include "global.h"
#include "bg.h"
#include "decompress.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "field_weather.h"
#include "fieldmap.h"
#include "gpu_regs.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "overworld.h"
#include "palette.h"
#include "scanline_effect.h"
#include "shop.h"
#include "sound.h"
#include "sprite.h"
#include "swsh_shop.h"
#include "task.h"
#include "window.h"
#include "constants/event_object_movement.h"
#include "constants/event_objects.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define SHOP_MENU_PALETTE_ID (gMapHeader.mapLayout->isFrlg ? 11 : 12)

// Mont note: shop menu shares char base 0 with the field tileset, current design only use 9 tiles
#define SHOP_MENU_BASE_TILE 1014

// in metatiles unit
#define VIEW_WIDTH                 15
#define VIEW_HEIGHT                10
#define VIEW_X_OFFSET              1
#define VIEW_Y_OFFSET              3
#define VIEW_OBJECT_SCAN_WIDTH     5
#define VIEW_OBJECT_SCAN_HEIGHT    10

enum {
    OBJ_EVENT_ID,
    X_COORD,
    Y_COORD,
    ANIM_NUM,
    VIEWPORT_OBJECT_FIELD_COUNT,
};

struct ShopData
{
    u16 menuTilemap[0x400];         // shop menu
    u16 mapTopTilemap[0x400];       // BG1, shared with shop menu
    u16 mapMidTilemap[0x400];       // BG2
    u16 mapBottomTilemap[0x400];    // BG3
    s16 viewportObjects[OBJECT_EVENTS_COUNT][VIEWPORT_OBJECT_FIELD_COUNT];
};

static EWRAM_DATA struct ShopData *sShopData = NULL;

static void CB2_BuyMenu(void);
static void VBlankCB_BuyMenu(void);
static void BuyMenuInitBgs(void);
static void BuyMenuInitWindows(void);
static void BuyMenuDecompressBgGraphics(void);
static void BuyMenuDrawGraphics(void);
static void BuyMenuDrawMapGraphics(void);
static void BuyMenuDrawMapBg(void);
static void BuyMenuDrawMapMetatile(s16 x, s16 y, const u16 *src, u8 metatileLayerType);
static void BuyMenuDrawMapMetatileLayer(u16 *dest, s16 offset1, s16 offset2, const u16 *src);
static void BuyMenuCollectObjectEventData(void);
static void BuyMenuDrawObjectEvents(void);
static void BuyMenuCopyMenuBgToBg1TilemapBuffer(void);
static bool8 BuyMenuCheckForOverlapWithMenuBg(int x, int y);
static void BuyMenuFreeMemory(void);
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

static const struct WindowTemplate sShopBuyMenuWindowTemplates[] =
{
    DUMMY_WIN_TEMPLATE
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
    ShowBg(1);
    ShowBg(2);
    ShowBg(3);
}

static void BuyMenuInitWindows(void)
{
    InitWindows(sShopBuyMenuWindowTemplates);
    DeactivateAllTextPrinters();
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

static void BuyMenuFreeMemory(void)
{
    Free(sShopData);
    sShopData = NULL;
    FreeAllWindowBuffers();
}

static void Task_BuyMenu(u8 taskId)
{
    if (gPaletteFade.active)
        return;

    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        PlaySE(SE_SELECT);
        ExitBuyMenu(taskId);
    }
}

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
