#include "constants/global.h"
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
#include "string_util.h"
#include "swsh_graphics.h"
#include "swsh_option_menu.h"
#include "swsh_utils.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "gba/m4a_internal.h"
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

struct SwShOption
{
    const u8 *name;
    const u8 *description;                  // optional, description of what the options do
    const u8 *const *choices;               // NULL when using getLabel
    u8 choiceCount;
    void (*getLabel)(u8 *dest, u32 value);  // optional, prints one label that updates instead choices (see Frame_GetLabel)
    void (*drawTilemap)(u32 tilemapTop);    // optional, draws BG1 tiles over the row's cells (see Frame_DrawPreview)
};

enum
{
    WIN_LIST,
    WIN_DESCRIPTION,
};

enum
{
    FRAME_TILE_TOP_LEFT,
    FRAME_TILE_TOP,
    FRAME_TILE_TOP_RIGHT,
    FRAME_TILE_LEFT,
    FRAME_TILE_CENTER,
    FRAME_TILE_RIGHT,
    FRAME_TILE_BOTTOM_LEFT,
    FRAME_TILE_BOTTOM,
    FRAME_TILE_BOTTOM_RIGHT,
};

enum
{
    COLORID_NORMAL,
    COLORID_FOCUSED,
    COLORID_NOT_CHOSEN,
    COLORID_CHOSEN_FOCUSED,
    COLORID_NOT_CHOSEN_FOCUSED,
    COLORID_DESCRIPTION,
};

#define OPTIONS_SHOWN               6
#define OPTION_ROW_HEIGHT           16
#define OPTION_LABEL_LENGTH         16

#define LIST_FONT                   FONT_NARROW
#define LIST_TILEMAP_LEFT           3
#define LIST_TILEMAP_TOP            3
#define LIST_WIDTH                  24
#define LIST_HEIGHT                 (OPTIONS_SHOWN * OPTION_ROW_HEIGHT / 8)
#define LIST_BASE_BLOCK             1

#define DESCRIPTION_FONT            FONT_SHORT_NARROW
#define DESCRIPTION_TILEMAP_LEFT    5
#define DESCRIPTION_TILEMAP_TOP     16
#define DESCRIPTION_WIDTH           20
#define DESCRIPTION_HEIGHT          4
#define DESCRIPTION_BASE_BLOCK      (LIST_BASE_BLOCK + LIST_WIDTH * LIST_HEIGHT)
#define DESCRIPTION_BUFFER_SIZE     128

#define FRAME_TILES                 9
#define FRAME_BASE_BLOCK            (DESCRIPTION_BASE_BLOCK + DESCRIPTION_WIDTH * DESCRIPTION_HEIGHT)
#define FRAME_PALETTE_NUM           2
#define FRAME_PREVIEW_TILEMAP_LEFT  19
#define FRAME_PREVIEW_WIDTH         8

#define CHOICE_X                    88
#define CHOICE_SPAN                 102

#define TEXT_PALETTE_NUM            1

#define SCROLL_TRACK_TILEMAP_LEFT   28
#define SCROLL_TRACK_ROWS           LIST_HEIGHT
#define SCROLL_TRACK_LEN            (OPTIONS_SHOWN * OPTION_ROW_HEIGHT)
#define SCROLL_THUMB_X              (SCROLL_TRACK_TILEMAP_LEFT * 8 + 4)
#define SCROLL_THUMB_MIN_LEN        8
#define SCROLL_THUMB_SPRITES_COUNT  3

#define CURSOR_X                    12
#define HOVER_SLOT_X                128
#define HOVER_SLOT_FRAME_TILES      ((32 * 16) / (8 * 8))
#define HOVER_SLOT_SHEET_TILES      36

#define TAG_SWSH_OPTION_UI_PAL      55400
#define TAG_OPTION_CURSOR           55401
#define TAG_OPTION_HOVER_SLOT       55402
#define TAG_OPTION_SCROLL_THUMB     55403

// priority 2
#define SUBPRIORITY_CURSOR          0
#define SUBPRIORITY_HOVER_SLOT      SUBPRIORITY_CURSOR + 1
#define SUBPRIORITY_SCROLL_THUMB    SUBPRIORITY_HOVER_SLOT + 1

