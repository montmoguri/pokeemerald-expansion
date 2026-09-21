#ifndef GUARD_SWSH_SHOP_H
#define GUARD_SWSH_SHOP_H

#define SWSH_SHOP_MENU              TRUE                                        // Use SwSh shop menu

void SetBuyMenuMart_SwSh(u8 martType, const u16 *itemList, u16 itemCount);
void CB2_InitBuyMenu_SwSh(void);

#endif // GUARD_SWSH_SHOP_H
