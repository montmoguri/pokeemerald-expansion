#include "global.h"
#include "bg.h"
#include "comfy_anim.h"
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
#include "swsh_graphics.h"
#include "swsh_utils.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "constants/rgb.h"
#include "swsh_pokedex.h"

#define BG3_SLIDE_DISTANCE      100
#define BG3_SLIDE_EASE_DIV      4

#define BLEND_MAX               16
#define BLEND_STEP              3
#define LIST_FADE_STEP          4

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

#define WIN_LIST_WIDTH                  16
#define WIN_LIST_HEIGHT                 14
#define WIN_LIST_SORT_TYPE_WIDTH        12
#define WIN_LIST_SORT_TYPE_HEIGHT       3
#define WIN_LIST_CAUGHT_SEEN_NUM_WIDTH  9
#define WIN_LIST_CAUGHT_SEEN_NUM_HEIGHT 3
#define WIN_LIST_REGION_WIDTH           5
#define WIN_LIST_REGION_HEIGHT          3

#define WIN_ENTRY_HEADER_WIDTH          19
#define WIN_ENTRY_HEADER_HEIGHT         3
#define WIN_ENTRY_CATEGORY_WIDTH        15
#define WIN_ENTRY_CATEGORY_HEIGHT       2
#define WIN_ENTRY_DESCRIPTION_WIDTH     26
#define WIN_ENTRY_DESCRIPTION_HEIGHT    9
#define WIN_ENTRY_STATS_WIDTH           17
#define WIN_ENTRY_STATS_HEIGHT          4

#define WIN_BASE_BLOCK_AFTER(prev)  (WIN_##prev##_BASE_BLOCK + WIN_##prev##_WIDTH * WIN_##prev##_HEIGHT)

enum
{
    WIN_LIST_BASE_BLOCK                 = 1,
    WIN_LIST_SORT_TYPE_BASE_BLOCK       = WIN_BASE_BLOCK_AFTER(LIST),
    WIN_LIST_CAUGHT_SEEN_NUM_BASE_BLOCK = WIN_BASE_BLOCK_AFTER(LIST_SORT_TYPE),
    WIN_LIST_REGION_BASE_BLOCK          = WIN_BASE_BLOCK_AFTER(LIST_CAUGHT_SEEN_NUM),
    WIN_ENTRY_HEADER_BASE_BLOCK         = WIN_LIST_BASE_BLOCK,
    WIN_ENTRY_CATEGORY_BASE_BLOCK       = WIN_BASE_BLOCK_AFTER(ENTRY_HEADER),
    WIN_ENTRY_DESCRIPTION_BASE_BLOCK    = WIN_BASE_BLOCK_AFTER(ENTRY_CATEGORY),
    WIN_ENTRY_STATS_BASE_BLOCK          = WIN_BASE_BLOCK_AFTER(ENTRY_DESCRIPTION),
};

#define BALL_WIDTH              8
#define BALL_HEIGHT             16
#define BALL_TILE_COUNT         2
#define CAUGHT_BALL_X           0
#define SEEN_BALL_X             40
#define CAUGHT_NUM_X            11
#define SEEN_NUM_X              51
#define COUNT_NUM_SPAN          20

#define LIST_ROWS_SHOWN         7
#define LIST_ROW_HEIGHT         16
#define LIST_SHIFT_UP           0
#define LIST_SHIFT_DOWN         1
#define DEX_SMALL_FONT_Y        1
#define LIST_INDEX_X            4
#define LIST_NAME_X             54
#define LIST_NAME_FIT_WIDTH     60
#define LIST_BALL_X             116
#define DEX_NUM_DIGITS          4

#define ENTRY_TEXT_Y            6
#define ENTRY_INDEX_X           4
#define ENTRY_NAME_X            56
#define ENTRY_NAME_FIT_WIDTH    64
#define ENTRY_BALL_X            140
#define ENTRY_BALL_Y            6

#define TAG_DEX_SPRITE_PAL      55400
#define TAG_DEX_CURSOR          55401
#define TAG_DEX_HOVER_BAR       55402
#define TAG_DEX_SCROLL_THUMB    55403

#define LIST_TOP_Y              (sWindowTemplates[WIN_LIST].tilemapTop * TILE_HEIGHT)
#define LIST_ROW_CENTER_Y(row)  (LIST_TOP_Y + LIST_ROW_HEIGHT / 2 + LIST_ROW_HEIGHT * (row))
#define LIST_SPRITE_PRIORITY    2
#define SUBPRIORITY_CURSOR      0
#define SUBPRIORITY_HOVER_BAR   (SUBPRIORITY_CURSOR + 1)
#define SUBPRIORITY_THUMB       (SUBPRIORITY_HOVER_BAR + 1)

#define CURSOR_X                97
#define CURSOR_SIZE             16
#define HOVER_BAR_X             167
#define HOVER_BAR_PIECES        4
#define HOVER_BAR_PIECE_WIDTH   32
#define HOVER_BAR_PIECE_HEIGHT  16
#define HOVER_BAR_PIECE_TILES   ((HOVER_BAR_PIECE_WIDTH * HOVER_BAR_PIECE_HEIGHT) / (TILE_WIDTH * TILE_HEIGHT))
#define HOVER_BAR_PIECE_X(i)    ((i) * HOVER_BAR_PIECE_WIDTH - HOVER_BAR_PIECE_WIDTH * HOVER_BAR_PIECES / 2)