struct OptionMenu
{
    u8 bg1TilemapBuffer[BG_SCREEN_SIZE];
    u8 bg2TilemapBuffer[BG_SCREEN_SIZE];
    u8 bg3TilemapBuffer[BG_SCREEN_SIZE];
    u8 descriptionBuffer[DESCRIPTION_BUFFER_SIZE];
    u16 scrollOffset;
    u16 selectedRow;
    u8 cursorSpriteId;
    u8 hoverSlotSpriteId;
    u8 scrollThumbSpriteIds[SCROLL_THUMB_SPRITES_COUNT];
    u32 cursorAnimId;
    u32 cursorBobAnimId;
    u32 scrollThumbAnimId;
};

static EWRAM_DATA struct OptionMenu *sOptionMenu = NULL;

static void CB2_OptionMenu(void);
static void VBlankCB_OptionMenu(void);
static void OptionMenu_InitBgs(void);
static void OptionMenu_InitWindows(void);
static void OptionMenu_LoadGraphics(void);
static void OptionMenu_DrawScrollTrack(void);
static void OptionMenu_CreateSprites(void);
static void OptionMenu_FreeResources(void);
static void OptionList_PrintAll(void);
static void OptionList_DrawTilemaps(void);
static void OptionMenu_PrintDescription(s8 speed);
static void OptionList_Move(bool32 movingDown, bool32 allowWrap);
static void OptionList_ChangeValue(bool32 increment);
static void SpriteCB_Cursor(struct Sprite *sprite);
static void SpriteCB_ScrollThumb(struct Sprite *sprite);
static void Task_OptionMenu(u8 taskId);
static void Task_OptionMenuFadeOut(u8 taskId);
static void Frame_GetLabel(u8 *dest, u32 value);
static void Frame_LoadGfx(u32 frameType);
static void Frame_DrawPreview(u32 tilemapTop);
static u32 GetOptionValue(enum SwShOptionId optionId);
static void SetOptionValue(enum SwShOptionId optionId, u32 value);

static const u32 sOptionMenu_Gfx[]      = INCGFX_U32("graphics/option/swsh/tiles.png", ".4bpp.smol");
static const u16 sOptionMenu_Pal[]      = INCGFX_U16("graphics/option/swsh/tiles.png", ".gbapal");
static const u32 sOptionMenu_BG2Map[]   = INCGFX_U32("graphics/option/swsh/bg2.bin", ".smolTM");
static const u32 sOptionMenu_BG3Map[]   = INCGFX_U32("graphics/option/swsh/bg3.bin", ".smolTM");
static const u8 sScrollTrack_Tiles[]    = INCBIN_U8("graphics/option/swsh/scroll_track.bin");
static const u32 sHoverSlot_Gfx[]       = INCGFX_U32("graphics/option/swsh/hover_slot.png", ".4bpp.smol");
static const u32 sScrollThumb_Gfx[]     = INCGFX_U32("graphics/option/swsh/scroll_thumb.png", ".4bpp.smol");
static const u16 sOptionMenuUI_Pal[]    = INCGFX_U16("graphics/option/swsh/scroll_thumb.png", ".gbapal");

static const u8 *const sTextSpeedChoices[] =
{
    [OPTIONS_TEXT_SPEED_SLOW] = COMPOUND_STRING("Slow"),
    [OPTIONS_TEXT_SPEED_MID]  = COMPOUND_STRING("Mid"),
    [OPTIONS_TEXT_SPEED_FAST] = COMPOUND_STRING("Fast"),
};

// indexed by optionsBattleSceneOff
static const u8 *const sBattleSceneChoices[] =
{
    [FALSE] = COMPOUND_STRING("On"),
    [TRUE]  = COMPOUND_STRING("Off"),
};

static const u8 *const sBattleStyleChoices[] =
{
    [OPTIONS_BATTLE_STYLE_SHIFT] = COMPOUND_STRING("Shift"),
    [OPTIONS_BATTLE_STYLE_SET]   = COMPOUND_STRING("Set"),
};

static const u8 *const sSoundChoices[] =
{
    [OPTIONS_SOUND_MONO]   = COMPOUND_STRING("Mono"),
    [OPTIONS_SOUND_STEREO] = COMPOUND_STRING("Stereo"),
};

static const u8 *const sButtonModeChoices[] =
{
    [OPTIONS_BUTTON_MODE_NORMAL]     = COMPOUND_STRING("Normal"),
    [OPTIONS_BUTTON_MODE_LR]         = COMPOUND_STRING("LR"),
    [OPTIONS_BUTTON_MODE_L_EQUALS_A] = COMPOUND_STRING("L=A"),
};

