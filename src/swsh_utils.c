#include "global.h"
#include "string_util.h"
#include "swsh_utils.h"
#include "text.h"
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