#define SCROLL_THUMB_X          236
#define SCROLL_THUMB_WIDTH      8
#define SCROLL_THUMB_HEIGHT     32
#define SCROLL_THUMB_SEGMENTS   4
#define SCROLL_THUMB_MIN_LEN    8
#define SCROLL_TRACK_LEN        (WIN_LIST_HEIGHT * TILE_HEIGHT)

#define SLIDE_FRAMES            8
#define SLIDE_FRAMES_HELD       1
#define SLIDE_FRAMES_INTERRUPT  2

enum DexPage
{
    DEX_PAGE_LIST,
    DEX_PAGE_ENTRY,
    DEX_PAGE_HABITAT,
};

enum DexWindow
{
    WIN_LIST,
    WIN_LIST_SORT_TYPE,
    WIN_LIST_CAUGHT_SEEN_NUM,
    WIN_LIST_REGION,
    WIN_ENTRY_HEADER,
    WIN_ENTRY_CATEGORY,
    WIN_ENTRY_DESCRIPTION,
    WIN_ENTRY_STATS,
    WIN_COUNT,
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
    COLORID_LIST_SORT_TYPE,
    COLORID_LIST_COUNT_NUM,
    COLORID_LIST_REGION,
    COLORID_LIST_UNSEEN,
    COLORID_LIST_NORMAL,
    COLORID_LIST_HOVER_INDEX,
    COLORID_LIST_HOVER_NAME,
    COLORID_ENTRY_INDEX = COLORID_LIST_HOVER_INDEX,
    COLORID_ENTRY_NAME = COLORID_LIST_HOVER_NAME,
};

struct SwShPokedex
{
    u16 bg1TilemapBuffer[BG_SCREEN_SIZE / 2];
    u16 bg2TilemapBuffer[BG_SCREEN_SIZE / 2];
    u16 bg3TilemapBuffer[2][BG_SCREEN_SIZE / 2];
    s16 bg3X;
    u8 page;
    u8 bg1Alpha;
    u8 bg1Page;
    u8 spriteAlpha;
    u8 bg2Page;
    u8 bg2Alpha;
    u8 bg3White;
    bool8 bg3Habitat;
    bool8 bg3Loading;
    u16 listNatNum[NATIONAL_DEX_COUNT];
    u16 listCount;
    u16 listCursor;
    u16 listScroll;
    u8 cursorSpriteId;
    u8 hoverBarSpriteId;
    u8 scrollThumbSpriteIds[SCROLL_THUMB_SEGMENTS];
    u8 cursorAnimId;
    u8 scrollThumbAnimId;
    u32 cursorBobAnimId;
};

static void SpriteCB_SlideCursorY(struct Sprite *);
static void SpriteCB_ScrollThumb(struct Sprite *);

static EWRAM_DATA struct SwShPokedex *sPokedex = NULL;

static const u32 sSwShPokedex_Gfx[]                 = INCGFX_U32("graphics/pokedex/swsh/tiles.png", ".4bpp.smol");
static const u16 sSwShPokedex_Pal[]                 = INCGFX_U16("graphics/pokedex/swsh/tiles.png", ".gbapal");
static const u32 sSwShPokedex_BG3a_Tilemap[]        = INCGFX_U32("graphics/pokedex/swsh/bg3a.bin", ".smolTM");
static const u32 sSwShPokedex_BG3b_Tilemap[]        = INCGFX_U32("graphics/pokedex/swsh/bg3b.bin", ".smolTM");
static const u32 sSwShPokedex_BG2List_Tilemap[]     = INCGFX_U32("graphics/pokedex/swsh/bg2_list.bin", ".smolTM");
static const u32 sSwShPokedex_BG2Entry_Tilemap[]    = INCGFX_U32("graphics/pokedex/swsh/bg2_entry.bin", ".smolTM");
static const u8 sCaughtBall_Gfx[]                   = INCGFX_U8("graphics/pokedex/swsh/caught_ball.png", ".4bpp");

static const u32 sHoverBar_Gfx[]                    = INCGFX_U32("graphics/pokedex/swsh/hover_bar.png", ".4bpp.smol");
static const u16 sHoverBar_Pal[]                    = INCGFX_U16("graphics/pokedex/swsh/hover_bar.png", ".gbapal");
static const u32 sScrollThumb_Gfx[]                 = INCGFX_U32("graphics/pokedex/swsh/scroll_thumb.png", ".4bpp.smol");

static const u8 sTextColors[][3] =
{
    [COLORID_LIST_SORT_TYPE]    = {0, 1, 2 },
    [COLORID_LIST_COUNT_NUM]    = {0, 1, 5 },
    [COLORID_LIST_REGION]       = {0, 3, 6 },
    [COLORID_LIST_UNSEEN]       = {0, 9, 4 },
    [COLORID_LIST_NORMAL]       = {0, 3, 4 },
    [COLORID_LIST_HOVER_INDEX]  = {0, 1, 10},
    [COLORID_LIST_HOVER_NAME]   = {0, 1, 2 },
};