static const struct SwShOption sOptions[OPTION_COUNT] =
{
    [OPTION_TEXT_SPEED] =
    {
        .name = COMPOUND_STRING("Text Speed"),
        .description = COMPOUND_STRING("Choose text speed to control how quickly messages appear"),
        .choices = sTextSpeedChoices,
        .choiceCount = ARRAY_COUNT(sTextSpeedChoices),
    },
    [OPTION_BATTLE_SCENE] =
    {
        .name = COMPOUND_STRING("Battle Scene"),
        .description = COMPOUND_STRING("Choose whether to see move animations during battles"),
        .choices = sBattleSceneChoices,
        .choiceCount = ARRAY_COUNT(sBattleSceneChoices),
    },
    [OPTION_BATTLE_STYLE] =
    {
        .name = COMPOUND_STRING("Battle Style"),
        .description = COMPOUND_STRING("Choose whether to switch Pokémon after an opponent faints"),
        .choices = sBattleStyleChoices,
        .choiceCount = ARRAY_COUNT(sBattleStyleChoices),
    },
    [OPTION_SOUND] =
    {
        .name = COMPOUND_STRING("Sound"),
        .description = COMPOUND_STRING("Choose audio output to suit headphones or GBA speaker"),
        .choices = sSoundChoices,
        .choiceCount = ARRAY_COUNT(sSoundChoices),
    },
    [OPTION_BUTTON_MODE] =
    {
        .name = COMPOUND_STRING("Button Mode"),
        .description = COMPOUND_STRING("Choose how the L and R buttons function"),
        .choices = sButtonModeChoices,
        .choiceCount = ARRAY_COUNT(sButtonModeChoices),
    },
    [OPTION_FRAME] =
    {
        .name = COMPOUND_STRING("Frame"),
        .description = COMPOUND_STRING("Choose the decorative frame for non-dialogue text windows"),
        .choiceCount = WINDOW_FRAMES_COUNT,
        .getLabel = Frame_GetLabel,
        .drawTilemap = Frame_DrawPreview,
    },
};

static const u8 sTextColors[][3] =
{
    [COLORID_NORMAL]             = {0, 4, 5},
    [COLORID_FOCUSED]            = {0, 1, 2},
    [COLORID_NOT_CHOSEN]         = {0, 7, 8},
    [COLORID_CHOSEN_FOCUSED]     = {0, 1, 3},
    [COLORID_NOT_CHOSEN_FOCUSED] = {0, 6, 3},
    [COLORID_DESCRIPTION]        = {0, 1, 3},
};

static const struct BgTemplate sOptionMenuBgTemplates[] =
{
    {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
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

static const struct WindowTemplate sOptionMenuWindowTemplates[] =
{
    [WIN_LIST] =
    {
        .bg = 1,
        .tilemapLeft = LIST_TILEMAP_LEFT,
        .tilemapTop = LIST_TILEMAP_TOP,
        .width = LIST_WIDTH,
        .height = LIST_HEIGHT,
        .paletteNum = TEXT_PALETTE_NUM,
        .baseBlock = LIST_BASE_BLOCK,
    },
    [WIN_DESCRIPTION] =
    {
        .bg = 1,
        .tilemapLeft = DESCRIPTION_TILEMAP_LEFT,
        .tilemapTop = DESCRIPTION_TILEMAP_TOP,
        .width = DESCRIPTION_WIDTH,
        .height = DESCRIPTION_HEIGHT,
        .paletteNum = TEXT_PALETTE_NUM,
        .baseBlock = DESCRIPTION_BASE_BLOCK,
    },
    DUMMY_WIN_TEMPLATE,
};

static const struct OamData sOamData_Cursor =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .size = SPRITE_SIZE(16x16),
    .priority = 2,
};

static const struct CompressedSpriteSheet sSpriteSheet_Cursor =
{
    .data = gCursorSwSh_Gfx,
    .size = (16 * 16) / 2,
    .tag = TAG_OPTION_CURSOR,
};

static const struct SpriteTemplate sSpriteTemplate_Cursor =
{
    .tileTag = TAG_OPTION_CURSOR,
    .paletteTag = TAG_SWSH_UI_PAL,
    .oam = &sOamData_Cursor,
    .callback = SpriteCB_Cursor,
};

static const struct OamData sOamData_HoverSlot =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x16),
    .size = SPRITE_SIZE(32x16),
    .priority = 2,
};

