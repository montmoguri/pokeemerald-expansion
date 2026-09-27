#ifndef GUARD_SWSH_UTILS_H
#define GUARD_SWSH_UTILS_H

#include "sprite.h"

#define MAX_DESCRIPTION_LINES   2

// see graphics/interface/swsh/status_icons.png
enum StatusIcon
{
    STATUS_ICON_PSN,
    STATUS_ICON_PRZ,
    STATUS_ICON_SLP,
    STATUS_ICON_FRZ,
    STATUS_ICON_BRN,
    STATUS_ICON_PKRS,
    STATUS_ICON_FNT,
    STATUS_ICON_FRB,
    STATUS_ICON_TOX,
    STATUS_ICON_COUNT,
    STATUS_ICON_NONE = STATUS_ICON_COUNT,
};

// see graphics/types/swsh/move_types.png
#define MOVE_TYPE_ICON_SIZE         (32 * 16 / 2)
#define MOVE_TYPE_ICONS_GFX_SIZE    ((NUMBER_OF_MON_TYPES + CONTEST_CATEGORIES_COUNT) * MOVE_TYPE_ICON_SIZE)

// shared sprite tags
#define TAG_SWSH_UI_PAL                 55300   // status_icons.png
#define TAG_SWSH_QUANTITY_FRAME         55301
#define TAG_SWSH_SPINNER_ARROW          55302
#define TAG_SWSH_STATUS_ICONS           55303

#define QUANTITY_FRAME_SPRITES_COUNT    2

enum
{
    SPINNER_ARROW_UP,
    SPINNER_ARROW_DOWN,
    SPINNER_ARROW_SPRITES_COUNT,
};

enum
{
    SPINNER_ARROW_STATIC,
    SPINNER_ARROW_ANIM,
    SPINNER_ARROW_LOOP,
};

#define SPINNER_ARROW_Y_OFFSET      10

extern const struct SpritePalette gSpritePalette_SwShUI;
extern const struct OamData gOamData_StatusIconsSwSh;
extern const union AnimCmd *const gSpriteAnimTable_StatusIconsSwSh[];
extern const struct CompressedSpriteSheet gSpriteSheet_StatusIconsSwSh;
extern const struct SpriteTemplate gSpriteTemplate_StatusIconsSwSh;
extern const struct CompressedSpriteSheet gSpriteSheet_QuantityFrameSwSh;
extern const struct CompressedSpriteSheet gSpriteSheet_SpinnerArrowSwSh;

u8 FormatDescriptionByWidth(u8 *result, s32 resultSize, s32 maxWidth, u8 fontId, const u8 *str, s16 letterSpacing);
void ConvertMoneyToCommaString(u8 *dest, u32 amount);
enum StatusIcon GetStatusIconFromStatus(u32 status);
u8 *LoadMoveTypeIconCache(void);
void CopyMoveTypeIconTiles(const u8 *cache, u32 iconId, void *dest);
void CreateSpinnerArrowSprites(u8 *spriteIds, s16 x, s16 y, u8 mode, u8 priority, u8 subpriority);
void DestroySpinnerArrowSprites(u8 *spriteIds);
void AnimateQuantitySpinner(const u8 *spriteIds);
void CreateQuantityFrameSprites(u8 *frameSpriteIds, u8 *spinnerSpriteIds, s16 x, s16 y, u8 priority, u8 subpriority);
void DestroyQuantityFrameSprites(u8 *frameSpriteIds, u8 *spinnerSpriteIds);
void PrintQuantityFrameCount(u8 spriteId, u16 quantity);
void PrintQuantityFrameTotal(u8 spriteId, u32 total);
void StartCursorBob(u8 spriteId, u32 *animId);
void UpdateCursorBob(struct Sprite *sprite, u32 animId);

#endif // GUARD_SWSH_UTILS_H
