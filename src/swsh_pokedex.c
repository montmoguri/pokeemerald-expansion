#include "global.h"
#include "bg.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "overworld.h"
#include "palette.h"
#include "pokedex.h"
#include "pokedex_area_region_map.h"
#include "scanline_effect.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "constants/rgb.h"
#include "swsh_pokedex.h"

#define BG3_SLIDE_DISTANCE      100
#define BG3_SLIDE_EASE_DIV      4

#define BLEND_MAX               16
#define BLEND_STEP              2

#define DEX_GFX_CHAR_BASE       2
#define HABITAT_MAP_CHAR_BASE   1
#define DEX_TEXT_CHAR_BASE      0

#define TILEMAP_TILE_WIDTH      32
#define PROMPT_BAR_TILE_HEIGHT  2
#define PROMPT_BAR_TILE_Y       (DISPLAY_TILE_HEIGHT - PROMPT_BAR_TILE_HEIGHT)

#define DEX_TEXT_PAL_NUM        1
#define DEX_PAL_BANK_COUNT      2
#define DEX_TEXT_Y              4
#define DEX_COUNT_DIGITS        4

#define WIN_LIST_WIDTH          16
#define WIN_LIST_HEIGHT         14
#define WIN_SORT_TYPE_WIDTH     12
#define WIN_SORT_TYPE_HEIGHT    3
#define WIN_CAUGHT_SEEN_WIDTH   9
#define WIN_CAUGHT_SEEN_HEIGHT  3
#define WIN_REGION_WIDTH        5
#define WIN_REGION_HEIGHT       3

#define WIN_BASE_BLOCK_AFTER(prev)  (WIN_##prev##_BASE_BLOCK + WIN_##prev##_WIDTH * WIN_##prev##_HEIGHT)

enum
{
    WIN_LIST_BASE_BLOCK         = 1,
    WIN_SORT_TYPE_BASE_BLOCK    = WIN_BASE_BLOCK_AFTER(LIST),
    WIN_CAUGHT_SEEN_BASE_BLOCK  = WIN_BASE_BLOCK_AFTER(SORT_TYPE),
    WIN_REGION_BASE_BLOCK       = WIN_BASE_BLOCK_AFTER(CAUGHT_SEEN),
};

#define BALL_WIDTH              8
#define BALL_HEIGHT             16
#define BALL_TILE_COUNT         2
#define CAUGHT_BALL_X           0
#define SEEN_BALL_X             40
#define CAUGHT_NUM_X            11
#define SEEN_NUM_X              51
#define COUNT_NUM_SPAN          20

enum DexPage
{
    DEX_PAGE_LIST,
    DEX_PAGE_ENTRY,
    DEX_PAGE_HABITAT,
};

enum DexWindow
{
    WIN_DEX_LIST,
    WIN_DEX_SORT_TYPE,
    WIN_DEX_CAUGHT_SEEN_NUM,
    WIN_DEX_REGION,
    WIN_DEX_COUNT,
};

enum DexBall
{
    BALL_CAUGHT_COLOR,
    BALL_CAUGHT_NORMAL,
    BALL_CAUGHT_HOVER,
    BALL_SEEN_NORMAL,
    BALL_SEEN_HOVER,
};

enum DexTextColor
{
    COLORID_SORT,
    COLORID_COUNT_NUM,
    COLORID_REGION,
};

struct SwShPokedex
{
    u16 bg1TilemapBuffer[BG_SCREEN_SIZE / 2];
    u16 bg2TilemapBuffer[BG_SCREEN_SIZE / 2];
    u16 bg3TilemapBuffer[2][BG_SCREEN_SIZE / 2];
    s16 bg3X;
    u8 page;
    u8 bg2Page;
    u8 bg2Alpha;
    u8 bg3White;
    bool8 bg3Habitat;
    bool8 bg3Loading;
};

static EWRAM_DATA struct SwShPokedex *sPokedex = NULL;