static const struct CompressedSpriteSheet sSpriteSheet_HoverSlot =
{
    .data = sHoverSlot_Gfx,
    .size = HOVER_SLOT_SHEET_TILES * TILE_SIZE_4BPP,
    .tag = TAG_OPTION_HOVER_SLOT,
};

static const struct SpriteTemplate sSpriteTemplate_HoverSlot =
{
    .tileTag = TAG_OPTION_HOVER_SLOT,
    .paletteTag = TAG_SWSH_OPTION_UI_PAL,
    .oam = &sOamData_HoverSlot,
};

static const struct Subsprite sSubsprites_HoverSlot[] =
{
    { .x = -112, .y = -8, .shape = SPRITE_SHAPE(32x16), .size = SPRITE_SIZE(32x16), .tileOffset = 0,  .priority = 2 },
    { .x =  -80, .y = -8, .shape = SPRITE_SHAPE(32x16), .size = SPRITE_SIZE(32x16), .tileOffset = 5,  .priority = 2 },
    { .x =  -48, .y = -8, .shape = SPRITE_SHAPE(32x16), .size = SPRITE_SIZE(32x16), .tileOffset = 12, .priority = 2 },
    { .x =  -16, .y = -8, .shape = SPRITE_SHAPE(32x16), .size = SPRITE_SIZE(32x16), .tileOffset = 20, .priority = 2 },
    { .x =   16, .y = -8, .shape = SPRITE_SHAPE(32x16), .size = SPRITE_SIZE(32x16), .tileOffset = 20, .priority = 2 },
    { .x =   48, .y = -8, .shape = SPRITE_SHAPE(32x16), .size = SPRITE_SIZE(32x16), .tileOffset = 20, .priority = 2 },
    { .x =   80, .y = -8, .shape = SPRITE_SHAPE(32x16), .size = SPRITE_SIZE(32x16), .tileOffset = 28, .priority = 2 },
};

STATIC_ASSERT(28 + HOVER_SLOT_FRAME_TILES == HOVER_SLOT_SHEET_TILES, HoverSlotSheetTiles);

static const struct SubspriteTable sSubspriteTable_HoverSlot[] =
{
    {
        .subspriteCount = ARRAY_COUNT(sSubsprites_HoverSlot),
        .subsprites = sSubsprites_HoverSlot
    }
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
        sOptionMenu->cursorSpriteId = SPRITE_NONE;
        sOptionMenu->hoverSlotSpriteId = SPRITE_NONE;
        memset(sOptionMenu->scrollThumbSpriteIds, SPRITE_NONE, sizeof(sOptionMenu->scrollThumbSpriteIds));
        sOptionMenu->cursorAnimId = INVALID_COMFY_ANIM;
        sOptionMenu->cursorBobAnimId = INVALID_COMFY_ANIM;
        sOptionMenu->scrollThumbAnimId = INVALID_COMFY_ANIM;
        OptionMenu_InitBgs();
        OptionMenu_InitWindows();
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
        OptionList_PrintAll();
        OptionList_DrawTilemaps();
        OptionMenu_PrintDescription(TEXT_SKIP_DRAW);
        ScheduleBgCopyTilemapToVram(1);
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
    RunTextPrinters();
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
    SetBgTilemapBuffer(1, sOptionMenu->bg1TilemapBuffer);
    SetBgTilemapBuffer(2, sOptionMenu->bg2TilemapBuffer);
    SetBgTilemapBuffer(3, sOptionMenu->bg3TilemapBuffer);
    ResetAllBgsCoordinates();
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    ShowBg(1);
    ShowBg(2);
    ShowBg(3);
}

static void OptionMenu_InitWindows(void)
{
    InitWindows(sOptionMenuWindowTemplates);
    DeactivateAllTextPrinters();
    FillWindowPixelBuffer(WIN_LIST, PIXEL_FILL(0));
    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(0));
    PutWindowTilemap(WIN_LIST);
    PutWindowTilemap(WIN_DESCRIPTION);
}

