#include "global.h"
#include "comfy_anim.h"
#include "decompress.h"
#include "dma3.h"
#include "international_string_util.h"
#include "main.h"
#include "malloc.h"
#include "money.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "swsh_graphics.h"
#include "swsh_utils.h"
#include "text.h"
#include "constants/battle.h"
#include "constants/characters.h"

/*
Mont note:
- The functions here remove hyphens from specific words when they are split across lines.
- For example, Incineum Z has "Incine-\n" and "roar", which the previous formatter would have rendered: "Incine-roar"
- Is this 100% overkill for just a few words? Yes.
- Is it worth it to read item descriptions and not see random jank? My sanity says yes.
*/
// Lookup table for hyphen removal - stores both parts of hyphenated words
static const struct {
    const u8 *before;
    const u8 *after;
} sHyphenRemovalPatterns[] = {
    {COMPOUND_STRING("Incine"),  COMPOUND_STRING("roar")},
    {COMPOUND_STRING("La"),      COMPOUND_STRING("riat")},
    {COMPOUND_STRING("Marsha"),  COMPOUND_STRING("dow")},
    {COMPOUND_STRING("Thi"),     COMPOUND_STRING("ef")},
    {COMPOUND_STRING("Elec"),    COMPOUND_STRING("tric")},
    {COMPOUND_STRING("Fight"),   COMPOUND_STRING("ing")},
    {COMPOUND_STRING("pro"),     COMPOUND_STRING("motes")},
    {COMPOUND_STRING("Decidu"),  COMPOUND_STRING("eye")},
    {COMPOUND_STRING("Sha"),     COMPOUND_STRING("ckle")},
    {COMPOUND_STRING("invigor"), COMPOUND_STRING("ating")},
    {COMPOUND_STRING("Thunder"), COMPOUND_STRING("bolt")},
    {COMPOUND_STRING("inde"),    COMPOUND_STRING("scribable")},
    {COMPOUND_STRING("Poké"),    COMPOUND_STRING("mon")},
};

static bool32 ShouldRemoveHyphen(const u8 *p, const u8 *start)
{
    for (u32 i = 0; i < ARRAY_COUNT(sHyphenRemovalPatterns); i++)
    {
        const u8 *before = sHyphenRemovalPatterns[i].before;
        const u8 *after = sHyphenRemovalPatterns[i].after;
        u32 beforeLen = StringLength(before);

        if (p < start + beforeLen)
            continue;

        if (StringCompareN(p - beforeLen, before, beforeLen) == 0
         && StringCompareN(p + 1, after, StringLength(after)) == 0)
            return TRUE;
    }

    return FALSE;
}

