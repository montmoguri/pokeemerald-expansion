#include "global.h"
#include "bg.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "overworld.h"
#include "palette.h"
#include "scanline_effect.h"
#include "sprite.h"
#include "task.h"
#include "constants/rgb.h"
#include "swsh_pokedex.h"

#define BG3_SLIDE_DISTANCE  100
#define BG3_SLIDE_EASE_DIV  4

struct SwShPokedex
{
    u16 bg3TilemapBuffer[2][BG_SCREEN_SIZE / 2];
    s16 bg3X;
    s16 bg3TargetX;
};

static EWRAM_DATA struct SwShPokedex *sPokedex = NULL;

static const u32 sSwShPokedex_Gfx[]          = INCGFX_U32("graphics/pokedex/swsh/tiles.png", ".4bpp.smol");
static const u16 sSwShPokedex_Pal[]          = INCGFX_U16("graphics/pokedex/swsh/tiles.png", ".gbapal");
static const u32 sSwShPokedex_BG3a_Tilemap[] = INCGFX_U32("graphics/pokedex/swsh/bg3a.bin", ".smolTM");
static const u32 sSwShPokedex_BG3b_Tilemap[] = INCGFX_U32("graphics/pokedex/swsh/bg3b.bin", ".smolTM");

static const struct BgTemplate sBgTemplates[] =
{
    {
        .bg = 3,
        .charBaseIndex = 2,
        .mapBaseIndex = 28,
        .screenSize = 1,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0,
    },
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

static void UpdateBg3Slide(void)
{
    s32 delta = sPokedex->bg3TargetX - sPokedex->bg3X;

    if (delta == 0)
        return;

    s32 step = delta / BG3_SLIDE_EASE_DIV;
    if (step == 0)
        step = (delta > 0) ? 1 : -1;

    sPokedex->bg3X += step;
    ChangeBgX(3, sPokedex->bg3X << 8, BG_COORD_SET);
}

static void Task_ExitSwShPokedex(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        UnsetBgTilemapBuffer(3);
        TRY_FREE_AND_SET_NULL(sPokedex);
        DestroyTask(taskId);
        SetMainCallback2(CB2_ReturnToFieldWithOpenMenu);
    }
}

static void Task_SwShPokedexInput(u8 taskId)
{
    UpdateBg3Slide();

    if (gPaletteFade.active)
        return;

    if (JOY_NEW(A_BUTTON))
    {
        sPokedex->bg3TargetX = BG3_SLIDE_DISTANCE;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        if (sPokedex->bg3TargetX != 0)
        {
            sPokedex->bg3TargetX = 0;
        }
        else
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            gTasks[taskId].func = Task_ExitSwShPokedex;
        }
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
        SetBgTilemapBuffer(3, sPokedex->bg3TilemapBuffer);
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
            gMain.state++;
        }
        break;
    case 4:
        LoadPalette(sSwShPokedex_Pal, BG_PLTT_ID(0), PLTT_SIZE_4BPP);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(3);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB_SwShPokedex);
        CreateTask(Task_SwShPokedexInput, 0);
        SetMainCallback2(CB2_SwShPokedex);
        break;
    }
}
