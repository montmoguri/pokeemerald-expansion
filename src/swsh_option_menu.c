#include "global.h"
#include "bg.h"
#include "comfy_anim.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "palette.h"
#include "scanline_effect.h"
#include "sprite.h"
#include "swsh_option_menu.h"
#include "task.h"
#include "constants/rgb.h"

enum SwShOptionId
{
    OPTION_TEXT_SPEED,
    OPTION_BATTLE_SCENE,
    OPTION_BATTLE_STYLE,
    OPTION_SOUND,
    OPTION_BUTTON_MODE,
    OPTION_FRAME,
    OPTION_COUNT,
};

#define OPTIONS_SHOWN               6
#define OPTION_ROW_HEIGHT           16

#define LIST_TILEMAP_TOP            3

#define SCROLL_TRACK_TILEMAP_LEFT   28
#define SCROLL_TRACK_ROWS           (OPTIONS_SHOWN * OPTION_ROW_HEIGHT / 8)
#define SCROLL_TRACK_LEN            (OPTIONS_SHOWN * OPTION_ROW_HEIGHT)
#define SCROLL_THUMB_X              (SCROLL_TRACK_TILEMAP_LEFT * 8 + 4)
#define SCROLL_THUMB_MIN_LEN        8
#define SCROLL_THUMB_SPRITES_COUNT  3

#define TAG_SWSH_OPTION_UI_PAL      55400
#define TAG_OPTION_SCROLL_THUMB     55403

// priority 2
#define SUBPRIORITY_SCROLL_THUMB    2

struct OptionMenu
{
    u8 bg2TilemapBuffer[BG_SCREEN_SIZE];
    u8 bg3TilemapBuffer[BG_SCREEN_SIZE];
    u16 scrollOffset;
    u16 selectedRow;
    u8 scrollThumbSpriteIds[SCROLL_THUMB_SPRITES_COUNT];
    u32 scrollThumbAnimId;
};

static EWRAM_DATA struct OptionMenu *sOptionMenu = NULL;

static void CB2_OptionMenu(void);
static void VBlankCB_OptionMenu(void);
static void OptionMenu_InitBgs(void);
static void OptionMenu_LoadGraphics(void);
static void OptionMenu_DrawScrollTrack(void);
static void OptionMenu_CreateSprites(void);
static void OptionMenu_FreeResources(void);
static void OptionList_Move(bool32 movingDown, bool32 allowWrap);
static void SpriteCB_ScrollThumb(struct Sprite *sprite);
static void Task_OptionMenu(u8 taskId);
static void Task_OptionMenuFadeOut(u8 taskId);

static const u32 sOptionMenu_Gfx[]      = INCGFX_U32("graphics/option/swsh/tiles.png", ".4bpp.smol");
static const u16 sOptionMenu_Pal[]      = INCGFX_U16("graphics/option/swsh/tiles.png", ".gbapal");
static const u32 sOptionMenu_BG2Map[]   = INCGFX_U32("graphics/option/swsh/bg2.bin", ".smolTM");
static const u32 sOptionMenu_BG3Map[]   = INCGFX_U32("graphics/option/swsh/bg3.bin", ".smolTM");
static const u8 sScrollTrack_Tiles[]    = INCBIN_U8("graphics/option/swsh/scroll_track.bin");
static const u32 sScrollThumb_Gfx[]     = INCGFX_U32("graphics/option/swsh/scroll_thumb.png", ".4bpp.smol");
static const u16 sOptionMenuUI_Pal[]    = INCGFX_U16("graphics/option/swsh/scroll_thumb.png", ".gbapal");

STATIC_ASSERT(sizeof(sScrollTrack_Tiles) == SCROLL_TRACK_ROWS, ScrollTrackRows);

static const struct BgTemplate sOptionMenuBgTemplates[] =
{
    {
        .bg = 2,
        .charBaseIndex = 2,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
    {
        .bg = 3,
        .charBaseIndex = 2,
        .mapBaseIndex = 29,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    },
};

static const struct OamData sOamData_ScrollThumb =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(8x32),
    .size = SPRITE_SIZE(8x32),
    .priority = 2,
};

static const struct CompressedSpriteSheet sSpriteSheet_ScrollThumb =
{
    .data = sScrollThumb_Gfx,
    .size = (8 * 32) / 2,
    .tag = TAG_OPTION_SCROLL_THUMB,
};

static const struct SpritePalette sSpritePalette_OptionMenuUI =
{
    .data = sOptionMenuUI_Pal,
    .tag = TAG_SWSH_OPTION_UI_PAL,
};

