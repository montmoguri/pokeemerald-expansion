#ifndef GUARD_CONFIG_SWSH_PARTY_MENU_H
#define GUARD_CONFIG_SWSH_PARTY_MENU_H

#define SWSH_PARTY_MENU               TRUE

#define SWSH_PARTY_PC_ACCESS          (SWSH_PARTY_MENU && TRUE)     // Enable PC access directly from SwSh party menu
#define SWSH_PARTY_MON_IDLE_ANIMS     (SWSH_PARTY_MENU && TRUE)     // Animate Pokemon sprite while idling

#define SWSH_PARTY_MON_SHADOW         (SWSH_PARTY_MENU && TRUE)     // Add Pokemon shadow sprite
                                                                    // If SWSH_PARTY_STATUS_ICONS_FADE is active, MON_SHADOW_COLOR is used for sprite instead of alpha blending
#define SWSH_PARTY_STATUS_ICONS_FADE  (SWSH_PARTY_MENU && TRUE)     // Fade Pokemonstatus condition icons in and out to show their levels
                                                                    // Overtake alpha blending from Pokemon shadow if SWSH_PARTY_MON_SHADOW is active
#endif // GUARD_CONFIG_SWSH_PARTY_MENU_H
