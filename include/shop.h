#ifndef GUARD_SHOP_H
#define GUARD_SHOP_H

extern struct ItemSlot gMartPurchaseHistory[3];

void CreatePokemartMenu(const u16 *itemsForSale);
void CreateDecorationShop1Menu(const u16 *itemsForSale);
void CreateDecorationShop2Menu(const u16 *itemsForSale);
void CB2_ExitSellMenu(void);
void MapPostLoadHook_ReturnToShopMenu(void);  // make public from static for use by swsh shop

#endif // GUARD_SHOP_H