static const struct SpriteTemplate sSpriteTemplate_ScrollThumb =
{
    .tileTag = TAG_OPTION_SCROLL_THUMB,
    .paletteTag = TAG_SWSH_OPTION_UI_PAL,
    .oam = &sOamData_ScrollThumb,
    .callback = SpriteCB_ScrollThumb,
};

void CB2_InitOptionMenu_SwSh(void)
{
    switch (gMain.state)
    {
    case 0:
        SetVBlankHBlankCallbacksToNull();
        DmaClearLarge16(3, (void *)VRAM, VRAM_SIZE, 0x1000);
        CpuFastFill(0, (void *)OAM, OAM_SIZE);
        ScanlineEffect_Stop();
        ResetTempTileDataBuffers();
        FreeAllSpritePalettes();
        ResetPaletteFade();
        ResetSpriteData();
        ResetTasks();
        ClearScheduledBgCopiesToVram();
        sOptionMenu = AllocZeroed(sizeof(*sOptionMenu));
        memset(sOptionMenu->scrollThumbSpriteIds, SPRITE_NONE, sizeof(sOptionMenu->scrollThumbSpriteIds));
        sOptionMenu->scrollThumbAnimId = INVALID_COMFY_ANIM;
        OptionMenu_InitBgs();
        OptionMenu_LoadGraphics();
        gMain.state++;
        break;
    case 1:
        if (!FreeTempTileDataBuffersIfPossible())
            gMain.state++;
        break;
    default:
        OptionMenu_DrawScrollTrack();
        OptionMenu_CreateSprites();
        ScheduleBgCopyTilemapToVram(2);
        ScheduleBgCopyTilemapToVram(3);
        CreateTask(Task_OptionMenu, 0);
        BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB_OptionMenu);
        SetMainCallback2(CB2_OptionMenu);
        break;
    }
}