static const u32 *const sBg2Tilemaps[] =
{
    [DEX_PAGE_LIST]     = sSwShPokedex_BG2List_Tilemap,
    [DEX_PAGE_ENTRY]    = sSwShPokedex_BG2Entry_Tilemap,
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
    [WIN_LIST] =
    {
        .bg = 1,
        .tilemapLeft = 13,
        .tilemapTop = 3,
        .width = WIN_LIST_WIDTH,
        .height = WIN_LIST_HEIGHT,
        .paletteNum = DEX_TEXT_PAL_NUM,
        .baseBlock = WIN_LIST_BASE_BLOCK,
    },
    [WIN_LIST_SORT_TYPE] =
    {
        .bg = 1,
        .tilemapLeft = 17,
        .tilemapTop = 0,
        .width = WIN_LIST_SORT_TYPE_WIDTH,
        .height = WIN_LIST_SORT_TYPE_HEIGHT,
        .paletteNum = DEX_TEXT_PAL_NUM,
        .baseBlock = WIN_LIST_SORT_TYPE_BASE_BLOCK,
    },
    [WIN_LIST_CAUGHT_SEEN_NUM] =
    {
        .bg = 1,
        .tilemapLeft = 7,
        .tilemapTop = 0,
        .width = WIN_LIST_CAUGHT_SEEN_NUM_WIDTH,
        .height = WIN_LIST_CAUGHT_SEEN_NUM_HEIGHT,
        .paletteNum = DEX_TEXT_PAL_NUM,
        .baseBlock = WIN_LIST_CAUGHT_SEEN_NUM_BASE_BLOCK,
    },
    [WIN_LIST_REGION] =
    {
        .bg = 1,
        .tilemapLeft = 1,
        .tilemapTop = 0,
        .width = WIN_LIST_REGION_WIDTH,
        .height = WIN_LIST_REGION_HEIGHT,
        .paletteNum = DEX_TEXT_PAL_NUM,
        .baseBlock = WIN_LIST_REGION_BASE_BLOCK,
    },
    [WIN_ENTRY_HEADER] =
    {
        .bg = 1,
        .tilemapLeft = 10,
        .tilemapTop = 0,
        .width = WIN_ENTRY_HEADER_WIDTH,
        .height = WIN_ENTRY_HEADER_HEIGHT,
        .paletteNum = DEX_TEXT_PAL_NUM,
        .baseBlock = WIN_ENTRY_HEADER_BASE_BLOCK,
    },
    [WIN_ENTRY_CATEGORY] =
    {
        .bg = 1,
        .tilemapLeft = 13,
        .tilemapTop = 3,
        .width = WIN_ENTRY_CATEGORY_WIDTH,
        .height = WIN_ENTRY_CATEGORY_HEIGHT,
        .paletteNum = DEX_TEXT_PAL_NUM,
        .baseBlock = WIN_ENTRY_CATEGORY_BASE_BLOCK,
    },
    [WIN_ENTRY_DESCRIPTION] =
    {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 9,
        .width = WIN_ENTRY_DESCRIPTION_WIDTH,
        .height = WIN_ENTRY_DESCRIPTION_HEIGHT,
        .paletteNum = DEX_TEXT_PAL_NUM,
        .baseBlock = WIN_ENTRY_DESCRIPTION_BASE_BLOCK,
    },
    [WIN_ENTRY_STATS] =
    {
        .bg = 1,
        .tilemapLeft = 12,
        .tilemapTop = 5,
        .width = WIN_ENTRY_STATS_WIDTH,
        .height = WIN_ENTRY_STATS_HEIGHT,
        .paletteNum = DEX_TEXT_PAL_NUM,
        .baseBlock = WIN_ENTRY_STATS_BASE_BLOCK,
    },
    DUMMY_WIN_TEMPLATE,
};

struct DexPageWindows
{
    u8 first;
    u8 end;
};

static const struct DexPageWindows sPageWindows[] =
{
    [DEX_PAGE_LIST]     = {WIN_LIST, WIN_ENTRY_HEADER},
    [DEX_PAGE_ENTRY]    = {WIN_ENTRY_HEADER, WIN_COUNT},
};

static const struct SpritePalette sSpritePalette_Dex =
{
    .data = sHoverBar_Pal,
    .tag = TAG_DEX_SPRITE_PAL,
};

static const struct OamData sOamData_Cursor =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .size = SPRITE_SIZE(16x16),
    .priority = LIST_SPRITE_PRIORITY,
};

static const struct CompressedSpriteSheet sSpriteSheet_Cursor =
{
    .data = gCursorSwSh_Gfx,
    .size = (CURSOR_SIZE * CURSOR_SIZE) / 2,
    .tag = TAG_DEX_CURSOR
};

static const struct SpriteTemplate sSpriteTemplate_Cursor =
{
    .tileTag = TAG_DEX_CURSOR,
    .paletteTag = TAG_SWSH_UI_PAL,
    .oam = &sOamData_Cursor,
    .callback = SpriteCB_SlideCursorY,
};

static const struct OamData sOamData_HoverBar =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x16),
    .size = SPRITE_SIZE(32x16),
    .priority = LIST_SPRITE_PRIORITY,
};

static const struct CompressedSpriteSheet sSpriteSheet_HoverBar =
{
    .data = sHoverBar_Gfx,
    .size = (HOVER_BAR_PIECE_WIDTH * HOVER_BAR_PIECE_HEIGHT * HOVER_BAR_PIECES) / 2,
    .tag = TAG_DEX_HOVER_BAR
};

