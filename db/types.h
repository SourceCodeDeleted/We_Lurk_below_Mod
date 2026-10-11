// Hand-maintained types for ILB.exe (GOG build, 2022-01-09). Loaded into IDA by tools/ida/apply_types.py and
// checked by db/types_check.cpp. Sources for each layout are noted: "video" = 2019 source shown in the dev
// stream (README.MD), "binary" = offset confirmed in the 2022 exe. Offsets in comments are decimal.

#pragma once

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;

enum { num_damage_types = 6 };          // binary: makes unit_stats fit between unit+2140 and unit+2612
enum { max_emitters_per_unit = 16 };    // video
enum { max_animation_frames = 16 };     // video
enum { unit_custom_data_size = 6144 };  // binary: unit pool stride is 8936 bytes (init_units); was 4096 in 2019
enum { max_ancient_tree_nodes = 16 };   // video
enum { max_enchant_affixes = 4 };       // video
enum { num_item_zero_expansion = 25 };  // video
enum { num_item_invalid_expansion = 25 };  // video

struct vector2d { float x; float y; };
struct ivector2d { int x; int y; };
struct box { float v[4]; };             // 16 bytes (binary); member names and order not confirmed

// binary: the assert handler keeps a length and a 1024-character wide buffer; the strings table stores one
// of these after its 64-byte name (record size 2116 = 64 + 2052).
struct safestring
{
    int             length;
    unsigned short  text[1024];
};

enum unit_type_t
{
    unit_type_invalid = -1,
    unit_type_player = 0,
    unit_type_monster,
    unit_type_missile,
    unit_type_item,                     // binary: asserts compare unit_type with 3
    unit_type_object,
    unit_type_npc,
    unit_type_ambient,
    num_unit_types
};

enum unit_collision_shape_t
{
    unit_collision_shape_none = 0,
    unit_collision_shape_rectangle,
    unit_collision_shape_circle,
    num_collision_shapes
};

struct anim_data                        // video
{
    dword       flags;
    int         num_frames;
    int         rate;
    int         frame[max_animation_frames];
    vector2d    wand_direction;
};

// video layout; binary confirms attribute_flags (unit+2384), aspire/cosmic/holiday_set (unit+2536/2540/2548)
// and the total size (472 bytes) with num_damage_types = 6.
struct unit_stats
{
    int         attack_power;
    int         attack_power_scaled;
    int         vitality;
    int         vitality_scaled;
    int         intelligence;
    int         intelligence_scaled;
    int         old_wisdom;
    int         old_wisdom_scaled;
    int         dexterity;
    int         dexterity_scaled;
    int         vigor;
    int         vigor_scaled;
    int         speed;

    int         resistances[num_damage_types];
    int         critical_hit;

    int         hit_points;
    int         hit_points_scaled;
    int         hit_points_percent;
    int         hit_points_crit_steal;
    int         hit_point_regen;
    int         mana;
    int         mana_scaled;
    int         mana_percent;
    int         armor;
    int         armor_scaled;
    int         armor_percent;

    int         damage_type;
    int         min_damage;
    int         min_plus;
    int         max_damage;
    int         max_plus;
    int         undead_percent;
    int         demon_percent;
    int         attack_rate;
    int         rate_modifier;

    int         execute;
    int         armor_damage_percent;
    int         dodge_percent;
    int         block_percent;
    int         absorb_shield;
    int         ele_damage[num_damage_types];
    int         amplify;
    int         crit_damage_percent;
    int         resurrect_percent;

    int         gold_find;
    int         luck_rating;

    int         exp_percent;

    int         rank;
    int         crit_crit;
    int         crit_dodge;
    int         mind_crit;

    dword       attribute_flags;
    dword       attribute2_flags;
    dword       weapon_flags;
    dword       trigger_flags;
    dword       trigger2_flags;
    dword       status_flags;

    int         extra_attack_percent;
    int         reflect_percent;
    int         damage_done_reduction_percent;
    int         hit_point_regen_percent;
    int         purified_armor_percent;
    int         execute_reset_percent;
    int         recharge_speed;
    int         armor_penetration;

    int         elemental_crit[num_damage_types];

    int         riposte;
    int         damage_taken_reduction_percent;
    int         life_steal_percent;

    dword       expansion_stats[7];     // video; later builds may have named some of these

    dword       bard_flags;
    dword       cleric_flags;
    dword       enchanter_flags;
    dword       necro_flags;
    dword       paladin_flags;
    dword       rogue_flags;
    dword       warrior_flags;
    dword       wizard_flags;