static const u32 sSwShPokedex_Gfx[]                 = INCGFX_U32("graphics/pokedex/swsh/tiles.png", ".4bpp.smol");
static const u16 sSwShPokedex_Pal[]                 = INCGFX_U16("graphics/pokedex/swsh/tiles.png", ".gbapal");
static const u32 sSwShPokedex_BG3a_Tilemap[]        = INCGFX_U32("graphics/pokedex/swsh/bg3a.bin", ".smolTM");
static const u32 sSwShPokedex_BG3b_Tilemap[]        = INCGFX_U32("graphics/pokedex/swsh/bg3b.bin", ".smolTM");
static const u32 sSwShPokedex_BG2List_Tilemap[]     = INCGFX_U32("graphics/pokedex/swsh/bg2_list.bin", ".smolTM");
static const u32 sSwShPokedex_BG2Entry_Tilemap[]    = INCGFX_U32("graphics/pokedex/swsh/bg2_entry.bin", ".smolTM");
static const u8 sCaughtBall_Gfx[]                   = INCGFX_U8("graphics/pokedex/swsh/caught_ball.png", ".4bpp");

static const u8 sTextColors[][3] =
{
    [COLORID_SORT]      = {0, 1, 2},
    [COLORID_COUNT_NUM] = {0, 1, 5},
    [COLORID_REGION]    = {0, 3, 6},
};

static const u32 *const sBg2Tilemaps[] =
{
    [DEX_PAGE_LIST]  = sSwShPokedex_BG2List_Tilemap,
    [DEX_PAGE_ENTRY] = sSwShPokedex_BG2Entry_Tilemap,
};

static const struct BgTemplate sBgTemplates[] =
{
    {
        .bg = 1,
        .charBaseIndex = DEX_TEXT_CHAR_BASE,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0,
    },
    {
        .bg = 2,
        .charBaseIndex = DEX_GFX_CHAR_BASE,
        .mapBaseIndex = 27,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0,
    },
    {
        .bg = 3,
        .charBaseIndex = DEX_GFX_CHAR_BASE,
        .mapBaseIndex = 28,
        .screenSize = 1,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0,
    },
};

static const struct WindowTemplate sWindowTemplates[] =
{
    [WIN_DEX_LIST] =
    {
        .bg = 1,
        .tilemapLeft = 13,
        .tilemapTop = 3,
        .width = WIN_LIST_WIDTH,
        .height = WIN_LIST_HEIGHT,
        .paletteNum = DEX_TEXT_PAL_NUM,
        .baseBlock = WIN_LIST_BASE_BLOCK,
    },
    [WIN_DEX_SORT_TYPE] =
    {
        .bg = 1,
        .tilemapLeft = 17,
        .tilemapTop = 0,
        .width = WIN_SORT_TYPE_WIDTH,
        .height = WIN_SORT_TYPE_HEIGHT,
        .paletteNum = DEX_TEXT_PAL_NUM,
        .baseBlock = WIN_SORT_TYPE_BASE_BLOCK,
    },
    [WIN_DEX_CAUGHT_SEEN_NUM] =
    {
        .bg = 1,
        .tilemapLeft = 7,
        .tilemapTop = 0,
        .width = WIN_CAUGHT_SEEN_WIDTH,
        .height = WIN_CAUGHT_SEEN_HEIGHT,
        .paletteNum = DEX_TEXT_PAL_NUM,
        .baseBlock = WIN_CAUGHT_SEEN_BASE_BLOCK,
    },
    [WIN_DEX_REGION] =
    {
        .bg = 1,
        .tilemapLeft = 1,
        .tilemapTop = 0,
        .width = WIN_REGION_WIDTH,
        .height = WIN_REGION_HEIGHT,
        .paletteNum = DEX_TEXT_PAL_NUM,
        .baseBlock = WIN_REGION_BASE_BLOCK,
    },
    DUMMY_WIN_TEMPLATE,
};

