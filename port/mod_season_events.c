// mod_season_events.c - holiday events that follow the in-game season (hand-written, NOT part of the original).
//
// To remove: configure with -DILB_MOD_SEASON_EVENTS=OFF (and delete this file if wanted). The only reference
// to it is one guarded expression in monster_drop_seasonal_currency, placed by tools/draft_fix.py
// (VALUE_HOOKS).
//
// The original build has its Christmas event switched on for good: every monster that gives experience has
// a small chance to drop snowflakes (item 1022), whatever the date. The game also knows a pumpkin currency
// (item 533, from the Halloween releases) that nothing in this build drops. With this module:
//
//   Autumn           the same drop gives pumpkins instead of snowflakes
//   other seasons    snowflakes, as in the original
//
// Same chance and same number as the original drop; only the item differs.
#ifdef ILB_MOD_SEASON_EVENTS
#include "ILB_draft_decls.h"

#define SEASON                  ( *(int *)ILB_ADDR( 0x3891918 ) )   // 0 spring, 1 summer, 2 autumn, 3 winter
#define SEASON_AUTUMN           2
#define ITEM_PUMPKIN_CURRENCY   533

// Called where monster_drop_seasonal_currency names the item it drops; `original` is the game's choice.
int ILB_mod_season_currency( int original )
{
    return SEASON == SEASON_AUTUMN ? ITEM_PUMPKIN_CURRENCY : original;
}
#endif
