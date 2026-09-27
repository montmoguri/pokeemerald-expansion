#ifndef GUARD_SWSH_UTILS_H
#define GUARD_SWSH_UTILS_H

#define MAX_DESCRIPTION_LINES   2

u8 FormatDescriptionByWidth(u8 *result, s32 resultSize, s32 maxWidth, u8 fontId, const u8 *str, s16 letterSpacing);

#endif // GUARD_SWSH_UTILS_H
