// cheats.c - testing shortcuts (hand-written, NOT part of the original game).
//
// To remove: delete this file and configure with -DILB_CHEATS=OFF (or just the latter). Nothing else in the
// game or the port refers to it except one guarded call in draft/native/native_platform.c and, for god
// mode, three guarded calls placed by tools/draft_fix.py (ENTRY_HOOKS) at the start of unit_take_damage,
// unit_take_undefended_damage and do_player_die. While it is in, the game does not see the `, F1, F2, F3
// and F4 keys during play (F1 normally opens one of its text panels).
//
//   `    open the level-up panel with 99 points to spend, at any time
//   F1   fill gold and orbs (what ancient item skill nodes cost) and open the skill tree of an ancient item,
//        if the player has one in the world
//   F2   set the survival skill points to 99 and open (or close) the survival panel
//   F3   crafting materials: every block type to 10000 (the game's own limit), a stack of 999 of every ore,
//        metal bar and gem, and 99999 crystal shards and compost. Ores, bars and gems are real items: a stack
//        the player already has (inventory or bank) is topped up, a missing one is made with the game's own
//        item_new / player_add_inventory_item and moved to a free bank slot if there is one. With no free
//        slot left the rest is skipped (the count is printed).
//   F4   god mode on / off: players take no damage and cannot die. It covers every player in the game, so
//        that the two copies of a network game stay in step. Off again at the next start of the program.
//
// The player's own data starts 2792 bytes into its unit: level-up points at +56, orbs at +60, survival
// points at +80, gold at +108 (from player_get_leveling_points, player_get_orbs, player_buy_survival_skill,
// player_get_gold), crystal shards at +112, compost at +116, 46 inventory slots (item ids) at +460, bank slots
// at +704 with their number at +2144, one 16-bit counter per block type at +2244 (from
// player_update_component_count, player_update_recipe_component, player_add_to_block_inventory).
// An item's own data starts at the same offset in its unit: stack count at +12, largest stack at +104.
#ifdef ILB_CHEATS
#include "ILB_draft_decls.h"

typedef struct _IO_FILE FILE;
extern FILE *stderr;
int fprintf( FILE *, const char *, ... );

#define PLAYER_DATA( player )       ( (int *)( (char *)( player ) + 2792 ) )
#define LEVEL_UP_STATE( client )    ( ( (int *)ILB_ADDR( 0x3916AE0 ) )[ 216 * ( client ) ] )    // 0 closed, 1 "level up" button shown
#define UNIT_ARRAY                  ( *(char **)ILB_ADDR( 0x3A73C9C ) )
#define UNIT_SIZE                   8936
#define MAX_UNITS                   0x4000
#define UNIT_TYPE_PLAYER            0
#define UNIT_TYPE_ITEM              3
#define UNIT_HIT_POINTS( unit )     ( ( (int *)( unit ) )[ 555 ] )      // what unit_take_undefended_damage takes from
#define APP_MODE                    ( *(int *)ILB_ADDR( 0x3A73CC0 ) )
#define APP_MODE_GAME               11

static void cheat_level_up( struct unit *player )
{
    PLAYER_DATA( player )[ 14 ] = 99;               // leveling points
    if ( LEVEL_UP_STATE( 0 ) == 1 )
        level_up_button_left_clicked( 5100 );       // the game's own button: closes the button, opens the panel
    else if ( LEVEL_UP_STATE( 0 ) == 0 )
        level_up_popup_activate( 0 );
    fprintf( stderr, "cheat: level-up panel, 99 points\n" );
}

