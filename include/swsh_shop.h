#ifndef GUARD_SWSH_SHOP_H
#define GUARD_SWSH_SHOP_H

#include "config/swsh_shop.h"
#include "constants/swsh_shop.h"

void SetBuyMenuMart_SwSh(u8 martType, const u16 *itemList, u16 itemCount);
void CB2_InitBuyMenu_SwSh(void);
void CreateCurrencyMart_SwSh(const u16 *products, enum MartCurrency currency);
void CreateCurrencyMartDecoration_SwSh(const u16 *products, enum MartCurrency currency);

#endif // GUARD_SWSH_SHOP_H