static void OptionMenu_LoadGraphics(void)
{
    DecompressAndCopyTileDataToVram(2, sOptionMenu_Gfx, 0, 0, 0);
    DecompressDataWithHeaderWram(sOptionMenu_BG2Map, sOptionMenu->bg2TilemapBuffer);
    DecompressDataWithHeaderWram(sOptionMenu_BG3Map, sOptionMenu->bg3TilemapBuffer);
    LoadPalette(sOptionMenu_Pal, BG_PLTT_ID(0), sizeof(sOptionMenu_Pal));
    Frame_LoadGfx(GetOptionValue(OPTION_FRAME));
    LoadCompressedSpriteSheet(&sSpriteSheet_Cursor);
    LoadCompressedSpriteSheet(&sSpriteSheet_HoverSlot);
    LoadCompressedSpriteSheet(&sSpriteSheet_ScrollThumb);
    LoadSpritePalette(&sSpritePalette_OptionMenuUI);
    LoadSpritePalette(&gSpritePalette_SwShUI);
}

static void OptionMenu_DrawScrollTrack(void)
{
    u32 i;

    if (OPTION_COUNT <= OPTIONS_SHOWN)
        return;

    for (i = 0; i < SCROLL_TRACK_ROWS; i++)
        FillBgTilemapBufferRect(3, sScrollTrack_Tiles[i], SCROLL_TRACK_TILEMAP_LEFT, LIST_TILEMAP_TOP + i, 1, 1, 0);
}

static u32 GetOptionValue(enum SwShOptionId optionId)
{
    switch (optionId)
    {
    case OPTION_TEXT_SPEED:
        return gSaveBlock2Ptr->optionsTextSpeed;
    case OPTION_BATTLE_SCENE:
        return gSaveBlock2Ptr->optionsBattleSceneOff;
    case OPTION_BATTLE_STYLE:
        return gSaveBlock2Ptr->optionsBattleStyle;
    case OPTION_SOUND:
        return gSaveBlock2Ptr->optionsSound;
    case OPTION_BUTTON_MODE:
        return gSaveBlock2Ptr->optionsButtonMode;
    case OPTION_FRAME:
        return gSaveBlock2Ptr->optionsWindowFrameType;
    default:
        return 0;
    }
}

static void SetOptionValue(enum SwShOptionId optionId, u32 value)
{
    switch (optionId)
    {
    case OPTION_TEXT_SPEED:
        gSaveBlock2Ptr->optionsTextSpeed = value;
        break;
    case OPTION_BATTLE_SCENE:
        gSaveBlock2Ptr->optionsBattleSceneOff = value;
        break;
    case OPTION_BATTLE_STYLE:
        gSaveBlock2Ptr->optionsBattleStyle = value;
        break;
    case OPTION_SOUND:
        gSaveBlock2Ptr->optionsSound = value;
        SetPokemonCryStereo(value);
        break;
    case OPTION_BUTTON_MODE:
        gSaveBlock2Ptr->optionsButtonMode = value;
        break;
    case OPTION_FRAME:
        gSaveBlock2Ptr->optionsWindowFrameType = value;
        Frame_LoadGfx(value);
        break;
    default:
        break;
    }
}

static void Frame_GetLabel(u8 *dest, u32 value)
{
    dest = StringCopy(dest, COMPOUND_STRING("Type "));
    ConvertIntToDecimalStringN(dest, value + 1, STR_CONV_MODE_LEFT_ALIGN, 2);
}

static void Frame_LoadGfx(u32 frameType)
{
    const struct TilesPal *frame = GetWindowFrameTilesPal(frameType);

    LoadBgTiles(1, frame->tiles, FRAME_TILES * TILE_SIZE_4BPP, FRAME_BASE_BLOCK);
    LoadPalette(frame->pal, BG_PLTT_ID(FRAME_PALETTE_NUM), PLTT_SIZE_4BPP);
}