static bool32 PerformTextFormatting(u8 *result, s32 resultSize, s32 maxWidth, u8 fontId, const u8 *str, s16 letterSpacing, u32 *outLineCount)
{
    u8 *end = result;
    u8 *limit = result + resultSize - 1;

    while (*str != EOS && end < limit)
    {
        if (*str == CHAR_SPACE || *str == CHAR_NEWLINE)
        {
            if (!(*str == CHAR_NEWLINE && end > result && *(end - 1) == CHAR_HYPHEN))
                *end++ = EOS;
        }
        else
        {
            *end++ = *str;
        }

        str++;
    }
    *end = EOS;

    u8 *p = result;
    while (p < end)
    {
        if (*p == CHAR_HYPHEN && ShouldRemoveHyphen(p, result))
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

    u8 *ptr = result;
    u8 *curLine = ptr;

    *outLineCount = 1;
    while (*ptr != EOS)
        ptr++;

    while (ptr != end)
    {
        u8 *lastSpace = ptr++;

        *lastSpace = CHAR_SPACE;
        if (GetStringWidth(fontId, curLine, letterSpacing) > maxWidth)
        {
            *lastSpace = CHAR_NEWLINE;
            (*outLineCount)++;
            curLine = ptr;
        }

        while (*ptr != EOS)
            ptr++;
    }

    return (GetStringWidth(fontId, curLine, letterSpacing) <= maxWidth);
}

u8 FormatDescriptionByWidth(u8 *result, s32 resultSize, s32 maxWidth, u8 fontId, const u8 *str, s16 letterSpacing)
{
    while (TRUE)
    {
        u32 lineCount;
        bool32 lastLineFits = PerformTextFormatting(result, resultSize, maxWidth, fontId, str, letterSpacing, &lineCount);

        if (lineCount <= MAX_DESCRIPTION_LINES && lastLineFits)
            break;

        if (fontId != FONT_SHORT_NARROW)
            break;

        fontId = FONT_SHORT_NARROWER;
        letterSpacing = GetFontAttribute(fontId, FONTATTR_LETTER_SPACING);
    }

    return fontId;
}

// 123456 -> "123,456".
void ConvertMoneyToCommaString(u8 *dest, u32 amount)
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

enum StatusIcon GetStatusIconFromStatus(u32 status)
{
    if (status & STATUS1_TOXIC_POISON)
        return STATUS_ICON_TOX;
    if (status & STATUS1_PSN_ANY)
        return STATUS_ICON_PSN;
    if (status & STATUS1_SLEEP)
        return STATUS_ICON_SLP;
    if (status & STATUS1_PARALYSIS)
        return STATUS_ICON_PRZ;
    if (status & STATUS1_FREEZE)
        return STATUS_ICON_FRZ;
    if (status & STATUS1_BURN)
        return STATUS_ICON_BRN;
    if (status & STATUS1_FROSTBITE)
        return STATUS_ICON_FRB;
    return STATUS_ICON_NONE;
}

u8 *LoadMoveTypeIconCache(void)
{
    u8 *cache = Alloc(MOVE_TYPE_ICONS_GFX_SIZE);

    DecompressDataWithHeaderWram(gMoveTypesSwSh_Gfx, cache);
    return cache;
}

void CopyMoveTypeIconTiles(const u8 *cache, u32 iconId, void *dest)
{
    RequestDma3Copy(&cache[iconId * MOVE_TYPE_ICON_SIZE], dest, MOVE_TYPE_ICON_SIZE, 0x10);
}

#define SPINNER_ARROW_ANIM_FRAMES   3
#define SPINNER_ARROW_LOOP_FRAMES   8

#define QUANTITY_FRAME_TILES        ((64 * 32) / (8 * 8))
#define QUANTITY_FRAME_SPACING      64
#define QUANTITY_SPINNER_X_OFFSET   8

#define QUANTITY_FILL_INDEX         13
#define QUANTITY_COUNT_LEFT         16
#define QUANTITY_COUNT_RIGHT        48
#define QUANTITY_COUNT_TOP          8
#define QUANTITY_TOTAL_RIGHT        48

static void SpriteCB_SpinnerArrow(struct Sprite *sprite);

static const union TextColor sQuantityTextColor =
{
    .background = 0,
    .foreground = 12,
    .shadow = 14,
};

const struct SpritePalette gSpritePalette_SwShUI =
{
    .data = gStatusIconsSwSh_Pal,
    .tag = TAG_SWSH_UI_PAL,
};

const struct OamData gOamData_StatusIconsSwSh =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x8),
    .size = SPRITE_SIZE(32x8),
    .priority = 1,
};

static const union AnimCmd sSpriteAnim_StatusPoison[]    = { ANIMCMD_FRAME(0,  0), ANIMCMD_END };
static const union AnimCmd sSpriteAnim_StatusParalyzed[] = { ANIMCMD_FRAME(4,  0), ANIMCMD_END };
static const union AnimCmd sSpriteAnim_StatusSleep[]     = { ANIMCMD_FRAME(8,  0), ANIMCMD_END };
static const union AnimCmd sSpriteAnim_StatusFrozen[]    = { ANIMCMD_FRAME(12, 0), ANIMCMD_END };
static const union AnimCmd sSpriteAnim_StatusBurn[]      = { ANIMCMD_FRAME(16, 0), ANIMCMD_END };
static const union AnimCmd sSpriteAnim_StatusPokerus[]   = { ANIMCMD_FRAME(20, 0), ANIMCMD_END };
static const union AnimCmd sSpriteAnim_StatusFaint[]     = { ANIMCMD_FRAME(24, 0), ANIMCMD_END };
static const union AnimCmd sSpriteAnim_StatusFrostbite[] = { ANIMCMD_FRAME(28, 0), ANIMCMD_END };
static const union AnimCmd sSpriteAnim_StatusToxic[]     = { ANIMCMD_FRAME(32, 0), ANIMCMD_END };

