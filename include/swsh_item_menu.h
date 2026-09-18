#ifndef GUARD_SWSH_BAG_MENU_H
#define GUARD_SWSH_BAG_MENU_H

#define SWSH_BAG_MENU               TRUE                                        // Use SwSh bag menu

#define SWSH_BAG_CONTEST_INFO       (SWSH_BAG_MENU && TRUE)                     // Show contest info for TMs/HMs in the item menu
#define SWSH_BAG_BERRY_STAT         (SWSH_BAG_MENU && TRUE)                     // Show berry stat (flavors, size, etc.) in the item menu
#define SWSH_BAG_BERRY_TAG          (SWSH_BAG_BERRY_STAT && FALSE)              // Show berry tag info
#define SWSH_BAG_SCROLLING_BG       (SWSH_BAG_MENU && TRUE)                     // Enable scrolling background (BG3)

#define SWSH_BAG_IN_BAG_USE         (SWSH_BAG_MENU && TRUE)                     // Perform item actions (Use/Give) in bag (skip party menu)
#define SWSH_BAG_IN_BAG_REUSE       (SWSH_BAG_IN_BAG_USE && TRUE)               // Keep item cursor in party after use/give
#define SWSH_BAG_IN_BATTLE_USE      (SWSH_BAG_IN_BAG_USE && TRUE)               // Use items in bag during battle (skip party menu)
#define SWSH_BAG_PARTY_HP_BAR       (SWSH_BAG_IN_BAG_USE && TRUE)               // Show HP bar in party slot for certain items usage
#define SWSH_BAG_PARTY_HP_VALUE     (SWSH_BAG_PARTY_HP_BAR && FALSE)            // Print curHP/maxHP over the party HP bar
#define SWSH_BAG_ITEM_CURSOR        (SWSH_BAG_IN_BAG_USE && FALSE)              // When use/give item, TRUE uses item icon as cursor in party panel

#define SWSH_BAG_PYRAMID            (SWSH_BAG_MENU && TRUE)                     // Use SwSh bag menu for the Battle Pyramid
#define SWSH_BAG_PYRAMID_ACTION     (SWSH_BAG_PYRAMID && SWSH_BAG_IN_BAG_USE)   // Perform inline Use/Give in the pyramid bag

#define SWSH_BAG_BATTLE_POCKETS     (SWSH_BAG_MENU && TRUE)                     // In battle, show battle pockets (Medicine/Poké Balls/Battle Items/Berries) instead of the field pockets

#if SWSH_BAG_IN_BAG_USE
void BagMenu_OpenPartySelect(u8 taskId);
#if SWSH_BAG_IN_BATTLE_USE
void BagMenu_OpenPartySelectBattle(u8 taskId);
#endif // SWSH_BAG_IN_BATTLE_USE
#endif // SWSH_BAG_IN_BAG_USE

#endif // GUARD_SWSH_BAG_H