static void VBlankCB_SwShPokedex(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB2_SwShPokedex(void)
{
    RunTasks();
    DoScheduledBgTilemapCopiesToVram();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void BlitCaughtBall(u32 windowId, enum DexBall ball, u32 x, u32 y)
{
    BlitBitmapToWindow(windowId, sCaughtBall_Gfx + ball * BALL_TILE_COUNT * TILE_SIZE_4BPP, x, y, BALL_WIDTH, BALL_HEIGHT);
}

static void PrintDexText(u32 windowId, u32 x, enum DexTextColor color, const u8 *str)
{
    AddTextPrinterParameterized3(windowId, FONT_NARROW, x, DEX_TEXT_Y, sTextColors[color], TEXT_SKIP_DRAW, str);
}

static void PrintCount(u32 windowId, u32 x, u32 count)
{
    ConvertIntToDecimalStringN(gStringVar1, count, STR_CONV_MODE_LEFT_ALIGN, DEX_COUNT_DIGITS);
    PrintDexText(windowId, x + GetStringCenterAlignXOffset(FONT_NARROW, gStringVar1, COUNT_NUM_SPAN),
                 COLORID_COUNT_NUM, gStringVar1);
}

static void PrintTopBar(void)
{
    const u8 *sortStr = COMPOUND_STRING("By Number");
    bool32 national = (gSaveBlock2Ptr->pokedex.mode == DEX_MODE_NATIONAL);
    u32 caught = national ? GetNationalPokedexCount(FLAG_GET_CAUGHT) : GetHoennPokedexCount(FLAG_GET_CAUGHT);
    u32 seen = national ? GetNationalPokedexCount(FLAG_GET_SEEN) : GetHoennPokedexCount(FLAG_GET_SEEN);

    for (u32 i = 0; i < WIN_DEX_COUNT; i++)
    {
        FillWindowPixelBuffer(i, PIXEL_FILL(0));
    }

    PrintDexText(WIN_DEX_SORT_TYPE, GetStringCenterAlignXOffset(FONT_NARROW, sortStr, WIN_SORT_TYPE_WIDTH * TILE_WIDTH),
                 COLORID_SORT, sortStr);

    BlitCaughtBall(WIN_DEX_CAUGHT_SEEN_NUM, BALL_CAUGHT_COLOR, CAUGHT_BALL_X, DEX_TEXT_Y);
    BlitCaughtBall(WIN_DEX_CAUGHT_SEEN_NUM, BALL_SEEN_HOVER, SEEN_BALL_X, DEX_TEXT_Y);
    PrintCount(WIN_DEX_CAUGHT_SEEN_NUM, CAUGHT_NUM_X, caught);
    PrintCount(WIN_DEX_CAUGHT_SEEN_NUM, SEEN_NUM_X, seen);

    PrintDexText(WIN_DEX_REGION, 0, COLORID_REGION,
                 national ? COMPOUND_STRING("NATIONAL") : GetExpandedPlaceholder(PLACEHOLDER_ID_REGION));

    for (u32 i = 0; i < WIN_DEX_COUNT; i++)
    {
        PutWindowTilemap(i);
        CopyWindowToVram(i, COPYWIN_FULL);
    }
}

static void LoadBg2Page(enum DexPage page)
{
    sPokedex->bg2Page = page;
    if (page == DEX_PAGE_HABITAT)
    {
        // region map has no prompt bar, copies it to BG2 during transition
        CpuFill16(0, sPokedex->bg2TilemapBuffer, sizeof(sPokedex->bg2TilemapBuffer));
        CopyToBgTilemapBufferRect(2, &sPokedex->bg3TilemapBuffer[0][PROMPT_BAR_TILE_Y * TILEMAP_TILE_WIDTH],
                                  0, PROMPT_BAR_TILE_Y, TILEMAP_TILE_WIDTH, PROMPT_BAR_TILE_HEIGHT);
    }
    else
    {
        DecompressDataWithHeaderWram(sBg2Tilemaps[page], sPokedex->bg2TilemapBuffer);
    }
    sPokedex->bg2Alpha = 0;
    ScheduleBgCopyTilemapToVram(2);
}

static void LoadBg3Gfx(bool32 habitat)
{
    sPokedex->bg3Habitat = habitat;
    sPokedex->bg3Loading = TRUE;
    if (habitat)
    {
        SetBgAttribute(3, BG_ATTR_CHARBASEINDEX, HABITAT_MAP_CHAR_BASE);
        LoadPokedexAreaMapGfx();
        CpuCopy32(gPlttBufferUnfaded, gPlttBufferFaded, BG_PLTT_SIZE);
    }
    else
    {
        SetBgAttribute(3, BG_ATTR_CHARBASEINDEX, DEX_GFX_CHAR_BASE);
        SetBgAttribute(3, BG_ATTR_PALETTEMODE, 0);
        CopyBgTilemapBufferToVram(3);
    }
    ShowBg(3);
}

static void UpdateBg2Fade(enum DexPage page)
{
    if (sPokedex->bg2Page != page)
    {
        if (sPokedex->bg2Page != DEX_PAGE_HABITAT && sPokedex->bg2Alpha > 0)
            sPokedex->bg2Alpha -= min(BLEND_STEP, sPokedex->bg2Alpha);
        else
            LoadBg2Page(page);
    }
    else if (page == DEX_PAGE_HABITAT)
    {
        sPokedex->bg2Alpha = BLEND_MAX;
    }
    else if (sPokedex->bg2Alpha < BLEND_MAX)
    {
        sPokedex->bg2Alpha += min(BLEND_STEP, BLEND_MAX - sPokedex->bg2Alpha);
    }

    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG3 | BLDCNT_TGT2_BD);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(sPokedex->bg2Alpha, BLEND_MAX - sPokedex->bg2Alpha));
}