static const struct SpriteTemplate sSpriteTemplate_HoverBar =
{
    .tileTag = TAG_DEX_HOVER_BAR,
    .paletteTag = TAG_DEX_SPRITE_PAL,
    .oam = &sOamData_HoverBar,
};

static const struct Subsprite sSubsprites_HoverBar[] =
{
    { .x = HOVER_BAR_PIECE_X(0), .y = -HOVER_BAR_PIECE_HEIGHT / 2, .shape = SPRITE_SHAPE(32x16), .size = SPRITE_SIZE(32x16), .tileOffset = 0 * HOVER_BAR_PIECE_TILES, .priority = LIST_SPRITE_PRIORITY },
    { .x = HOVER_BAR_PIECE_X(1), .y = -HOVER_BAR_PIECE_HEIGHT / 2, .shape = SPRITE_SHAPE(32x16), .size = SPRITE_SIZE(32x16), .tileOffset = 1 * HOVER_BAR_PIECE_TILES, .priority = LIST_SPRITE_PRIORITY },
    { .x = HOVER_BAR_PIECE_X(2), .y = -HOVER_BAR_PIECE_HEIGHT / 2, .shape = SPRITE_SHAPE(32x16), .size = SPRITE_SIZE(32x16), .tileOffset = 2 * HOVER_BAR_PIECE_TILES, .priority = LIST_SPRITE_PRIORITY },
    { .x = HOVER_BAR_PIECE_X(3), .y = -HOVER_BAR_PIECE_HEIGHT / 2, .shape = SPRITE_SHAPE(32x16), .size = SPRITE_SIZE(32x16), .tileOffset = 3 * HOVER_BAR_PIECE_TILES, .priority = LIST_SPRITE_PRIORITY },
};

static const struct SubspriteTable sSubspriteTable_HoverBar[] =
{
    {
        .subspriteCount = ARRAY_COUNT(sSubsprites_HoverBar),
        .subsprites = sSubsprites_HoverBar
    }
};

static const struct OamData sOamData_ScrollThumb =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(8x32),
    .size = SPRITE_SIZE(8x32),
    .priority = LIST_SPRITE_PRIORITY,
};

static const struct CompressedSpriteSheet sSpriteSheet_ScrollThumb =
{
    .data = sScrollThumb_Gfx,
    .size = (SCROLL_THUMB_WIDTH * SCROLL_THUMB_HEIGHT) / 2,
    .tag = TAG_DEX_SCROLL_THUMB
};