const union AnimCmd *const gSpriteAnimTable_StatusIconsSwSh[STATUS_ICON_COUNT] =
{
    [STATUS_ICON_PSN]  = sSpriteAnim_StatusPoison,
    [STATUS_ICON_PRZ]  = sSpriteAnim_StatusParalyzed,
    [STATUS_ICON_SLP]  = sSpriteAnim_StatusSleep,
    [STATUS_ICON_FRZ]  = sSpriteAnim_StatusFrozen,
    [STATUS_ICON_BRN]  = sSpriteAnim_StatusBurn,
    [STATUS_ICON_PKRS] = sSpriteAnim_StatusPokerus,
    [STATUS_ICON_FNT]  = sSpriteAnim_StatusFaint,
    [STATUS_ICON_FRB]  = sSpriteAnim_StatusFrostbite,
    [STATUS_ICON_TOX]  = sSpriteAnim_StatusToxic,
};

const struct CompressedSpriteSheet gSpriteSheet_StatusIconsSwSh =
{
    .data = gStatusIconsSwSh_Gfx,
    .size = STATUS_ICON_COUNT * 4 * TILE_SIZE_4BPP,
    .tag = TAG_SWSH_STATUS_ICONS,
};

const struct SpriteTemplate gSpriteTemplate_StatusIconsSwSh =
{
    .tileTag = TAG_SWSH_STATUS_ICONS,
    .paletteTag = TAG_SWSH_UI_PAL,
    .oam = &gOamData_StatusIconsSwSh,
    .anims = gSpriteAnimTable_StatusIconsSwSh,
    .callback = SpriteCallbackDummy,
};

static const struct OamData sOamData_QuantityFrame =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x32),
    .size = SPRITE_SIZE(64x32),
    .priority = 1,
};

static const union AnimCmd sSpriteAnim_QuantityFrame_0[] =
{
    ANIMCMD_FRAME(0 * QUANTITY_FRAME_TILES, 0, FALSE, FALSE),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_QuantityFrame_1[] =
{
    ANIMCMD_FRAME(1 * QUANTITY_FRAME_TILES, 0, FALSE, FALSE),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_QuantityFrame[QUANTITY_FRAME_SPRITES_COUNT] =
{
    sSpriteAnim_QuantityFrame_0,
    sSpriteAnim_QuantityFrame_1,
};

const struct CompressedSpriteSheet gSpriteSheet_QuantityFrameSwSh =
{
    .data = gQuantityFrameSwSh_Gfx,
    .size = (64 * 64) / 2,
    .tag = TAG_SWSH_QUANTITY_FRAME,
};

static const struct SpriteTemplate sSpriteTemplate_QuantityFrame =
{
    .tileTag = TAG_SWSH_QUANTITY_FRAME,
    .paletteTag = TAG_SWSH_UI_PAL,
    .oam = &sOamData_QuantityFrame,
    .anims = sSpriteAnimTable_QuantityFrame,
    .callback = SpriteCallbackDummy,
};

static const struct OamData sOamData_SpinnerArrow =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x8),
    .size = SPRITE_SIZE(16x8),
    .priority = 1,
};