static void cheat_ancient_tree( struct unit *player )
{
    PLAYER_DATA( player )[ 15 ] = 99;               // orbs
    PLAYER_DATA( player )[ 27 ] = 9999999;          // gold
    for ( int i = 0; UNIT_ARRAY && i < MAX_UNITS; i++ )
    {
        struct unit *item = (struct unit *)( UNIT_ARRAY + i * UNIT_SIZE );
        if ( item->id == (dword)-1 || ( item->id & 0x3FFF ) != (dword)i || item->unit_type != UNIT_TYPE_ITEM )
            continue;
        if ( item->unit_class < 0 || item->unit_class >= num_item_definitions_611458 )
            continue;
        if ( !item_definitions_61145C[ item->unit_class ].is_ancient || item_definitions_61145C[ item->unit_class ].ancient_version == 0 )
            continue;
        ui_ancient_activate( item );
        fprintf( stderr, "cheat: gold and orbs filled, skill tree of ancient item %d opened\n", i );
        return;
    }
    fprintf( stderr, "cheat: gold and orbs filled; no ancient item found to open a skill tree for\n" );
}

#define ITEM_STACK_CHEAT    999         // guess: larger than any real stack (500), still fits the count display
#define INVENTORY_SLOTS     46
#define INVALID_ID          ( (dword)-1 )

static struct unit *cheat_unit( dword id )
{
    struct unit *unit;
    if ( id == INVALID_ID || UNIT_ARRAY == 0 )
        return 0;
    unit = (struct unit *)( UNIT_ARRAY + ( id & 0x3FFF ) * UNIT_SIZE );
    return unit->id == id && unit->unit_type == UNIT_TYPE_ITEM ? unit : 0;
}

// the slot (inventory first, then bank) holding an item of this class, or one holding `wanted_id`; 0 if none
static dword *cheat_find_slot( struct unit *player, int item_class, dword wanted_id )
{
    int *data = PLAYER_DATA( player );
    int bank_slots = data[ 2144 / 4 ];
    for ( int i = 0; i < INVENTORY_SLOTS + bank_slots; i++ )
    {
        dword *slot = (dword *)( i < INVENTORY_SLOTS ? &data[ 460 / 4 + i ] : &data[ 704 / 4 + i - INVENTORY_SLOTS ] );
        struct unit *item = cheat_unit( *slot );
        if ( item_class < 0 ? *slot == wanted_id : ( item != 0 && item->unit_class == item_class ) )
            return slot;
    }
    return 0;
}

static void cheat_materials( struct unit *player )
{
    int *data = PLAYER_DATA( player );
    int made = 0, topped_up = 0, skipped = 0, blocks = 0;

    data[ 112 / 4 ] = 99999;                        // crystal shards
    data[ 116 / 4 ] = 99999;                        // compost
    for ( int i = 0; i < 76; i++ )                  // the game's list of block items (player_update_component_count)
    {
        int item_class = dword_5DFAB0[ i ];
        if ( item_class < 0 || item_class >= num_item_definitions_611458 || item_definitions_61145C[ item_class ].block_class < 0 )
            continue;
        ( (unsigned short *)( (char *)data + 2244 ) )[ item_definitions_61145C[ item_class ].block_class ] = 10000;
        blocks++;
    }
    ( (char *)data )[ 3413 ] = 1;                   // "block counts changed", as player_add_to_block_inventory sets

    for ( int item_class = 0; item_class < num_item_definitions_611458; item_class++ )
    {
        dword *slot, *bank, id;
        struct unit *item;
        int is_gem = item_class >= 724 && item_class <= 727;    // gem_green .. gem_orange in the item table
        if ( !item_definitions_61145C[ item_class ].is_ore && !item_definitions_61145C[ item_class ].is_metal_bar && !is_gem )
            continue;
        slot = cheat_find_slot( player, item_class, 0 );
        if ( slot == 0 )
        {
            bank = cheat_find_slot( player, -1, INVALID_ID );   // any free slot at all? (the game asserts otherwise)
            if ( bank == 0 )
            {
                skipped++;
                continue;
            }
            id = item_new( player->world_id, item_class, 3, 1, 1, 0 );      // as player_attempt_crafting makes its result
            if ( id == INVALID_ID || !player_add_inventory_item( id, 0, 1 ) || ( slot = cheat_find_slot( player, -1, id ) ) == 0 )
            {
                skipped++;
                continue;
            }
            // out of the way into a free bank slot, the way try_swap_to_bank_from_inv moves an item
            bank = 0;
            for ( int i = 0; i < data[ 2144 / 4 ] && bank == 0; i++ )
                if ( (dword)data[ 704 / 4 + i ] == INVALID_ID )
                    bank = (dword *)&data[ 704 / 4 + i ];
            if ( bank != 0 && bank != slot )
            {
                *bank = *slot;
                *slot = INVALID_ID;
                slot = bank;
            }
            made++;
        }
        else
            topped_up++;
        item = cheat_unit( *slot );
        PLAYER_DATA( item )[ 104 / 4 ] = ITEM_STACK_CHEAT;      // largest stack
        PLAYER_DATA( item )[ 12 / 4 ] = ITEM_STACK_CHEAT;       // stack count
    }
    player_finished_item_placing( (int *)player, 1 );
    player_needs_recipe_component_update( (int)player );
    fprintf( stderr, "cheat: materials - %d block types at 10000, %d ore/bar/gem stacks made, %d topped up, %d skipped (no free slot); shards and compost 99999\n",
             blocks, made, topped_up, skipped );
}

