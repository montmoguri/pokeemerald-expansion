#ifndef GUARD_SHOP_H
#define GUARD_SHOP_H

extern struct ItemSlot gMartPurchaseHistory[3];

void CreatePokemartMenu(const u16 *itemsForSale);
void CreateDecorationShop1Menu(const u16 *itemsForSale);
void CreateDecorationShop2Menu(const u16 *itemsForSale);
void CB2_ExitSellMenu(void);

// make public from static for use by swsh shop
void MapPostLoadHook_ReturnToShopMenu(void);
void RecordItemPurchase(u8 taskId);

#endif // GUARD_SHOP_H