static const struct SpriteTemplate sSpriteTemplate_ScrollThumb =
{
    .tileTag = TAG_DEX_SCROLL_THUMB,
    .paletteTag = TAG_DEX_SPRITE_PAL,
    .oam = &sOamData_ScrollThumb,
    .callback = SpriteCB_ScrollThumb,
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
    AdvanceComfyAnimations();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void BlitCaughtBall(u32 windowId, enum DexBall ball, u32 x, u32 y)
{
    BlitBitmapToWindow(windowId, sCaughtBall_Gfx + ball * BALL_TILE_COUNT * TILE_SIZE_4BPP, x, y, BALL_WIDTH, BALL_HEIGHT);
}

static void PrintDexText(u32 windowId, u32 fontId, u32 x, u32 y, enum DexTextColor color, const u8 *str)
{
    AddTextPrinterParameterized3(windowId, fontId, x, y, sTextColors[color], TEXT_SKIP_DRAW, str);
}

static void PrintCount(u32 windowId, u32 x, u32 count)
{
    ConvertIntToDecimalStringN(gStringVar1, count, STR_CONV_MODE_LEFT_ALIGN, DEX_COUNT_DIGITS);
    PrintDexText(windowId, FONT_NARROW, x + GetStringCenterAlignXOffset(FONT_NARROW, gStringVar1, COUNT_NUM_SPAN),
                 DEX_TEXT_Y, COLORID_LIST_COUNT_NUM, gStringVar1);
}

static void BuildDexList(void)
{
    u32 first = 0;
    u32 last = 0;

    for (u32 natNum = 1; natNum <= NATIONAL_DEX_COUNT; natNum++)
    {
        if (GetSetPokedexFlag(natNum, FLAG_GET_SEEN))
        {
            if (first == 0)
            {
                first = natNum;
            }
            last = natNum;
        }
    }

    sPokedex->listCount = 0;
    for (u32 natNum = first; natNum <= last; natNum++)
    {
        sPokedex->listNatNum[sPokedex->listCount++] = natNum;
    }
}

static bool32 IsListIndexSeen(u32 index)
{
    return GetSetPokedexFlag(sPokedex->listNatNum[index], FLAG_GET_SEEN);
}

static void PrintDexNumber(u32 windowId, u32 x, u32 y, enum DexTextColor color, u32 natNum)
{
    const u8 *prefix = COMPOUND_STRING("{NO}.{CLEAR 2}");

    PrintDexText(windowId, FONT_NARROW, x, y, color, prefix);
    ConvertIntToDecimalStringN(gStringVar1, natNum, STR_CONV_MODE_LEADING_ZEROS, DEX_NUM_DIGITS);
    PrintDexText(windowId, FONT_SMALL, x + GetStringWidth(FONT_NARROW, prefix, 0), y + DEX_SMALL_FONT_Y, color, gStringVar1);
}

static void PrintRow(u32 row)
{
    u32 index = sPokedex->listScroll + row;
    u32 y = row * LIST_ROW_HEIGHT;

    FillWindowPixelRect(WIN_LIST, PIXEL_FILL(0), 0, y, WIN_LIST_WIDTH * TILE_WIDTH, LIST_ROW_HEIGHT);
    if (index >= sPokedex->listCount)
    {
        return;
    }

    u32 natNum = sPokedex->listNatNum[index];
    bool32 hover = (index == sPokedex->listCursor);
    bool32 seen = IsListIndexSeen(index);
    enum DexTextColor indexColor = COLORID_LIST_UNSEEN;
    enum DexTextColor nameColor = COLORID_LIST_UNSEEN;

    if (seen)
    {
        indexColor = hover ? COLORID_LIST_HOVER_INDEX : COLORID_LIST_NORMAL;
        nameColor = hover ? COLORID_LIST_HOVER_NAME : COLORID_LIST_NORMAL;
    }

    PrintDexNumber(WIN_LIST, LIST_INDEX_X, y, indexColor, natNum);

    if (!seen)
    {
        PrintDexText(WIN_LIST, FONT_SMALL, LIST_NAME_X, y + DEX_SMALL_FONT_Y, nameColor, COMPOUND_STRING("?????"));
        return;
    }

    const u8 *name = GetSpeciesName(NationalPokedexNumToSpecies(natNum));

    PrintDexText(WIN_LIST, GetFontIdToFit(name, FONT_NARROW, 0, LIST_NAME_FIT_WIDTH), LIST_NAME_X, y, nameColor, name);
    if (GetSetPokedexFlag(natNum, FLAG_GET_CAUGHT))
    {
        BlitCaughtBall(WIN_LIST, hover ? BALL_CAUGHT_HOVER : BALL_CAUGHT_NORMAL, LIST_BALL_X, y);
    }
    else
    {
        BlitCaughtBall(WIN_LIST, hover ? BALL_SEEN_HOVER : BALL_SEEN_NORMAL, LIST_BALL_X, y);
    }
}

static void PrintAll(void)
{
    FillWindowPixelBuffer(WIN_LIST, PIXEL_FILL(0));
    for (u32 row = 0; row < LIST_ROWS_SHOWN; row++)
    {
        PrintRow(row);
    }
}

static u32 ScrollThumb_Length(u32 total)
{
    if (total <= LIST_ROWS_SHOWN)
    {
        return 0;
    }
    return max(SCROLL_TRACK_LEN * LIST_ROWS_SHOWN / total, SCROLL_THUMB_MIN_LEN);
}

static u32 ScrollThumb_Offset(u32 total, u32 index)
{
    u32 len = ScrollThumb_Length(total);

    if (len == 0)
    {
        return 0;
    }
    return index * (SCROLL_TRACK_LEN - len) / (total - 1);
}

static u32 ScrollThumb_SegmentSize(u32 len)
{
    if (len >= SCROLL_THUMB_HEIGHT)
    {
        return SCROLL_THUMB_HEIGHT;
    }
    if (len >= SCROLL_THUMB_HEIGHT / 2)
    {
        return SCROLL_THUMB_HEIGHT / 2;
    }
    return SCROLL_THUMB_MIN_LEN;
}

static u32 ScrollThumb_SegmentCount(u32 len)
{
    u32 size = ScrollThumb_SegmentSize(len);

    return (len + size - 1) / size;
}

static u32 ScrollThumb_SegmentOffset(u32 len, u32 index)
{
    u32 size = ScrollThumb_SegmentSize(len);

    return (index == ScrollThumb_SegmentCount(len) - 1) ? len - size : index * size;
}

static void ScrollThumb_SetSegmentOam(struct Sprite *sprite, u32 size)
{
    switch (size)
    {
    case SCROLL_THUMB_HEIGHT:
        sprite->oam.shape = SPRITE_SHAPE(8x32);
        sprite->oam.size = SPRITE_SIZE(8x32);
        break;
    case SCROLL_THUMB_HEIGHT / 2:
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

static void SpriteCB_SlideCursorY(struct Sprite *sprite)
{
    UpdateCursorBob(sprite, sPokedex->cursorBobAnimId);

    sprite->y = ReadComfyAnimValueSmooth(&gComfyAnims[sPokedex->cursorAnimId]);
    gSprites[sPokedex->hoverBarSpriteId].y = sprite->y;
    gSprites[sPokedex->hoverBarSpriteId].invisible = !(sPokedex->spriteAlpha > 0 && IsListIndexSeen(sPokedex->listCursor));
}

static void SpriteCB_ScrollThumb(struct Sprite *sprite)
{
    u32 len = ScrollThumb_Length(sPokedex->listCount);
    u32 index = sprite->data[0];

    if (sPokedex->spriteAlpha == 0 || len == 0 || index >= ScrollThumb_SegmentCount(len))
    {
        sprite->invisible = TRUE;
        return;
    }

    u32 size = ScrollThumb_SegmentSize(len);
    ScrollThumb_SetSegmentOam(sprite, size);
    sprite->y = LIST_TOP_Y + size / 2 + ScrollThumb_SegmentOffset(len, index);
    sprite->y2 = ReadComfyAnimValueSmooth(&gComfyAnims[sPokedex->scrollThumbAnimId]);
    sprite->invisible = FALSE;
}

static void LoadListSpriteGfx(void)
{
    LoadSpritePalette(&gSpritePalette_SwShUI);
    LoadSpritePalette(&sSpritePalette_Dex);
    LoadCompressedSpriteSheet(&sSpriteSheet_Cursor);
    LoadCompressedSpriteSheet(&sSpriteSheet_HoverBar);
    LoadCompressedSpriteSheet(&sSpriteSheet_ScrollThumb);
}

static void CreateListSprites(void)
{
    u32 cursorY = LIST_ROW_CENTER_Y(sPokedex->listCursor - sPokedex->listScroll);
    u32 thumbOffset = ScrollThumb_Offset(sPokedex->listCount, sPokedex->listCursor);

    sPokedex->cursorAnimId = CreateComfyAnim_Easing(&(struct ComfyAnimEasingConfig){
        .from = Q_24_8(cursorY),
        .to = Q_24_8(cursorY),
        .durationFrames = 1,
        .easingFunc = ComfyAnimEasing_EaseOutCubic,
    });
    sPokedex->scrollThumbAnimId = CreateComfyAnim_Easing(&(struct ComfyAnimEasingConfig){
        .from = Q_24_8(thumbOffset),
        .to = Q_24_8(thumbOffset),
        .durationFrames = 1,
        .easingFunc = ComfyAnimEasing_EaseOutCubic,
    });

    sPokedex->hoverBarSpriteId = CreateSprite(&sSpriteTemplate_HoverBar, HOVER_BAR_X, cursorY, SUBPRIORITY_HOVER_BAR);
    SetSubspriteTables(&gSprites[sPokedex->hoverBarSpriteId], sSubspriteTable_HoverBar);

    sPokedex->cursorSpriteId = CreateSprite(&sSpriteTemplate_Cursor, CURSOR_X, cursorY, SUBPRIORITY_CURSOR);
    StartCursorBob(sPokedex->cursorSpriteId, &sPokedex->cursorBobAnimId);

    for (u32 i = 0; i < SCROLL_THUMB_SEGMENTS; i++)
    {
        u8 spriteId = CreateSprite(&sSpriteTemplate_ScrollThumb, SCROLL_THUMB_X, 0, SUBPRIORITY_THUMB);

        sPokedex->scrollThumbSpriteIds[i] = spriteId;
        gSprites[spriteId].data[0] = i;
        gSprites[spriteId].invisible = TRUE;
    }
}

static void DestroyListSprites(void)
{
    DestroySprite(&gSprites[sPokedex->cursorSpriteId]);
    DestroySprite(&gSprites[sPokedex->hoverBarSpriteId]);
    for (u32 i = 0; i < SCROLL_THUMB_SEGMENTS; i++)
    {
        DestroySprite(&gSprites[sPokedex->scrollThumbSpriteIds[i]]);
    }
    FreeSpriteTilesByTag(TAG_DEX_CURSOR);
    FreeSpriteTilesByTag(TAG_DEX_HOVER_BAR);
    FreeSpriteTilesByTag(TAG_DEX_SCROLL_THUMB);
    FreeSpritePaletteByTag(TAG_SWSH_UI_PAL);
    FreeSpritePaletteByTag(TAG_DEX_SPRITE_PAL);
    ReleaseComfyAnim(sPokedex->cursorAnimId);
    ReleaseComfyAnim(sPokedex->cursorBobAnimId);
    ReleaseComfyAnim(sPokedex->scrollThumbAnimId);
}

static void SlideListSprites(void)
{
    struct ComfyAnim *cursorAnim = &gComfyAnims[sPokedex->cursorAnimId];
    struct ComfyAnim *thumbAnim = &gComfyAnims[sPokedex->scrollThumbAnimId];
    u32 durationFrames = SLIDE_FRAMES;

    if (!cursorAnim->completed)
    {
        durationFrames = (gMain.heldKeys & (DPAD_UP | DPAD_DOWN)) ? SLIDE_FRAMES_HELD : SLIDE_FRAMES_INTERRUPT;
    }

    InitComfyAnim_Easing(&(struct ComfyAnimEasingConfig){
        .from = cursorAnim->position,
        .to = Q_24_8(LIST_ROW_CENTER_Y(sPokedex->listCursor - sPokedex->listScroll)),
        .durationFrames = durationFrames,
        .easingFunc = ComfyAnimEasing_EaseOutCubic,
    }, cursorAnim);

    if (sPokedex->listCount > LIST_ROWS_SHOWN)
    {
        InitComfyAnim_Easing(&(struct ComfyAnimEasingConfig){
            .from = thumbAnim->position,
            .to = Q_24_8(ScrollThumb_Offset(sPokedex->listCount, sPokedex->listCursor)),
            .durationFrames = durationFrames,
            .easingFunc = ComfyAnimEasing_EaseOutCubic,
        }, thumbAnim);
    }
}

static void SeekListTo(u32 index)
{
    u32 top = 0;

    if (sPokedex->listCount > LIST_ROWS_SHOWN && index > LIST_ROWS_SHOWN / 2)
    {
        top = min(index - LIST_ROWS_SHOWN / 2, sPokedex->listCount - LIST_ROWS_SHOWN);
    }

    sPokedex->listScroll = top;
    sPokedex->listCursor = index;
}

static void MoveListCursor(s32 delta, bool32 allowWrap)
{
    u32 oldCursor = sPokedex->listCursor;
    u32 oldScroll = sPokedex->listScroll;
    u32 lastIndex = sPokedex->listCount - 1;
    s32 target = (s32)oldCursor + delta;
    u32 index;

    if (target < 0)
    {
        index = (allowWrap && oldCursor == 0) ? lastIndex : 0;
    }
    else if (target > (s32)lastIndex)
    {
        index = (allowWrap && oldCursor == lastIndex) ? 0 : lastIndex;
    }
    else
    {
        index = target;
    }

    if (index == oldCursor)
    {
        return;
    }

    SeekListTo(index);

    if (sPokedex->listScroll == oldScroll + 1)
    {
        ScrollWindow(WIN_LIST, LIST_SHIFT_UP, LIST_ROW_HEIGHT, PIXEL_FILL(0));
        PrintRow(LIST_ROWS_SHOWN - 1);
    }
    else if (sPokedex->listScroll + 1 == oldScroll)
    {
        ScrollWindow(WIN_LIST, LIST_SHIFT_DOWN, LIST_ROW_HEIGHT, PIXEL_FILL(0));
        PrintRow(0);
    }
    else if (sPokedex->listScroll != oldScroll)
    {
        PrintAll();
    }

    if (oldCursor >= sPokedex->listScroll && oldCursor < sPokedex->listScroll + LIST_ROWS_SHOWN)
    {
        PrintRow(oldCursor - sPokedex->listScroll);
    }
    PrintRow(sPokedex->listCursor - sPokedex->listScroll);
    CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
    SlideListSprites();
}

static void PrintTopBar(void)
{
    const u8 *sortStr = COMPOUND_STRING("By Number");
    bool32 national = (gSaveBlock2Ptr->pokedex.mode == DEX_MODE_NATIONAL);
    u32 caught = national ? GetNationalPokedexCount(FLAG_GET_CAUGHT) : GetHoennPokedexCount(FLAG_GET_CAUGHT);
    u32 seen = national ? GetNationalPokedexCount(FLAG_GET_SEEN) : GetHoennPokedexCount(FLAG_GET_SEEN);

    for (u32 i = WIN_LIST_SORT_TYPE; i < sPageWindows[DEX_PAGE_LIST].end; i++)
    {
        FillWindowPixelBuffer(i, PIXEL_FILL(0));
    }

    PrintDexText(WIN_LIST_SORT_TYPE, FONT_NARROW, GetStringCenterAlignXOffset(FONT_NARROW, sortStr, WIN_LIST_SORT_TYPE_WIDTH * TILE_WIDTH),
                 DEX_TEXT_Y, COLORID_LIST_SORT_TYPE, sortStr);

    BlitCaughtBall(WIN_LIST_CAUGHT_SEEN_NUM, BALL_CAUGHT_COLOR, CAUGHT_BALL_X, DEX_TEXT_Y);
    BlitCaughtBall(WIN_LIST_CAUGHT_SEEN_NUM, BALL_SEEN_HOVER, SEEN_BALL_X, DEX_TEXT_Y);
    PrintCount(WIN_LIST_CAUGHT_SEEN_NUM, CAUGHT_NUM_X, caught);
    PrintCount(WIN_LIST_CAUGHT_SEEN_NUM, SEEN_NUM_X, seen);

    PrintDexText(WIN_LIST_REGION, FONT_NARROW, 0, DEX_TEXT_Y, COLORID_LIST_REGION,
                 national ? COMPOUND_STRING("NATIONAL") : GetExpandedPlaceholder(PLACEHOLDER_ID_REGION));

    for (u32 i = WIN_LIST_SORT_TYPE; i < sPageWindows[DEX_PAGE_LIST].end; i++)
    {
        CopyWindowToVram(i, COPYWIN_GFX);
    }
}

static void PrintEntryPage(void)
{
    u32 natNum = sPokedex->listNatNum[sPokedex->listCursor];
    const u8 *name = GetSpeciesName(NationalPokedexNumToSpecies(natNum));

    for (u32 i = sPageWindows[DEX_PAGE_ENTRY].first; i < sPageWindows[DEX_PAGE_ENTRY].end; i++)
    {
        FillWindowPixelBuffer(i, PIXEL_FILL(0));
    }

    PrintDexNumber(WIN_ENTRY_HEADER, ENTRY_INDEX_X, ENTRY_TEXT_Y, COLORID_ENTRY_INDEX, natNum);
    PrintDexText(WIN_ENTRY_HEADER, GetFontIdToFit(name, FONT_NARROW, 0, ENTRY_NAME_FIT_WIDTH), ENTRY_NAME_X, ENTRY_TEXT_Y, COLORID_ENTRY_NAME, name);
    BlitCaughtBall(WIN_ENTRY_HEADER, GetSetPokedexFlag(natNum, FLAG_GET_CAUGHT) ? BALL_CAUGHT_HOVER : BALL_SEEN_HOVER, ENTRY_BALL_X, ENTRY_BALL_Y);
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

static void UpdateBg1Fade(bool32 show)
{
    if (show && sPokedex->bg1Alpha < BLEND_MAX)
    {
        if (sPokedex->bg1Alpha == 0)
        {
            ShowBg(1);
        }
        sPokedex->bg1Alpha += min(LIST_FADE_STEP, BLEND_MAX - sPokedex->bg1Alpha);
    }
    else if (!show)
    {
        sPokedex->bg1Alpha -= min(LIST_FADE_STEP, sPokedex->bg1Alpha);
        if (sPokedex->bg1Alpha == 0)
        {
            HideBg(1);
        }
    }

    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG1 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3 | BLDCNT_TGT2_OBJ | BLDCNT_TGT2_BD);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(sPokedex->bg1Alpha, BLEND_MAX - sPokedex->bg1Alpha));
}

static void UpdateSpriteFade(bool32 show)
{
    if (show)
    {
        sPokedex->spriteAlpha += min(LIST_FADE_STEP, BLEND_MAX - sPokedex->spriteAlpha);
    }
    else
    {
        sPokedex->spriteAlpha -= min(LIST_FADE_STEP, sPokedex->spriteAlpha);
    }

    gSprites[sPokedex->cursorSpriteId].invisible = (sPokedex->spriteAlpha == 0);
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_OBJ | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3 | BLDCNT_TGT2_BD);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(sPokedex->spriteAlpha, BLEND_MAX - sPokedex->spriteAlpha));
}

