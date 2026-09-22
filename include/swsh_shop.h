#ifndef GUARD_SWSH_SHOP_H
#define GUARD_SWSH_SHOP_H

#define SWSH_SHOP_MENU              TRUE                            // Use SwSh shop menu
#define SWSH_SHOP_TM_INFO           (SWSH_SHOP_MENU && TRUE)        // SELECT shows a TM/HM's PP, power and accuracy
#define SWSH_SHOP_CONTEST_INFO      (SWSH_SHOP_TM_INFO && TRUE)     // TM/HM info also cycles through the contest description and appeal/jam hearts
#define SWSH_SHOP_BERRY_STAT        (SWSH_SHOP_MENU && TRUE)        // SELECT shows a Berry's size, firmness and flavors
#define SWSH_SHOP_BERRY_TAG         (SWSH_SHOP_BERRY_STAT && TRUE)  // Berry info also cycles through its Berry Tag description

void SetBuyMenuMart_SwSh(u8 martType, const u16 *itemList, u16 itemCount);
void CB2_InitBuyMenu_SwSh(void);

#endif // GUARD_SWSH_SHOP_H
