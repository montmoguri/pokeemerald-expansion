#ifndef GUARD_CONSTANTS_SWSH_SHOP_H
#define GUARD_CONSTANTS_SWSH_SHOP_H

// First u16 of a mart list whose prices are inlined after each item instead of read from item data
#define MART_LIST_INLINE_PRICES     0xFFFF

enum MartCurrency
{
    MART_CURRENCY_MONEY,
    MART_CURRENCY_BATTLE_POINTS,
};

#endif // GUARD_CONSTANTS_SWSH_SHOP_H
