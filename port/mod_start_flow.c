// mod_start_flow.c - a changed start of the story (hand-written, NOT part of the original game).
//
// To remove: configure with -DILB_MOD_START_FLOW=OFF (and delete this file if wanted). The only reference
// to it are guarded calls at the start of quests_init, quests_load, quest_set_new and
// quest_get_reward_item_class, placed by tools/draft_fix.py (ENTRY_HOOKS).
//
// The first quests in the original order (one chain):
//   Starting off Right -> Survival of the fittest -> Tools for the job -> Getting Stronger (gives the
//   Learning Potion) -> Instant use items (needs that potion; gives the class's first ancient item)
//   -> Ancient Items -> Cooking your food -> About Blocks -> Your home base -> Keeping yourself fed
//   -> Surviving Skills -> Commander Vile (gives the Homestone) -> Round and Round (the Homestone quest)
//   -> It Calls from Below -> ...
//
// With this module:
//   - "Getting Stronger" gives nothing and leads straight to "Ancient Items"; the first ancient item is
//     handed out when "Ancient Items" starts.
//   - When "Your home base" starts, the player is given the Homestone and "Round and Round" starts next to
//     it. It is no longer a link of the main chain, so it can be done at any time.
//   - "Commander Vile" gives the Learning Potion and is followed by "Instant use items" (which gives nothing
//     now), then "It Calls from Below" as before.
//
// The quest table is changed in memory (reward and next quest of four quests) every time a character starts
// or loads; the two hand-outs are done in quest_set_new. The dungeon game styles (3 and 4) have their own
// copies of the early quests (27, 28, 29, 30) and get the same treatment.
//
// Characters made before this module keep the original order: a character counts as "original order" when
// its saved quest states can only come from that order (see uses_original_order).
#ifdef ILB_MOD_START_FLOW
#include "ILB_draft_decls.h"

typedef struct _IO_FILE FILE;
extern FILE *stderr;
int fprintf( FILE *, const char *, ... );

#define GAME_STYLE              ( *(int *)ILB_ADDR( 0x6113F8 ) )    // 3 and 4 are the dungeon styles (quests_init)
#define QUEST_COMPLETE          4                                   // quest_complete_ok sets this; 0 = not started
#define QUEST_VILE              8
#define QUEST_HOMESTONE         9
#define ITEM_HOMESTONE          114
#define ITEM_LEARNING_POTION    175

#define UNIT_ARRAY              ( *(char **)ILB_ADDR( 0x3A73C9C ) )
#define UNIT_SIZE               8936
#define MAX_UNITS               0x4000
#define UNIT_TYPE_ITEM          3

static int new_order;           // set by ILB_mod_start_flow for the character being played
static int quest_instant;       // "Instant use items" for this game style (4 or 28)
static int quest_ancient;       // "Ancient Items" for this game style (17 or 29)
static int quest_home_base;     // "Your home base" for this game style (5 or 30)

static int started( struct unit *player, int quest )  { return player_get_quest_state( player, quest ) != 0; }
static int complete( struct unit *player, int quest ) { return player_get_quest_state( player, quest ) >= QUEST_COMPLETE; }

// Original order: "Instant use items" comes long before Commander Vile, and "It Calls from Below" is given
// by finishing "Round and Round". With this module "Instant use items" only starts after Vile and is what
// gives "It Calls from Below".
static int uses_original_order( struct unit *player, int instant, int after_homestone )
{
    if ( started( player, instant ) && !complete( player, QUEST_VILE ) )
        return 1;
    if ( complete( player, instant ) && complete( player, QUEST_VILE ) && !started( player, after_homestone ) )
        return 1;
    return 0;
}

static int past_both( struct unit *player, int level_up, int instant )      // nothing left for either order to change
{
    return complete( player, level_up ) && complete( player, instant ) && complete( player, QUEST_VILE ) && complete( player, QUEST_HOMESTONE );
}

static int first_ancient_class( void )
{
    int character_class = player_get_character_class( 0 );
    if ( character_class < 0 || character_class >= num_character_class_definitions_60E894 )
        return -1;
    return character_class_definitions_60CB44[ character_class ].ancient_1;
}

static int item_exists( int item_class )        // anywhere: bars, inventory, bank, on the ground
{
    for ( int i = 0; UNIT_ARRAY && i < MAX_UNITS; i++ )
    {
        struct unit *item = (struct unit *)( UNIT_ARRAY + i * UNIT_SIZE );
        if ( item->id != (dword)-1 && ( item->id & 0x3FFF ) == (dword)i && item->unit_type == UNIT_TYPE_ITEM && item->unit_class == item_class )
            return 1;
    }
    return 0;
}

// the same steps quest_complete_ok takes for a reward item
static void give_item( struct unit *player, int item_class, const char *what )
{
    dword id;
    if ( item_class < 0 )
        return;
    id = item_new( player->world_id, item_class, 3, 0, 1, 0 );
    if ( id == (dword)-1 )
        return;
    if ( player_add_inventory_item( id, 1, 0 ) == 0 )
        sub_474050( id, 1, player->position.x, player->position.y );
    if ( item_is_ancient( id ) )
        player_check_ancient_unlocks( 0 );
    fprintf( stderr, "mod start flow: %s (item class %d) given\n", what, item_class );
}