static void cheat_survival( struct unit *player )
{
    PLAYER_DATA( player )[ 20 ] = 99;               // survival points (player_buy_survival_skill takes one per skill)
    ui_survival_panel_toggle();                     // the game's own key handler for the panel
    fprintf( stderr, "cheat: 99 survival points, survival panel toggled\n" );
}

static int god_mode;

// Called at the start of unit_take_damage and unit_take_undefended_damage with the unit about to be hurt:
// 1 if nothing is to happen to it.
int ILB_cheat_god( unsigned int unit_id )
{
    struct unit *unit;
    if ( !god_mode || UNIT_ARRAY == 0 || unit_id >= MAX_UNITS )
        return 0;
    unit = (struct unit *)( UNIT_ARRAY + unit_id * UNIT_SIZE );
    return unit->id == unit_id && unit->unit_type == UNIT_TYPE_PLAYER;
}

// Called at the start of do_player_die: 1 if the player stays alive. Whatever emptied the hit points without
// going through the two routines above is undone to one point.
int ILB_cheat_god_die( struct unit *player )
{
    if ( !god_mode || player == 0 )
        return 0;
    if ( UNIT_HIT_POINTS( player ) < 1 )
        UNIT_HIT_POINTS( player ) = 1;
    return 1;
}

static void cheat_god( void )
{
    const char *text = ( god_mode = !god_mode ) ? "God mode on" : "God mode off";
    int n = 0;
    for ( ; text[ n ]; n++ ) stru_60B960.text[ n ] = (unsigned char)text[ n ];      // the game's own message line
    stru_60B960.text[ n ] = 0;
    stru_60B960.length = n;
    sub_5610F0( -256, 0 );
    fprintf( stderr, "cheat: %s\n", text );
}

// Called for every key press and release, before the game sees it. Returns 1 if the key is one of the
// shortcuts: the game is then not told about it (F1 is also the game's own key for a text panel).
int ILB_cheat_key( int key, int pressed )
{
    struct unit *player;
    if ( APP_MODE != APP_MODE_GAME || ( key != '`' && ( key < 0x4000003A || key > 0x4000003D ) ) )      // F1 .. F4
        return 0;
    player = (struct unit *)player_get_unit();
    if ( player == 0 || !pressed )
        return 1;
    if ( key == '`' )
        cheat_level_up( player );
    else if ( key == 0x4000003A )
        cheat_ancient_tree( player );
    else if ( key == 0x4000003B )
        cheat_survival( player );
    else if ( key == 0x4000003D )
        cheat_god();
    else
        cheat_materials( player );
    return 1;
}
#endif
