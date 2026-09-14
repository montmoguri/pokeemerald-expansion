#ifndef GUARD_SWSH_SUMMARY_SCREEN_H
#define GUARD_SWSH_SUMMARY_SCREEN_H

#include "main.h"
#include "constants/rgb.h"

// turn on and off the SwSh summary screen
#define SWSH_SUMMARY_SCREEN                     TRUE

// configs
#define SWSH_SUMMARY_AUTO_FORMAT_MOVE_DESC      (SWSH_SUMMARY_SCREEN && TRUE)   // automatically formats move descriptions to fit the new box size. disable if you want to format them manually
#define SWSH_SUMMARY_NATURE_COLORS              (SWSH_SUMMARY_SCREEN && TRUE)   // color stats increased or reduced by nature, red = boosted, blue = reduced
#define SWSH_SUMMARY_CATEGORY_ICONS             (SWSH_SUMMARY_SCREEN && TRUE)   // determines whether category (split) icons are shown or not
#define SWSH_SUMMARY_SHOW_IV_EV                 (SWSH_SUMMARY_SCREEN && TRUE)   // determines how to show IVs and EVs
#define SWSH_SUMMARY_SHOW_FRIENDSHIP            (SWSH_SUMMARY_SCREEN && TRUE)   // show a heart that fills up to indicate friendship value
#define SWSH_SUMMARY_SWSH_TYPE_ICONS            (SWSH_SUMMARY_SCREEN && TRUE)   // use Gen 8 style type icons instead of the default ones
#define SWSH_SUMMARY_SWSH_TYPE_ICONS_SV_PAL     (SWSH_SUMMARY_SCREEN && FALSE)  // use Scarlet/Violet palette for type icons
                                                                                // out of the box the vanilla icons don't fit well, this is mostly a compatibility
#define SWSH_SUMMARY_SCROLLING_BG               (SWSH_SUMMARY_SCREEN && TRUE)   // enables scrolling animated background
#define SWSH_SUMMARY_MON_IDLE_ANIMS             (SWSH_SUMMARY_SCREEN && TRUE)   // loops the mon animations regularly as an "idle" anim
#define SWSH_SUMMARY_MON_SHADOWS                (SWSH_SUMMARY_SCREEN && TRUE)   // displays a shadow for the mon sprite
                                                                                // if SWSH_SUMMARY_STATUS_ICON_FADE is active the shadow is drawn flat with SWSH_SUMMARY_MON_SHADOW_COLOR
#define SWSH_SUMMARY_BG_BLEND                   (SWSH_SUMMARY_SCREEN && FALSE)  // enables alpha blending for the main UI (semi-transparency)
                                                                                // superseded by SWSH_SUMMARY_STATUS_ICON_FADE
#define SWSH_SUMMARY_STATUS_ICON_FADE           (SWSH_SUMMARY_SCREEN && TRUE)   // fades the status condition icon in and out so the level underneath shows through
                                                                                // takes alpha blending over from SWSH_SUMMARY_MON_SHADOWS and SWSH_SUMMARY_BG_BLEND
#define SWSH_SUMMARY_SHOW_CONTEST_PAGES         (SWSH_SUMMARY_SCREEN && TRUE)   // enables conditions and contest moves pages
#define SWSH_SUMMARY_SHOW_DYNAMAX_LEVEL         (SWSH_SUMMARY_SCREEN && FALSE)  // show dynamax level
#define SWSH_SUMMARY_SHOW_GIGANTAMAX            (SWSH_SUMMARY_SCREEN && FALSE)  // show gigantamax icon
#define SWSH_SUMMARY_SHOW_TERA_TYPE             (SWSH_SUMMARY_SCREEN && FALSE)  // show tera type icons

// constants
#define SWSH_MAX_MOVE_DESCRIPTION_LENGTH        100                             // (in pixels) modify if SWSH_SUMMARY_AUTO_FORMAT_MOVE_DESC is true
#define SWSH_SUMMARY_MON_IDLE_ANIMS_FRAMES      300                             // number of frames between each idle anim IF SWSH_SUMMARY_MON_IDLE_ANIMS is true.
                                                                                // for reference, Emerald runs at 60FPS by default
#define SWSH_SUMMARY_MON_SHADOW_COLOR           RGB(16, 16, 16)                 // flat shadow color, used when SWSH_SUMMARY_STATUS_ICON_FADE is true

void ShowPokemonSummaryScreen_SwSh(u8 mode, void *mons, u8 monIndex, u8 maxMonIndex, void (*callback)(void));
void ShowSelectMovePokemonSummaryScreen_SwSh(struct Pokemon *mons, u8 monIndex, void (*callback)(void), u16 newMove);
u8 GetMoveSlotToReplace_SwSh(void);
void SummaryScreen_SetAnimDelayTaskId_SwSh(u8 taskId);

// Looking for configs for renaming mons and relearning moves? Those use the standard expansion configs
// P_SUMMARY_SCREEN_RENAME and P_SUMMARY_SCREEN_MOVE_RELEARNER in include/config/pokemon.h
// Same with showing dynamic types:
// P_SHOW_DYNAMIC_TYPES

/* Info for users

General tilemap setup
BG3 - scrolling grid background (or not scrolling if you turned the config off)
BG2 - main UI overlayed on scrolling BG
BG1 - pop in move effect windows
BG0 - text windows

Mosaic effect used when transitioning between screens and actvating
effect windows is controlled by tMosaicStrength in the relevant
task functions.

BG scrolling speed can be modified by altering the value parameter
of the ChangeBgX and ChangeBgY functions in VBlank()

Main UI and shadow transparency levels can be adjusted by changing the
value written to the alpha blend register in swsh_summary_screen.c:

static void SetSummaryBlendRegs(void)
...
SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(14, 6));
...
}

*/

/* ravetodo in future updates
- extended move desc window
- ribbons
*/

#endif // GUARD_SWSH_SUMMARY_SCREEN_H