    dword       aspire_set;
    dword       cosmic_set;
    dword       regen_set;
    dword       holiday_set;
    dword       future_sets[15];        // video; later builds may have named some of these
};

// video layout corrected against the binary. Differences from 2019: 16 new bytes after flags, velocity and
// gravity no longer sit between facing and collision_shape, one new dword after light_intensity.
struct unit
{
    dword           unit_version;       //    0
    dword           id;                 //    4
    safestring      name;               //    8
    dword           flags;              // 2060  binary (unit_flag_* masks used here)
    byte            unknown_2064[16];   // 2064  not in the 2019 source

    int             unit_type;          // 2080  binary
    int             unit_class;         // 2084  binary
    int             unit_mode;          // 2088  binary

    dword           model_id;           // 2092  binary

    anim_data *     anim;               // 2096  binary
    anim_data *     default_anim;       // 2100
    int             anim_delay;         // 2104
    int             anim_rate;          // 2108
    int             anim_frame;         // 2112
    bool            anim_new_frame;     // 2116
    vector2d        wand_anim_offset;   // 2120
    float           wand_anim_angle;    // 2128
    float           wand_anim_delta;    // 2132
    float           anim_speed_mod;     // 2136

    unit_stats      stats;              // 2140
    int             level;              // 2612  binary

    dword           world_id;           // 2616  binary
    vector2d        position;           // 2620  binary
    vector2d        unwrapped_position; // 2628
    vector2d        movement;           // 2636  order of these three taken from the video,
    vector2d        forces;             // 2644  only the 8-byte slots are confirmed
    vector2d        facing;             // 2652

    int             collision_shape;    // 2660  binary
    box             aabb;               // 2664
    float           width;              // 2680
    float           height;             // 2684
    float           radius;             // 2688

    float           light_radius;       // 2692
    float           light_intensity;    // 2696
    dword           unknown_2700;       // 2700  not in the 2019 source

    dword           emitter_ids[max_emitters_per_unit];  // 2704

    dword           buffdebuff_list;    // 2768

    unit *          dungeon_next;       // 2772
    unit *          type_previous;      // 2776
    unit *          type_next;          // 2780
    unit *          team_next;          // 2784

    int             data_version;       // 2788  binary
    byte            custom_data[unit_custom_data_size];  // 2792  binary
};

// video layout; binary confirms in_world (1), stack_count (12), slot_shape (72), max_stack (104),
// total_slevel (112), num_crystal_slots (136), crystal_ancient_affix_id (280) and sizeof == 364 (item_save).
// The two named fields in the expansion areas are the only post-2019 additions found so far.
struct item_custom_data
{
    bool        identified;             //   0

    bool        in_world;               //   1
    bool        never_decay;            //   2
    int         on_ground_count;        //   4

    int         bounces;                //   8
    int         stack_count;            //  12
    int         spawn_life;             //  16

    int         item_quality;           //  20
    dword       prefix_string_id;       //  24
    dword       suffix_string_id;       //  28
    dword       rare_string_id;         //  32
    dword       unique_string_id;       //  36
    dword       delivery_string_id;     //  40
    dword       unidentified_string_id; //  44
    dword       affix_description_id;   //  48

    int         weapon_delivery_type;   //  52
    int         wand_charges;           //  56
    int         wand_recharge_ticks;    //  60

    dword       crystal_id;             //  64
    int         crystal_shards;         //  68

    int         slot_shape;             //  72

    dword       mushroom_type;          //  76

    int         gold;                   //  80
    int         cost;                   //  84
    int         buyback_price;          //  88

    int         cooldown_ticks;         //  92
    bool        can_upgrade;            //  96

    int         eat_amount;             // 100
    int         max_stack;              // 104

    dword       item_flags;             // 108
    dword       total_slevel;           // 112
    byte        ancient_tree[max_ancient_tree_nodes];  // 116
    int         level_required;         // 132
    int         num_crystal_slots;      // 136  was zero_expansion[0]
    dword       zero_expansion[num_item_zero_expansion - 1];  // 140

    dword       original_slot;          // 236
    dword       glow_id;                // 240
    dword       next_dropped;           // 244
    dword       enchant_id[max_enchant_affixes];  // 248
    dword       invalid_expansion_a[4]; // 264
    dword       crystal_ancient_affix_id;  // 280  was invalid_expansion[4]
    dword       invalid_expansion_b[num_item_invalid_expansion - 5];  // 284
};