static void SetBg1PageWindows(enum DexPage page)
{
    for (u32 i = sPageWindows[sPokedex->bg1Page].first; i < sPageWindows[sPokedex->bg1Page].end; i++)
    {
        ClearWindowTilemap(i);
    }

    if (page == DEX_PAGE_ENTRY)
    {
        PrintEntryPage();
    }

    for (u32 i = sPageWindows[page].first; i < sPageWindows[page].end; i++)
    {
        PutWindowTilemap(i);
        CopyWindowToVram(i, COPYWIN_GFX);
    }

    CopyBgTilemapBufferToVram(1);
    sPokedex->bg1Page = page;
}

static void UpdatePageTransition(void)
{
    enum DexPage page = sPokedex->page;
    bool32 wantHabitat = (page == DEX_PAGE_HABITAT);

    if (sPokedex->bg1Alpha > 0 && sPokedex->bg1Page != page)
    {
        UpdateBg1Fade(FALSE);
    }
    else if (page != DEX_PAGE_LIST && sPokedex->spriteAlpha > 0)
    {
        UpdateSpriteFade(FALSE);
    }
    else if (page != DEX_PAGE_HABITAT && sPokedex->bg2Page == page && sPokedex->bg2Alpha == BLEND_MAX)
    {
        if (page == DEX_PAGE_LIST && sPokedex->spriteAlpha < BLEND_MAX)
        {
            UpdateSpriteFade(TRUE);
        }
        else if (sPokedex->bg1Page != page)
        {
            SetBg1PageWindows(page);
        }
        else
        {
            UpdateBg1Fade(TRUE);
        }
    }
    else if (sPokedex->bg3Habitat != wantHabitat || sPokedex->bg3White > 0)
    {
        if (sPokedex->bg2Page != DEX_PAGE_HABITAT || sPokedex->bg2Alpha < BLEND_MAX)
        {
            UpdateBg2Fade(DEX_PAGE_HABITAT);
        }
        else
        {
            UpdateBg3Whiteout(wantHabitat);
        }
    }
    else
    {
        UpdateBg2Fade(page);
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

static void Task_ExitSwShPokedex(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        DestroyListSprites();
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

    if (gPaletteFade.active)
        return;

    switch (sPokedex->page)
    {
    case DEX_PAGE_LIST:
        if (JOY_NEW(A_BUTTON))
        {
            if (IsListIndexSeen(sPokedex->listCursor))
            {
                sPokedex->page = DEX_PAGE_ENTRY;
            }
        }
        else if (JOY_NEW(B_BUTTON))
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            gTasks[taskId].func = Task_ExitSwShPokedex;
        }
        else if (JOY_REPEAT(DPAD_UP))
        {
            MoveListCursor(-1, JOY_NEW(DPAD_UP));
        }
        else if (JOY_REPEAT(DPAD_DOWN))
        {
            MoveListCursor(1, JOY_NEW(DPAD_DOWN));
        }
        else if (JOY_REPEAT(DPAD_LEFT))
        {
            MoveListCursor(-LIST_ROWS_SHOWN, FALSE);
        }
        else if (JOY_REPEAT(DPAD_RIGHT))
        {
            MoveListCursor(LIST_ROWS_SHOWN, FALSE);
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
        sPokedex->cursorAnimId = INVALID_COMFY_ANIM;
        sPokedex->cursorBobAnimId = INVALID_COMFY_ANIM;
        sPokedex->scrollThumbAnimId = INVALID_COMFY_ANIM;
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
            sPokedex->bg1Alpha = BLEND_MAX;
            sPokedex->spriteAlpha = BLEND_MAX;
            BuildDexList();
            PrintAll();
            CopyWindowToVram(WIN_LIST, COPYWIN_GFX);
            PrintTopBar();
            for (u32 i = sPageWindows[DEX_PAGE_LIST].first; i < sPageWindows[DEX_PAGE_LIST].end; i++)
            {
                PutWindowTilemap(i);
            }
            ScheduleBgCopyTilemapToVram(1);
            gMain.state++;
        }
        break;
    case 4:
        LoadListSpriteGfx();
        CreateListSprites();
        gMain.state++;
        break;
    case 5:
        LoadPalette(sSwShPokedex_Pal, BG_PLTT_ID(0), DEX_PAL_BANK_COUNT * PLTT_SIZE_4BPP);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG1 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3 | BLDCNT_TGT2_OBJ | BLDCNT_TGT2_BD);
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