// skip the middle row in frame demo
static void Frame_DrawPreview(u32 tilemapTop)
{
    u32 left = FRAME_PREVIEW_TILEMAP_LEFT;
    u32 right = FRAME_PREVIEW_TILEMAP_LEFT + FRAME_PREVIEW_WIDTH - 1;
    u32 bottom = tilemapTop + 1;

    FillBgTilemapBufferRect(1, FRAME_BASE_BLOCK + FRAME_TILE_TOP_LEFT, left, tilemapTop, 1, 1, FRAME_PALETTE_NUM);
    FillBgTilemapBufferRect(1, FRAME_BASE_BLOCK + FRAME_TILE_TOP, left + 1, tilemapTop, FRAME_PREVIEW_WIDTH - 2, 1, FRAME_PALETTE_NUM);
    FillBgTilemapBufferRect(1, FRAME_BASE_BLOCK + FRAME_TILE_TOP_RIGHT, right, tilemapTop, 1, 1, FRAME_PALETTE_NUM);
    FillBgTilemapBufferRect(1, FRAME_BASE_BLOCK + FRAME_TILE_BOTTOM_LEFT, left, bottom, 1, 1, FRAME_PALETTE_NUM);
    FillBgTilemapBufferRect(1, FRAME_BASE_BLOCK + FRAME_TILE_BOTTOM, left + 1, bottom, FRAME_PREVIEW_WIDTH - 2, 1, FRAME_PALETTE_NUM);
    FillBgTilemapBufferRect(1, FRAME_BASE_BLOCK + FRAME_TILE_BOTTOM_RIGHT, right, bottom, 1, 1, FRAME_PALETTE_NUM);
}

static void OptionList_Print(const u8 *str, u32 x, u32 y, u32 colorId)
{
    AddTextPrinterParameterized4(WIN_LIST, LIST_FONT, x, y, 0, 0, sTextColors[colorId], TEXT_SKIP_DRAW, str);
}

static u32 OptionList_ChoiceColor(bool32 chosen, bool32 focused)
{
    if (focused)
        return chosen ? COLORID_CHOSEN_FOCUSED : COLORID_NOT_CHOSEN_FOCUSED;
    return chosen ? COLORID_NORMAL : COLORID_NOT_CHOSEN;
}

static void OptionList_PrintChoices(u32 optionId, u32 y, bool32 focused)
{
    const struct SwShOption *option = &sOptions[optionId];
    u32 value = GetOptionValue(optionId);
    u32 totalWidth = 0;
    u32 gaps, gap, extra, x, i;

    if (option->getLabel != NULL)
    {
        u8 label[OPTION_LABEL_LENGTH];

        option->getLabel(label, value);
        OptionList_Print(label, CHOICE_X, y, OptionList_ChoiceColor(TRUE, focused));
        return;
    }

    if (option->choiceCount == 1)
    {
        OptionList_Print(option->choices[0], CHOICE_X, y, OptionList_ChoiceColor(TRUE, focused));
        return;
    }

    for (i = 0; i < option->choiceCount; i++)
        totalWidth += GetStringWidth(LIST_FONT, option->choices[i], 0);

    gaps = option->choiceCount - 1;
    gap = (CHOICE_SPAN - totalWidth) / gaps;
    extra = (CHOICE_SPAN - totalWidth) % gaps;
    x = CHOICE_X;
    for (i = 0; i < option->choiceCount; i++)
    {
        OptionList_Print(option->choices[i], x, y, OptionList_ChoiceColor(i == value, focused));
        x += GetStringWidth(LIST_FONT, option->choices[i], 0) + gap + (i < extra);
    }
}

static void OptionList_PrintRow(u32 row)
{
    u32 optionId = sOptionMenu->scrollOffset + row;
    bool32 focused = (row == sOptionMenu->selectedRow);
    u32 y = row * OPTION_ROW_HEIGHT;

    if (row >= OPTIONS_SHOWN || optionId >= OPTION_COUNT)
        return;

    FillWindowPixelRect(WIN_LIST, PIXEL_FILL(0), 0, y, LIST_WIDTH * 8, OPTION_ROW_HEIGHT);
    OptionList_Print(sOptions[optionId].name, 0, y, focused ? COLORID_FOCUSED : COLORID_NORMAL);
    OptionList_PrintChoices(optionId, y, focused);
}

static void OptionList_PrintAll(void)
{
    u32 row;

    FillWindowPixelBuffer(WIN_LIST, PIXEL_FILL(0));
    for (row = 0; row < OPTIONS_SHOWN; row++)
        OptionList_PrintRow(row);
    CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
}

static void OptionList_DrawTilemaps(void)
{
    u32 row;

    PutWindowTilemap(WIN_LIST);
    for (row = 0; row < OPTIONS_SHOWN && sOptionMenu->scrollOffset + row < OPTION_COUNT; row++)
    {
        const struct SwShOption *option = &sOptions[sOptionMenu->scrollOffset + row];

        if (option->drawTilemap != NULL)
            option->drawTilemap(LIST_TILEMAP_TOP + row * OPTION_ROW_HEIGHT / 8);
    }
    ScheduleBgCopyTilemapToVram(1);
}