// Called at the start of quest_set_new, before the quest's state changes.
void ILB_mod_start_flow_quest_new( unsigned int quest )
{
    struct unit *player = (struct unit *)player_get_unit();
    if ( !new_order || player == 0 || started( player, quest ) )
        return;
    if ( (int)quest == quest_ancient )
        give_item( player, first_ancient_class(), "first ancient item" );
    if ( (int)quest == quest_home_base && !started( player, QUEST_HOMESTONE ) )
    {
        if ( !item_exists( ITEM_HOMESTONE ) )
            give_item( player, ITEM_HOMESTONE, "homestone" );
        quest_set_new( QUEST_HOMESTONE, 0 );
    }
}

// Called at the start of quest_get_reward_item_class: -2 = the game's own answer.
int ILB_mod_start_flow_reward( unsigned int quest )
{
    return new_order && (int)quest == quest_instant ? -1 : -2;
}

// Called at the start of quests_init (new character) and quests_load (loaded character); the character's
// quest states are set by then, and the list of quests that must be done in order is built right after.
void ILB_mod_start_flow( void )
{
    static int saved, original[ 6 ][ 3 ];                           // quest, reward, next quest
    static const int touched[ 6 ] = { 3, 4, QUEST_VILE, QUEST_HOMESTONE, 27, 28 };
    struct unit *player = (struct unit *)player_get_unit();
    int dungeon = GAME_STYLE == 3 || GAME_STYLE == 4;
    int level_up = dungeon ? 27 : 3, instant = dungeon ? 28 : 4, home_base = dungeon ? 30 : 5, after_instant, after_homestone;

    if ( quest_definitions_611494 == 0 )
        return;
    for ( int i = 0; i < 6; i++ )                                   // always start from the original table
    {
        if ( !saved )
        {
            original[ i ][ 0 ] = touched[ i ];
            original[ i ][ 1 ] = quest_definitions_611494[ touched[ i ] ].reward;
            original[ i ][ 2 ] = quest_definitions_611494[ touched[ i ] ].next_quest;
        }
        quest_definitions_611494[ touched[ i ] ].reward = original[ i ][ 1 ];
        quest_definitions_611494[ touched[ i ] ].next_quest = original[ i ][ 2 ];
    }
    saved = 1;
    new_order = 0;
    after_instant = quest_definitions_611494[ instant ].next_quest;
    after_homestone = quest_definitions_611494[ QUEST_HOMESTONE ].next_quest;
    if ( quest_definitions_611494[ level_up ].reward != ITEM_LEARNING_POTION || quest_definitions_611494[ QUEST_VILE ].reward != ITEM_HOMESTONE
         || (int)quest_definitions_611494[ level_up ].next_quest != instant || quest_definitions_611494[ QUEST_VILE ].next_quest != QUEST_HOMESTONE )
    {
        fprintf( stderr, "mod start flow: the quest table is not the one this was written for; left alone\n" );
        return;
    }
    if ( player != 0 )
        fprintf( stderr, "mod start flow: quest states - level-up %d, instant items %d, ancient items %d, home base %d, Vile %d, homestone %d (then %d)\n",
                 player_get_quest_state( player, level_up ), player_get_quest_state( player, instant ), player_get_quest_state( player, after_instant ),
                 player_get_quest_state( player, home_base ), player_get_quest_state( player, QUEST_VILE ),
                 player_get_quest_state( player, QUEST_HOMESTONE ), player_get_quest_state( player, after_homestone ) );
    if ( player != 0 && ( past_both( player, level_up, instant ) || uses_original_order( player, instant, after_homestone ) ) )
    {
        fprintf( stderr, "mod start flow: this character is already on (or past) the original order; left alone\n" );
        return;
    }
    quest_definitions_611494[ level_up ].reward = -1;
    quest_definitions_611494[ level_up ].next_quest = after_instant;
    quest_definitions_611494[ QUEST_VILE ].reward = ITEM_LEARNING_POTION;
    quest_definitions_611494[ QUEST_VILE ].next_quest = instant;
    quest_definitions_611494[ instant ].next_quest = after_homestone;
    quest_definitions_611494[ QUEST_HOMESTONE ].next_quest = -1;
    new_order = 1;
    quest_instant = instant;
    quest_ancient = after_instant;
    quest_home_base = home_base;
    // characters from earlier versions of this module: "Ancient Items" without the item, or "Your home base"
    // already started without the Homestone quest
    if ( player != 0 && started( player, quest_ancient ) && !complete( player, instant ) && first_ancient_class() >= 0
         && !item_exists( first_ancient_class() ) )
        give_item( player, first_ancient_class(), "first ancient item" );
    if ( player != 0 && started( player, home_base ) && !started( player, QUEST_HOMESTONE ) )
    {
        if ( !item_exists( ITEM_HOMESTONE ) )
            give_item( player, ITEM_HOMESTONE, "homestone" );
        quest_set_new( QUEST_HOMESTONE, 0 );
    }
    fprintf( stderr, "mod start flow: homestone and its quest with \"Your home base\" (quest %d), learning potion from Commander Vile\n", home_base );
}
#endif