static const union AnimCmd sSpriteAnim_SpinnerArrowUp[] =
{
    ANIMCMD_FRAME(0, 0, FALSE, FALSE),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_SpinnerArrowDown[] =
{
    ANIMCMD_FRAME(0, 0, FALSE, TRUE),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_SpinnerArrow[SPINNER_ARROW_SPRITES_COUNT] =
{
    [SPINNER_ARROW_UP]   = sSpriteAnim_SpinnerArrowUp,
    [SPINNER_ARROW_DOWN] = sSpriteAnim_SpinnerArrowDown,
};

const struct CompressedSpriteSheet gSpriteSheet_SpinnerArrowSwSh =
{
    .data = gSpinnerArrowSwSh_Gfx,
    .size = (16 * 8) / 2,
    .tag = TAG_SWSH_SPINNER_ARROW,
};

static const struct SpriteTemplate sSpriteTemplate_SpinnerArrow =
{
    .tileTag = TAG_SWSH_SPINNER_ARROW,
    .paletteTag = TAG_SWSH_UI_PAL,
    .oam = &sOamData_SpinnerArrow,
    .anims = sSpriteAnimTable_SpinnerArrow,
    .callback = SpriteCB_SpinnerArrow,
};

#define sDir    data[0]  // -1 = up, +1 = down
#define sMode   data[1]
#define sTimer  data[2]
#define sStep   data[3]

static const u8 sSpinnerArrowOffsets[] = {0, 1, 2, 1};

static void SpriteCB_SpinnerArrow(struct Sprite *sprite)
{
    u8 stepFrames;

    if (sprite->sMode == SPINNER_ARROW_STATIC)
        return;

    stepFrames = (sprite->sMode == SPINNER_ARROW_LOOP) ? SPINNER_ARROW_LOOP_FRAMES : SPINNER_ARROW_ANIM_FRAMES;
    if (++sprite->sTimer < stepFrames)
        return;

    sprite->sTimer = 0;
    if (++sprite->sStep >= (s16)ARRAY_COUNT(sSpinnerArrowOffsets))
    {
        sprite->sStep = 0;
        if (sprite->sMode == SPINNER_ARROW_ANIM)
            sprite->sMode = SPINNER_ARROW_STATIC;
    }
    sprite->y2 = sprite->sDir * sSpinnerArrowOffsets[sprite->sStep];
}

void CreateSpinnerArrowSprites(u8 *spriteIds, s16 x, s16 y, u8 mode, u8 priority, u8 subpriority)
{
    u32 i;

    for (i = 0; i < SPINNER_ARROW_SPRITES_COUNT; i++)
    {
        s8 dir = (i == SPINNER_ARROW_UP) ? -1 : 1;
        u8 spriteId = CreateSprite(&sSpriteTemplate_SpinnerArrow, x, y + dir * SPINNER_ARROW_Y_OFFSET, subpriority);

        if (spriteId == MAX_SPRITES)
            continue;

        StartSpriteAnim(&gSprites[spriteId], i);
        gSprites[spriteId].oam.priority = priority;
        gSprites[spriteId].sDir = dir;
        gSprites[spriteId].sMode = mode;
        spriteIds[i] = spriteId;
    }
}

void DestroySpinnerArrowSprites(u8 *spriteIds)
{
    u32 i;

    for (i = 0; i < SPINNER_ARROW_SPRITES_COUNT; i++)
    {
        if (spriteIds[i] != SPRITE_NONE)
        {
            DestroySprite(&gSprites[spriteIds[i]]);
            spriteIds[i] = SPRITE_NONE;
        }
    }
}

void AnimateQuantitySpinner(const u8 *spriteIds)
{
    u16 dpad = JOY_REPEAT(DPAD_ANY);
    u8 arrowIdx, spriteId;

    if (dpad == DPAD_UP || dpad == DPAD_RIGHT)
        arrowIdx = SPINNER_ARROW_UP;
    else if (dpad == DPAD_DOWN || dpad == DPAD_LEFT)
        arrowIdx = SPINNER_ARROW_DOWN;
    else
        return;

    spriteId = spriteIds[arrowIdx];
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

void CreateQuantityFrameSprites(u8 *frameSpriteIds, u8 *spinnerSpriteIds, s16 x, s16 y, u8 priority, u8 subpriority)
{
    u32 i;

    if (frameSpriteIds[0] != SPRITE_NONE)
        return;

    for (i = 0; i < QUANTITY_FRAME_SPRITES_COUNT; i++)
    {
        u8 spriteId = CreateSprite(&sSpriteTemplate_QuantityFrame, x + i * QUANTITY_FRAME_SPACING, y, subpriority + 1);

        if (spriteId == MAX_SPRITES)
            continue;

        StartSpriteAnim(&gSprites[spriteId], i);
        SetSpriteSheetFrameTileNum(&gSprites[spriteId]);
        gSprites[spriteId].oam.priority = priority;
        frameSpriteIds[i] = spriteId;
    }

    if (frameSpriteIds[1] != SPRITE_NONE)
        FillSpriteRectColor(frameSpriteIds[1], 0, QUANTITY_COUNT_TOP, QUANTITY_TOTAL_RIGHT,
                            GetFontAttribute(FONT_NARROW, FONTATTR_MAX_LETTER_HEIGHT), QUANTITY_FILL_INDEX);
    CreateSpinnerArrowSprites(spinnerSpriteIds, x + QUANTITY_SPINNER_X_OFFSET, y, SPINNER_ARROW_STATIC, priority, subpriority);
}

void DestroyQuantityFrameSprites(u8 *frameSpriteIds, u8 *spinnerSpriteIds)
{
    u32 i;

    for (i = 0; i < QUANTITY_FRAME_SPRITES_COUNT; i++)
    {
        if (frameSpriteIds[i] != SPRITE_NONE)
        {
            DestroySprite(&gSprites[frameSpriteIds[i]]);
            frameSpriteIds[i] = SPRITE_NONE;
        }
    }
    DestroySpinnerArrowSprites(spinnerSpriteIds);
}

void PrintQuantityFrameCount(u8 spriteId, u16 quantity)
{
    u32 slotWidth, countWidth;

    if (spriteId == SPRITE_NONE)
        return;

    FillSpriteRectColor(spriteId, QUANTITY_COUNT_LEFT, QUANTITY_COUNT_TOP, QUANTITY_COUNT_RIGHT - QUANTITY_COUNT_LEFT,
                        GetFontAttribute(FONT_NARROW, FONTATTR_MAX_LETTER_HEIGHT), QUANTITY_FILL_INDEX);

    // × holds the position of a full-width count; the count is centered in the digit slot after it
    ConvertIntToDecimalStringN(gStringVar1, 0, STR_CONV_MODE_LEADING_ZEROS, MAX_ITEM_DIGITS);
    slotWidth = GetStringWidth(FONT_NARROW, gStringVar1, 0);
    StringExpandPlaceholders(gStringVar4, gText_xVar1);
    AddSpriteTextPrinterParameterized6(spriteId, FONT_NARROW,
                                       GetStringRightAlignXOffset(FONT_NARROW, gStringVar4, QUANTITY_COUNT_RIGHT),
                                       QUANTITY_COUNT_TOP, 0, 0, sQuantityTextColor, 0, COMPOUND_STRING("×"));

    ConvertIntToDecimalStringN(gStringVar1, quantity, STR_CONV_MODE_LEFT_ALIGN, MAX_ITEM_DIGITS);
    countWidth = GetStringWidth(FONT_NARROW, gStringVar1, 0);
    AddSpriteTextPrinterParameterized6(spriteId, FONT_NARROW,
                                       QUANTITY_COUNT_RIGHT - slotWidth + (slotWidth - countWidth) / 2,
                                       QUANTITY_COUNT_TOP, 0, 0, sQuantityTextColor, 0, gStringVar1);
}

void PrintQuantityFrameTotal(u8 spriteId, const u8 *str)
{
    if (spriteId == SPRITE_NONE)
        return;

    FillSpriteRectColor(spriteId, 0, QUANTITY_COUNT_TOP, QUANTITY_TOTAL_RIGHT,
                        GetFontAttribute(FONT_NARROW, FONTATTR_MAX_LETTER_HEIGHT), QUANTITY_FILL_INDEX);
    AddSpriteTextPrinterParameterized6(spriteId, FONT_NARROW,
                                       GetStringRightAlignXOffset(FONT_NARROW, str, QUANTITY_TOTAL_RIGHT),
                                       QUANTITY_COUNT_TOP, 0, 0, sQuantityTextColor, 0, str);
}

#define CURSOR_BOB_RANGE    3
#define CURSOR_BOB_FRAMES   20
#define sBobTarget          data[0]

void StartCursorBob(u8 spriteId, u32 *animId)
{
    struct ComfyAnimEasingConfig config =
    {
        .from = Q_24_8(0),
        .to = Q_24_8(CURSOR_BOB_RANGE),
        .durationFrames = CURSOR_BOB_FRAMES,
        .easingFunc = ComfyAnimEasing_EaseInOutQuad,
    };

    if (*animId == INVALID_COMFY_ANIM)
        *animId = CreateComfyAnim_Easing(&config);
    else
        InitComfyAnim_Easing(&config, &gComfyAnims[*animId]);

    gSprites[spriteId].x2 = 0;
    gSprites[spriteId].sBobTarget = CURSOR_BOB_RANGE;
}

void UpdateCursorBob(struct Sprite *sprite, u32 animId)
{
    struct ComfyAnim *bob;

    if (animId == INVALID_COMFY_ANIM)
        return;

    bob = &gComfyAnims[animId];
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

#undef CURSOR_BOB_RANGE
#undef CURSOR_BOB_FRAMES
#undef sBobTarget