static void OptionMenu_PrintDescription(s8 speed)
{
    const u8 *description = sOptions[sOptionMenu->scrollOffset + sOptionMenu->selectedRow].description;

    DeactivateSingleTextPrinter(WIN_DESCRIPTION, WINDOW_TEXT_PRINTER);
    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(0));
    if (description == NULL)
    {
        HideBg(2);
    }
    else
    {
        u8 fontId = FormatDescriptionByWidth(sOptionMenu->descriptionBuffer, DESCRIPTION_BUFFER_SIZE, DESCRIPTION_WIDTH * 8,
                                             DESCRIPTION_FONT, description,
                                             GetFontAttribute(DESCRIPTION_FONT, FONTATTR_LETTER_SPACING));

        ShowBg(2);
        AddTextPrinterParameterized4(WIN_DESCRIPTION, fontId, 0, 0, 0, 0, sTextColors[COLORID_DESCRIPTION],
                                     speed, sOptionMenu->descriptionBuffer);
    }
    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_GFX);
}

static s16 OptionList_RowSpriteY(u32 row)
{
    return LIST_TILEMAP_TOP * 8 + row * OPTION_ROW_HEIGHT + OPTION_ROW_HEIGHT / 2;
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

static void SpriteCB_Cursor(struct Sprite *sprite)
{
    s16 y;

    UpdateCursorBob(sprite, sOptionMenu->cursorBobAnimId);

    if (sOptionMenu->cursorAnimId == INVALID_COMFY_ANIM)
        return;

    y = ReadComfyAnimValueSmooth(&gComfyAnims[sOptionMenu->cursorAnimId]);
    sprite->y = y;

    if (sOptionMenu->hoverSlotSpriteId != SPRITE_NONE)
        gSprites[sOptionMenu->hoverSlotSpriteId].y = y;
}

static void OptionMenu_CreateSprites(void)
{
    s16 initialY = OptionList_RowSpriteY(sOptionMenu->selectedRow);
    s16 initialThumbY2 = ScrollThumb_Offset(sOptionMenu->scrollOffset + sOptionMenu->selectedRow);
    u32 i;

    sOptionMenu->cursorAnimId = CreateComfyAnim_Easing(&(struct ComfyAnimEasingConfig){
        .from = Q_24_8(initialY),
        .to = Q_24_8(initialY),
        .durationFrames = 1,
        .easingFunc = ComfyAnimEasing_EaseOutCubic,
    });
    sOptionMenu->scrollThumbAnimId = CreateComfyAnim_Easing(&(struct ComfyAnimEasingConfig){
        .from = Q_24_8(initialThumbY2),
        .to = Q_24_8(initialThumbY2),
        .durationFrames = 1,
        .easingFunc = ComfyAnimEasing_EaseOutCubic,
    });

    sOptionMenu->hoverSlotSpriteId = CreateSprite(&sSpriteTemplate_HoverSlot, HOVER_SLOT_X, initialY, SUBPRIORITY_HOVER_SLOT);
    SetSubspriteTables(&gSprites[sOptionMenu->hoverSlotSpriteId], sSubspriteTable_HoverSlot);

    for (i = 0; i < SCROLL_THUMB_SPRITES_COUNT; i++)
    {
        u8 spriteId = CreateSprite(&sSpriteTemplate_ScrollThumb, SCROLL_THUMB_X, 0, SUBPRIORITY_SCROLL_THUMB);

        sOptionMenu->scrollThumbSpriteIds[i] = spriteId;
        gSprites[spriteId].data[0] = i;
        gSprites[spriteId].invisible = TRUE;
    }

    sOptionMenu->cursorSpriteId = CreateSprite(&sSpriteTemplate_Cursor, CURSOR_X, initialY, SUBPRIORITY_CURSOR);
    StartCursorBob(sOptionMenu->cursorSpriteId, &sOptionMenu->cursorBobAnimId);
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

static void OptionList_AnimateCursor(u16 absPos)
{
    u32 durationFrames = 8;

    if (sOptionMenu->cursorAnimId != INVALID_COMFY_ANIM)
    {
        struct ComfyAnim *cursorAnim = &gComfyAnims[sOptionMenu->cursorAnimId];

        if (!cursorAnim->completed)
            durationFrames = (gMain.heldKeys & (DPAD_UP | DPAD_DOWN)) ? 1 : 2;

        InitComfyAnim_Easing(&(struct ComfyAnimEasingConfig){
            .from = cursorAnim->position,
            .to = Q_24_8(OptionList_RowSpriteY(sOptionMenu->selectedRow)),
            .durationFrames = durationFrames,
            .easingFunc = ComfyAnimEasing_EaseOutCubic,
        }, cursorAnim);
    }

    if (OPTION_COUNT > OPTIONS_SHOWN && sOptionMenu->scrollThumbAnimId != INVALID_COMFY_ANIM)
    {
        struct ComfyAnim *thumbAnim = &gComfyAnims[sOptionMenu->scrollThumbAnimId];
        s32 maxOffset = Q_24_8(SCROLL_TRACK_LEN - ScrollThumb_Length());

        InitComfyAnim_Easing(&(struct ComfyAnimEasingConfig){
            .from = min(thumbAnim->position, maxOffset),
            .to = Q_24_8(ScrollThumb_Offset(absPos)),
            .durationFrames = durationFrames,
            .easingFunc = ComfyAnimEasing_EaseOutCubic,
        }, thumbAnim);
    }
}

static void OptionList_Move(bool32 movingDown, bool32 allowWrap)
{
    u16 oldScroll = sOptionMenu->scrollOffset;
    u16 oldAbs = oldScroll + sOptionMenu->selectedRow;
    u16 abs = oldAbs;

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

    if (sOptionMenu->scrollOffset == oldScroll + 1)
    {
        ScrollWindow(WIN_LIST, 0, OPTION_ROW_HEIGHT, PIXEL_FILL(0));
        OptionList_PrintRow(OPTIONS_SHOWN - 1);
    }
    else if (sOptionMenu->scrollOffset + 1 == oldScroll)
    {
        ScrollWindow(WIN_LIST, 1, OPTION_ROW_HEIGHT, PIXEL_FILL(0));
        OptionList_PrintRow(0);
    }
    else if (sOptionMenu->scrollOffset != oldScroll)
    {
        OptionList_PrintAll();
    }

    if (oldAbs >= sOptionMenu->scrollOffset)
        OptionList_PrintRow(oldAbs - sOptionMenu->scrollOffset);
    OptionList_PrintRow(sOptionMenu->selectedRow);
    CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
    if (sOptionMenu->scrollOffset != oldScroll)
        OptionList_DrawTilemaps();

    OptionMenu_PrintDescription(TEXT_SKIP_DRAW);
    OptionList_AnimateCursor(abs);
}

static void OptionList_ChangeValue(bool32 increment)
{
    u32 optionId = sOptionMenu->scrollOffset + sOptionMenu->selectedRow;
    const struct SwShOption *option = &sOptions[optionId];
    u32 value = GetOptionValue(optionId);

    if (increment)
        value = (value + 1) % option->choiceCount;
    else
        value = (value == 0 ? option->choiceCount : value) - 1;

    SetOptionValue(optionId, value);

    OptionList_PrintRow(sOptionMenu->selectedRow);
    CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
    if (optionId == OPTION_TEXT_SPEED)
        OptionMenu_PrintDescription(GetPlayerTextSpeedDelay());
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
    else if (JOY_NEW(DPAD_LEFT))
    {
        OptionList_ChangeValue(FALSE);
    }
    else if (JOY_NEW(DPAD_RIGHT))
    {
        OptionList_ChangeValue(TRUE);
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
    FreeSpriteTilesByTag(TAG_OPTION_CURSOR);
    FreeSpriteTilesByTag(TAG_OPTION_HOVER_SLOT);
    FreeSpriteTilesByTag(TAG_OPTION_SCROLL_THUMB);
    FreeSpritePaletteByTag(TAG_SWSH_OPTION_UI_PAL);
    FreeSpritePaletteByTag(TAG_SWSH_UI_PAL);
    ReleaseComfyAnims();
    DeactivateSingleTextPrinter(WIN_DESCRIPTION, WINDOW_TEXT_PRINTER);
    FreeAllWindowBuffers();
    Free(sOptionMenu);
    sOptionMenu = NULL;
}