static void UpdateBg3Whiteout(bool32 wantHabitat)
{
    if (sPokedex->bg3Habitat != wantHabitat)
    {
        if (sPokedex->bg3White < BLEND_MAX)
            sPokedex->bg3White += min(BLEND_STEP, BLEND_MAX - sPokedex->bg3White);
        else if (sPokedex->bg3X == 0)
            LoadBg3Gfx(wantHabitat);
    }
    else if (sPokedex->bg3Loading)
    {
        if (!FreeTempTileDataBuffersIfPossible())
            sPokedex->bg3Loading = FALSE;
    }
    else
    {
        sPokedex->bg3White -= min(BLEND_STEP, sPokedex->bg3White);
    }

    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG3 | BLDCNT_EFFECT_LIGHTEN);
    SetGpuReg(REG_OFFSET_BLDY, sPokedex->bg3White);
}

static void UpdatePageTransition(void)
{
    bool32 wantHabitat = (sPokedex->page == DEX_PAGE_HABITAT);

    if (sPokedex->bg3Habitat != wantHabitat || sPokedex->bg3White > 0)
    {
        if (sPokedex->bg2Page != DEX_PAGE_HABITAT || sPokedex->bg2Alpha < BLEND_MAX)
            UpdateBg2Fade(DEX_PAGE_HABITAT);
        else
            UpdateBg3Whiteout(wantHabitat);
    }
    else
    {
        UpdateBg2Fade(sPokedex->page);
    }
}

static void UpdateBg3Slide(void)
{
    s32 targetX = (sPokedex->page == DEX_PAGE_ENTRY && !sPokedex->bg3Habitat) ? BG3_SLIDE_DISTANCE : 0;
    s32 delta = targetX - sPokedex->bg3X;

    if (delta == 0)
        return;

    s32 step = delta / BG3_SLIDE_EASE_DIV;
    if (step == 0)
        step = (delta > 0) ? 1 : -1;

    sPokedex->bg3X += step;
    ChangeBgX(3, sPokedex->bg3X << 8, BG_COORD_SET);
}