static void CB2_OptionMenu(void)
{
    RunTasks();
    AdvanceComfyAnimations();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void VBlankCB_OptionMenu(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void OptionMenu_InitBgs(void)
{
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sOptionMenuBgTemplates, ARRAY_COUNT(sOptionMenuBgTemplates));
    SetBgTilemapBuffer(2, sOptionMenu->bg2TilemapBuffer);
    SetBgTilemapBuffer(3, sOptionMenu->bg3TilemapBuffer);
    ResetAllBgsCoordinates();
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    ShowBg(2);
    ShowBg(3);
}

static void OptionMenu_LoadGraphics(void)
{
    DecompressAndCopyTileDataToVram(2, sOptionMenu_Gfx, 0, 0, 0);
    DecompressDataWithHeaderWram(sOptionMenu_BG2Map, sOptionMenu->bg2TilemapBuffer);
    DecompressDataWithHeaderWram(sOptionMenu_BG3Map, sOptionMenu->bg3TilemapBuffer);
    LoadPalette(sOptionMenu_Pal, BG_PLTT_ID(0), sizeof(sOptionMenu_Pal));
    LoadCompressedSpriteSheet(&sSpriteSheet_ScrollThumb);
    LoadSpritePalette(&sSpritePalette_OptionMenuUI);
}

static void OptionMenu_DrawScrollTrack(void)
{
    u32 i;

    if (OPTION_COUNT <= OPTIONS_SHOWN)
        return;

    for (i = 0; i < SCROLL_TRACK_ROWS; i++)
        FillBgTilemapBufferRect(3, sScrollTrack_Tiles[i], SCROLL_TRACK_TILEMAP_LEFT, LIST_TILEMAP_TOP + i, 1, 1, 0);
}

static u8 ScrollThumb_Length(void)
{
    if (OPTION_COUNT <= OPTIONS_SHOWN)
        return 0;
    return max(SCROLL_TRACK_LEN * OPTIONS_SHOWN / OPTION_COUNT, SCROLL_THUMB_MIN_LEN);
}

static s16 ScrollThumb_Offset(u16 absIdx)
{
    u8 len = ScrollThumb_Length();

    if (len == 0)
        return 0;
    return absIdx * (SCROLL_TRACK_LEN - len) / (OPTION_COUNT - 1);
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
    u8 len = ScrollThumb_Length();
    u8 index = sprite->data[0];
    u8 size;

    if (len == 0 || index >= ScrollThumb_SegmentCount(len))
    {
        sprite->invisible = TRUE;
        return;
    }

    size = ScrollThumb_SegmentSize(len);
    ScrollThumb_SetSegmentOam(sprite, size);
    sprite->y = LIST_TILEMAP_TOP * 8 + size / 2 + ScrollThumb_SegmentOffset(len, index);
    sprite->invisible = FALSE;
    if (sOptionMenu->scrollThumbAnimId != INVALID_COMFY_ANIM)
        sprite->y2 = ReadComfyAnimValueSmooth(&gComfyAnims[sOptionMenu->scrollThumbAnimId]);
}

static void OptionMenu_CreateSprites(void)
{
    s16 initialThumbY2 = ScrollThumb_Offset(sOptionMenu->scrollOffset + sOptionMenu->selectedRow);
    u32 i;

    sOptionMenu->scrollThumbAnimId = CreateComfyAnim_Easing(&(struct ComfyAnimEasingConfig){
        .from = Q_24_8(initialThumbY2),
        .to = Q_24_8(initialThumbY2),
        .durationFrames = 1,
        .easingFunc = ComfyAnimEasing_EaseOutCubic,
    });

    for (i = 0; i < SCROLL_THUMB_SPRITES_COUNT; i++)
    {
        u8 spriteId = CreateSprite(&sSpriteTemplate_ScrollThumb, SCROLL_THUMB_X, 0, SUBPRIORITY_SCROLL_THUMB);

        sOptionMenu->scrollThumbSpriteIds[i] = spriteId;
        gSprites[spriteId].data[0] = i;
        gSprites[spriteId].invisible = TRUE;
    }
}

static void OptionList_SeekTo(u16 absPos)
{
    u16 top = 0;

    if (OPTION_COUNT > OPTIONS_SHOWN)
    {
        u16 halfScreen = OPTIONS_SHOWN / 2;
        u16 maxTop = OPTION_COUNT - OPTIONS_SHOWN;

        if (absPos > halfScreen)
            top = min(absPos - halfScreen, maxTop);
    }

    sOptionMenu->scrollOffset = top;
    sOptionMenu->selectedRow = absPos - top;
}

static void OptionList_Move(bool32 movingDown, bool32 allowWrap)
{
    u16 oldAbs = sOptionMenu->scrollOffset + sOptionMenu->selectedRow;
    u16 abs = oldAbs;
    u32 durationFrames = 8;

    if (movingDown)
    {
        if (abs < OPTION_COUNT - 1)
            abs++;
        else if (allowWrap)
            abs = 0;
    }
    else
    {
        if (abs != 0)
            abs--;
        else if (allowWrap)
            abs = OPTION_COUNT - 1;
    }

    if (abs == oldAbs)
        return;

    OptionList_SeekTo(abs);

    if (OPTION_COUNT > OPTIONS_SHOWN && sOptionMenu->scrollThumbAnimId != INVALID_COMFY_ANIM)
    {
        struct ComfyAnim *thumbAnim = &gComfyAnims[sOptionMenu->scrollThumbAnimId];
        s32 maxOffset = Q_24_8(SCROLL_TRACK_LEN - ScrollThumb_Length());

        if (!thumbAnim->completed)
            durationFrames = (gMain.heldKeys & (DPAD_UP | DPAD_DOWN)) ? 1 : 2;

        InitComfyAnim_Easing(&(struct ComfyAnimEasingConfig){
            .from = min(thumbAnim->position, maxOffset),
            .to = Q_24_8(ScrollThumb_Offset(abs)),
            .durationFrames = durationFrames,
            .easingFunc = ComfyAnimEasing_EaseOutCubic,
        }, thumbAnim);
    }
}

static void Task_OptionMenu(u8 taskId)
{
    if (gPaletteFade.active)
        return;

    if (JOY_NEW(B_BUTTON))
    {
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_OptionMenuFadeOut;
    }
    else if (JOY_REPEAT(DPAD_UP))
    {
        OptionList_Move(FALSE, JOY_NEW(DPAD_UP));
    }
    else if (JOY_REPEAT(DPAD_DOWN))
    {
        OptionList_Move(TRUE, JOY_NEW(DPAD_DOWN));
    }
}

static void Task_OptionMenuFadeOut(u8 taskId)
{
    if (gPaletteFade.active)
        return;

    OptionMenu_FreeResources();
    DestroyTask(taskId);
    SetMainCallback2(gMain.savedCallback);
}

static void OptionMenu_FreeResources(void)
{
    FreeSpriteTilesByTag(TAG_OPTION_SCROLL_THUMB);
    FreeSpritePaletteByTag(TAG_SWSH_OPTION_UI_PAL);
    ReleaseComfyAnims();
    Free(sOptionMenu);
    sOptionMenu = NULL;
}