static void UpdateBg1Visibility(void)
{
    if (sPokedex->page == DEX_PAGE_LIST && sPokedex->bg2Page == DEX_PAGE_LIST && sPokedex->bg2Alpha == BLEND_MAX)
    {
        ShowBg(1);
    }
    else
    {
        HideBg(1);
    }
}

static void Task_ExitSwShPokedex(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        FreeAllWindowBuffers();
        UnsetBgTilemapBuffer(1);
        UnsetBgTilemapBuffer(2);
        UnsetBgTilemapBuffer(3);
        TRY_FREE_AND_SET_NULL(sPokedex);
        DestroyTask(taskId);
        SetMainCallback2(CB2_ReturnToFieldWithOpenMenu);
    }
}

static void Task_SwShPokedexInput(u8 taskId)
{
    UpdateBg3Slide();
    UpdatePageTransition();
    UpdateBg1Visibility();

    if (gPaletteFade.active)
        return;

    switch (sPokedex->page)
    {
    case DEX_PAGE_LIST:
        if (JOY_NEW(A_BUTTON))
        {
            sPokedex->page = DEX_PAGE_ENTRY;
        }
        else if (JOY_NEW(B_BUTTON))
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            gTasks[taskId].func = Task_ExitSwShPokedex;
        }
        break;
    case DEX_PAGE_ENTRY:
        if (JOY_NEW(B_BUTTON))
            sPokedex->page = DEX_PAGE_LIST;
        else if (JOY_NEW(START_BUTTON))
            sPokedex->page = DEX_PAGE_HABITAT;
        break;
    case DEX_PAGE_HABITAT:
        if (JOY_NEW(B_BUTTON))
            sPokedex->page = DEX_PAGE_ENTRY;
        break;
    }
}

void CB2_OpenPokedex_SwSh(void)
{
    switch (gMain.state)
    {
    case 0:
    default:
        SetVBlankCallback(NULL);
        ResetVramOamAndBgCntRegs();
        gMain.state++;
        break;
    case 1:
        ScanlineEffect_Stop();
        ResetTasks();
        ResetSpriteData();
        ResetPaletteFade();
        FreeAllSpritePalettes();
        gMain.state++;
        break;
    case 2:
        sPokedex = AllocZeroed(sizeof(*sPokedex));
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
        SetBgTilemapBuffer(1, sPokedex->bg1TilemapBuffer);
        SetBgTilemapBuffer(2, sPokedex->bg2TilemapBuffer);
        SetBgTilemapBuffer(3, sPokedex->bg3TilemapBuffer);
        InitWindows(sWindowTemplates);
        ResetAllBgsCoordinates();
        ResetTempTileDataBuffers();
        DecompressAndCopyTileDataToVram(3, sSwShPokedex_Gfx, 0, 0, 0);
        gMain.state++;
        break;
    case 3:
        if (FreeTempTileDataBuffersIfPossible() != TRUE)
        {
            DecompressDataWithHeaderWram(sSwShPokedex_BG3a_Tilemap, sPokedex->bg3TilemapBuffer[0]);
            DecompressDataWithHeaderWram(sSwShPokedex_BG3b_Tilemap, sPokedex->bg3TilemapBuffer[1]);
            CopyBgTilemapBufferToVram(3);
            LoadBg2Page(DEX_PAGE_LIST);
            sPokedex->bg2Alpha = BLEND_MAX;
            PrintTopBar();
            gMain.state++;
        }
        break;
    case 4:
        LoadPalette(sSwShPokedex_Pal, BG_PLTT_ID(0), DEX_PAL_BANK_COUNT * PLTT_SIZE_4BPP);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG3 | BLDCNT_TGT2_BD);
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(BLEND_MAX, 0));
        ShowBg(1);
        ShowBg(2);
        ShowBg(3);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB_SwShPokedex);
        CreateTask(Task_SwShPokedexInput, 0);
        SetMainCallback2(CB2_SwShPokedex);
        break;
    }
}
