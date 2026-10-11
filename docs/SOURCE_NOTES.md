# It Lurks Below - source notes from dev stream video

Transcribed from on-screen source code in `Video/dwitch_u7coig2a.mp4` (Visual Studio solution `ILB`, x86 Debug,
recorded 2019-09-05). Field order and line numbers are as shown on screen, so struct layouts are in declaration order.
Timestamps are video time (mm:ss). Anything not visible in the video is marked as such.

Open tabs seen: `g_units.h`, `g_items.cpp`, `g_objects.cpp`, `main.cpp`, `g_dungeon.cpp`, `g_player.cpp`.

## g_units.h

### File header and unit flags (lines 1-35, video 11:08)

```c
//-----------------------------------------------------------------------------
// ShooterRPG
// by David Brevik
// Graybeard Games
// (C) 2016, Copyright 2016, All rights reserved.

#ifndef _G_UNITS_H_
#define _G_UNITS_H_

//-----------------------------------------------------------------------------
// defines

#define unit_flag_delete                0x80000000      // this unit will be deleted soon
#define unit_flag_active_list           0x40000000      // currently on active list
#define unit_flag_activated             0x20000000      // this unit needs to be put on the active list
#define unit_flag_nocollide             0x08000000      // this unit can walk through walls/blocks/etc
#define unit_flag_selected              0x04000000      // draw with highlight?
#define unit_flag_got_hit               0x02000000      // flash me when I get hit
#define unit_flag_in_air                0x01000000      // either jumping or falling
#define unit_flag_crouching             0x00800000      // crouching
#define unit_flag_fatigued              0x00400000      // this unit is out of energy
#define unit_flag_has_wand              0x00200000      // this unit is holding a wand
#define unit_flag_status_immune         0x00100000      // this unit is immune to status effects and AoE damage (bosses)
#define unit_flag_nearby                0x00080000      // this unit is near the player
#define unit_flag_damage_block          0x00040000      // this unit blocks beams and piercing
#define unit_flag_has_melee_sas         0x00020000      // this unit is holding sword/shield
#define unit_flag_has_melee_dual        0x00010000      // this unit is holding dual swords
#define unit_flag_dashing               0x00008000      // this unit is in the act of dashing
#define unit_flag_whirlwinding          0x00004000      // this unit is in the act of whirlwind
#define unit_flag_has_melee_2h          0x00002000      // this unit is holding two-handed weapon

#define unit_weapon_flags               ( unit_flag_has_wand | unit_flag_has_melee_sas | unit_flag_has_melee_dual | unit_flag_has_melee_2h /* line runs off screen */

#define unit_flag_good_guy              0x00000001
#define unit_flag_bad_guy               0x00000002
```

### Gravity, blind, collision, lighting, graphics, animation (lines 38-99)

```c
// unit gravity defines
#define unit_gravity_acceleration   0.01f
#define unit_gravity_max            0.5f
#define unit_jump_velocity          -0.25f

// blind status defines
#define blind_light_level       8
#define blind_color             make_color( blind_light_level, blind_light_level, blind_light_level )

// collision shapes
enum
{
    unit_collision_shape_none = 0,
    unit_collision_shape_rectangle,
    unit_collision_shape_circle,

    num_collision_shapes
};

#define max_unit_width              7.0f
#define max_unit_height             7.0f

// lighting
#define unit_light_intensity_max        255.0f

// unit graphics
#define max_verts_per_unit              6
#define max_emitters_per_unit           16

// animation
#define max_animation_frames        16

#define animflag_play_once          0x0000
#define animflag_loop               0x0001
#define animflag_hold_last          0x0002
#define animflag_uninteruptable     0x8000

struct anim_data
{
    dword       flags;
    int         num_frames;
    int         rate;
    int         frame[ max_animation_frames ];
    vector2d    wand_direction;
};

// data sizes
#define unit_custom_data_size           4096
```

### Unit types (lines 103-115)

```c
enum
{
    unit_type_invalid = -1,
    unit_type_player = 0,
    unit_type_monster,
    unit_type_missile,
    unit_type_item,
    unit_type_object,
    unit_type_npc,
    unit_type_ambient,

    num_unit_types
};
```

### Attribute flags (lines 117-171) - `unit_stats.attribute_flags` / `attribute2_flags`

```c
// typedefs

#define attribute_min           0
#define attribute_max           20

#define attribute_flag_slow_digestion       0x00000001
#define attribute_flag_no_digestion         0x00000002
#define attribute_flag_stamina              0x00000004
#define attribute_flag_in_shadows           0x00000008
#define attribute_flag_regeneration         0x00000010
#define attribute_flag_infravision          0x00000020
#define attribute_flag_no_blocks            0x00000040
#define attribute_flag_feather_fall         0x00000080
#define attribute_flag_high_jump            0x00000100
#define attribute_flag_crit_prot            0x00000200
#define attribute_flag_immunity             0x00000400
#define attribute_flag_boring               0x00000800
#define attribute_flag_pbaoe                0x00001000
#define attribute_flag_poison               0x00002000
#define attribute_flag_slow                 0x00004000
#define attribute_flag_t_absorb_shield      0x00008000
#define attribute_flag_double_jump          0x00010000
#define attribute_flag_steady_feet          0x00020000
#define attribute_flag_digging              0x00040000
#define attribute_flag_light                0x00080000
#define attribute_flag_large_pbaoe          0x00100000
#define attribute_flag_glide                0x00200000
#define attribute_flag_nightvision          0x00400000
#define attribute_flag_fly                  0x00800000
#define attribute_flag_resurrect            0x01000000
#define attribute_flag_potion_regen         0x02000000
#define attribute_flag_skip_charge          0x04000000
#define attribute_flag_no_target            0x08000000
#define attribute_flag_quick_pbaoe          0x10000000
#define attribute_flag_magic_ele_dmg_lvl    0x20000000
#define attribute_flag_vitality_to_resist   0x40000000
#define attribute_flag_triple_jump          0x80000000

#define attribute2_flag_resist_to_dmg       0x00000001
#define attribute2_flag_improved_absorb     0x00000002
#define attribute2_flag_wisp                0x00000004
#define attribute2_flag_extra_vitality      0x00000008
#define attribute2_flag_archeology          0x00000010
#define attribute2_flag_bat                 0x00000020
#define attribute2_flag_purified_armor      0x00000040
#define attribute2_flag_vigor_to_execute    0x00000080
#define attribute2_flag_crit_to_vigor       0x00000100
#define attribute2_flag_dig_corruption      0x00000200
#define attribute2_flag_absorb_shield       0x00000400
#define attribute2_flag_necro_extra_soul    0x00000800
#define attribute2_flag_poison_spread       0x00001000
#define attribute2_flag_quakeboots          0x00002000
#define attribute2_flag_explosive           0x00004000
#define attribute2_flag_conductor           0x00008000
```

### Class ancient-item flags (lines 173-209) - `unit_stats.wizard_flags` / `warrior_flags`

Only the wizard and warrior blocks exist at this point in the file; no defines for the other class flag fields
are shown.

```c
// flags for wizard class ancient items
#define wizard_flags_frozen_shield_minion   0x00000001
#define wizard_flags_thunder_text_cooldown  0x00000002
#define wizard_flags_familiar_armor_pen     0x00000004
#define wizard_flags_familiar_aoe           0x00000010      // 0x00000008 is skipped in the source
#define wizard_flags_familiar_heal          0x00000020
#define wizard_flags_firestorm_passive      0x00000040
#define wizard_flags_hit_burn_extradmg_20   0x00000080
#define wizard_flags_hit_burn_extradmg_40   0x00000100
#define wizard_flags_hit_cold_extradmg_20   0x00000200
#define wizard_flags_hit_cold_extradmg_40   0x00000400
#define wizard_flags_lightning_heal_1       0x00000800
#define wizard_flags_lightning_heal_2       0x00001000
#define wizard_flags_lightning_heal_3       0x00002000
#define wizard_flags_lightning_heal_4       0x00004000
#define wizard_flags_lightning_heal_mask    ( wizard_flags_lightning_heal_1 | wizard_flags_lightning_heal_2 | wizard_flags_lightning_heal_... /* off screen */
#define wizard_flags_familiar_dmg_1         0x00008000
#define wizard_flags_familiar_dmg_2         0x00010000
#define wizard_flags_familiar_dmg_4         0x00020000
#define wizard_flags_familiar_dmg_8         0x00040000
#define wizard_flags_familiar_dmg_mask      ( wizard_flags_familiar_dmg_1 | wizard_flags_familiar_dmg_2 | wizard_flags_familiar_dmg_4 | ... /* off screen */

// flags for warrior class ancient items
#define warrior_flags_reflect_missiles      0x00000001
#define warrior_flags_reflect_block         0x00000002
#define warrior_flags_def_dex               0x00000004
#define warrior_flags_def_vit               0x00000008
#define warrior_flags_def_vitdmg            0x00000010
#define warrior_flags_crouch_armor          0x00000020
#define warrior_flags_activate_blockdmg     0x00000040
#define warrior_flags_weap_vit              0x00000080
#define warrior_flags_weap_dex              0x00000100
#define warrior_flags_weap_execute          0x00000200
#define warrior_flags_triple_attack         0x00000400
#define warrior_flags_weap_lifesteal        0x00000800
#define warrior_flags_weap_hpdmg            0x00001000
#define warrior_flags_weap_whirlwind        0x00002000
```

### Weapon flags (lines 211-230) - `unit_stats.weapon_flags`

```c
// wand flags ( lower word 0x0000ffff ), upper word is melee
#define wand_flag_aoe                       0x00000001
#define wand_flag_aoe_dig                   0x00000002
#define wand_flag_rainbow                   0x00000004
#define wand_flag_shotgun                   0x00000008
#define wand_flag_piercing                  0x00000010
#define wand_flag_bounce                    0x00000020
#define wand_flag_dig                       0x00000040
#define wand_flag_special                   0x00000080
#define wand_flag_spray                     0x00000100
#define wand_flag_aoe_dig_no_blocks         0x00000200

// melee flags ( upper word 0xffff0000 ), lower word is wands
#define melee_flag_stun                     0x00010000
#define melee_flag_bleed                    0x00020000
#define melee_flag_cleave                   0x00040000
#define melee_flag_knockback                0x00080000
#define melee_flag_projectile               0x00100000
//#define melee_flag_rainbow                    0x00200000
#define melee_flag_mask                     0xffff0000
```

### Trigger flags (lines 232-285) - `unit_stats.trigger_flags` / `trigger2_flags`

```c
// trigger flags
#define trigger_flag_get_hit_ward           0x00000001
#define trigger_flag_get_hit_shield         0x00000002
#define trigger_flag_hit_execute            0x00000004
#define trigger_flag_hit_treasure           0x00000008
#define trigger_flag_kill_heal              0x00000010
#define trigger_flag_crit_crit              0x00000020
#define trigger_flag_crit_dodge             0x00000040
#define trigger_flag_hit_devour             0x00000080
#define trigger_flag_hit_blind              0x00000100
#define trigger_flag_hit_confuse            0x00000200
#define trigger_flag_hit_amplify            0x00000400
#define trigger_flag_attack_growth          0x00000800
#define trigger_flag_kill_regen             0x00001000
#define trigger_flag_get_hit_berzerker      0x00002000
#define trigger_flag_hit_fire               0x00004000
#define trigger_flag_hit_poison             0x00008000
#define trigger_flag_get_hit_poison         0x00010000
#define trigger_flag_get_hit_scream         0x00020000
#define trigger_flag_hit_ice_spear          0x00040000
#define trigger_flag_kill_soul_jar          0x00080000
#define trigger_flag_hit_stack              0x00100000
#define trigger_flag_ancient_crit_dodge     0x00200000
#define trigger_flag_ancient_kill_heal      0x00400000
#define trigger_flag_status_cleanse         0x00800000
#define trigger_flag_get_crit_regen         0x01000000
#define trigger_flag_hit_jump               0x02000000
#define trigger_flag_kill_smite             0x04000000
#define trigger_flag_get_hit_soul_jar       0x08000000
#define trigger_flag_hit_vitality           0x10000000
#define trigger_flag_hit_lightning          0x20000000
#define trigger_flag_hit_cold               0x40000000
#define trigger_flag_hit_mind_break         0x80000000

#define trigger2_flag_hit_haste             0x00000001
#define trigger2_flag_execute_reset         0x00000002
#define trigger2_flag_kill_purified_heal    0x00000004
#define trigger2_flag_extra_hit_vigor       0x00000008
#define trigger2_flag_crit_min_dmg          0x00000010
#define trigger2_flag_hit_armor_pen         0x00000020
#define trigger2_flag_crit_shield_charge    0x00000040
#define trigger2_flag_get_hit_autostorm     0x00000080
#define trigger2_flag_hit_familiar_heal     0x00000100
#define trigger2_flag_crit_autofire         0x00000200
#define trigger2_flag_get_hit_riposte       0x00000400
#define trigger2_flag_crit_next             0x00000800
#define trigger2_flag_get_hit_reflect       0x00001000
#define trigger2_flag_blockdmg_next         0x00002000
#define trigger2_flag_get_hit_bracer_atk    0x00004000
#define trigger2_flag_crit_charm_hps        0x00008000
#define trigger2_flag_execute_charm_hps     0x00010000
#define trigger2_flag_execute_heal          0x00020000
#define trigger2_flag_get_hit_autoheal      0x00040000
#define trigger2_flag_hit_deep_freeze       0x00080000
```

### Status flags (lines 287-306) - `unit_stats.status_flags`

```c
// status effects
#define status_flag_poisoned                0x00000001
#define status_flag_frozen                  0x00000002
#define status_flag_on_fire                 0x00000004
#define status_flag_charged                 0x00000008
#define status_flag_stunned                 0x00000010
#define status_flag_blind                   0x00000020
#define status_flag_confused                0x00000040
#define status_flag_bleeding                0x00000080
#define status_flag_knockback               0x00000100
#define status_flag_mind_break              0x00000200
#define status_flag_purified                0x00000400
#define status_flag_yanked                  0x00000800
#define status_flag_execute_zero            0x00001000
#define status_flag_pushed                  0x00002000
#define status_flag_crit_prot_zero          0x00004000
#define status_flag_no_autoheal             0x00008000
#define status_flag_no_poison_spread        0x00010000

#define status_elemental_mask               0x0000001f
```

### Scaling constants (lines 308-309)

```c
#define dex_to_armor_factor             5
#define vigor_to_resistance_factor      2
```

`num_damage_types` is used below but its value is never shown in the video (the earlier draft of this file used 8 as
a placeholder).

### struct unit_stats (lines 311-423, video 11:15 and 13:10-13:25)

```c
struct unit_stats
{
    // attributes that are 0-(attribute_max) range [Modified by all items]
    int         attack_power;                       // increase damage done, percentage
    int         attack_power_scaled;                // attack power scaled on player level
    int         vitality;                           // used to calculate how many hit points the unit has
    int         vitality_scaled;                    // vitality scaled on player level
    int         intelligence;                       // used to calculate how much mana the unit has
    int         intelligence_scaled;                // intelligence scaled on player level
    int         old_wisdom;                         // unused
    int         old_wisdom_scaled;                  // unused
    int         dexterity;                          // used by multiplying by a scaling factor and added to Armor Class and melee damage
    int         dexterity_scaled;                   // dex scaled on player level
    int         vigor;                              // used by multiplying by a scaling factor and added to resistances
    int         vigor_scaled;                       // vigor scaled on player level
    int         speed;                              // speed unit moves, percentage max 50%

    // percentage stats [Modified by armor/rings]
    int         resistances[ num_damage_types ];    // % chance to not get elemental effect
    int         critical_hit;                       // percent chance of doing 2x damage

    // stats that are calculated
    int         hit_points;                         // unit life
    int         hit_points_scaled;                  // life scaled on player level
    int         hit_points_percent;                 // life scaled on flat percent
    int         hit_points_crit_steal;              // percent of critical damage done added to hit points
    int         hit_point_regen;                    // how many hit points per second restored
    int         mana;                               // used to shoot wands
    int         mana_scaled;                        // mana scaled on player level
    int         mana_percent;                       // mana scaled on flat percent
    int         armor;                              // decrease damage taken, flat number up to 85%
    int         armor_scaled;                       // armor scaled on player level
    int         armor_percent;                      // armor scaled on flat percent

    // weapon damage stats
    int         damage_type;                        // type of damage you are doing
    int         min_damage;                         // min damage base attack does
    int         min_plus;                           // + to min damage
    int         max_damage;                         // max damage base attack does
    int         max_plus;                           // + to max damage
    int         undead_percent;                     // % of damage increase against undead
    int         demon_percent;                      // % of damage increase against demon
    int         attack_rate;                        // how fast you attack
    int         rate_modifier;                      // % change in attack_rate

    // special stats for skills/abilities/etc
    int         execute;                            // % chance of executing non-bosses
    int         armor_damage_percent;               // % of armor added to weapon damage (cleric skill)
    int         dodge_percent;                      // % chance of zero damage
    int         block_percent;                      // % chance of zero damage
    int         absorb_shield;                      // amount of damage to absorb
    int         ele_damage[ num_damage_types ];     // % damage increase for an element
    int         amplify;                            // % more damage taken
    int         crit_damage_percent;                // % increase in critical damage done (more than 2x)
    int         resurrect_percent;                  // % of life to resurrect with

    // item drop modifiers
    int         gold_find;                          // % more money when gold drops
    int         luck_rating;                        // increase diminishing returns number for increasing % of rare magic items

    // experience modifier
    int         exp_percent;                        // % more exp when you kill something

    // used for ancient items
    int         rank;
    int         crit_crit;                          // used to increase crit when you crit
    int         crit_dodge;                         // used to increase dodge when you crit
    int         mind_crit;                          // used to crit against targets with mind-break

    // special flags
    dword       attribute_flags;                    // Slow digestion, stealth, aggravation, regeneration, blindness, Infra vision
    dword       attribute2_flags;
    dword       weapon_flags;                       // flags for missiles, wands, etc
    dword       trigger_flags;                      // special effects that are a reaction to something (get hit, kill something, damage something)
    dword       trigger2_flags;
    dword       status_flags;                       // unit is poisoned, on-fire, etc

    // more ancient item stats
    int         extra_attack_percent;
    int         reflect_percent;
    int         damage_done_reduction_percent;
    int         hit_point_regen_percent;
    int         purified_armor_percent;
    int         execute_reset_percent;
    int         recharge_speed;
    int         armor_penetration;

    int         elemental_crit[ num_damage_types ];

    int         riposte;
    int         damage_taken_reduction_percent;
    int         life_steal_percent;                 // percent of damage to heal

    // future expansion
    dword       expansion_stats[7];

    // class ancient flags
    dword       bard_flags;
    dword       cleric_flags;
    dword       enchanter_flags;
    dword       necro_flags;
    dword       paladin_flags;
    dword       rogue_flags;
    dword       warrior_flags;
    dword       wizard_flags;

    // for armor sets
    dword       aspire_set;
    dword       cosmic_set;
    dword       regen_set;
    dword       holiday_set;
    dword       future_sets[15];
};
```

### struct unit (lines 425-485, video 11:19-12:50)

```c
#define unit_data_version               1

struct unit
{
    dword           unit_version;

    dword           id;

    safestring      name;
    dword           flags;                  // defined above

    int             unit_type;              // type = player, monster, item, etc
    int             unit_class;             // specific class within the type
    int             unit_mode;

    dword           model_id;

    anim_data *     anim;
    anim_data *     default_anim;
    int             anim_delay;
    int             anim_rate;
    int             anim_frame;
    bool            anim_new_frame;
    vector2d        wand_anim_offset;
    float           wand_anim_angle;
    float           wand_anim_delta;
    float           anim_speed_mod;

    unit_stats      stats;
    int             level;

    dword           world_id;
    vector2d        position;
    vector2d        unwrapped_position;
    vector2d        movement;
    vector2d        forces;
    vector2d        facing;
    float           velocity;
    float           gravity;

    int             collision_shape;
    box             aabb;
    float           width;
    float           height;
    float           radius;

    float           light_radius;
    float           light_intensity;

    dword           emitter_ids[ max_emitters_per_unit ];

    dword           buffdebuff_list;

    unit *          dungeon_next;
    unit *          type_previous;
    unit *          type_next;
    unit *          team_next;

    int             data_version;
    byte            custom_data[ unit_custom_data_size ];
};
```

`max_emitters_per_unit` is 16 and `unit_custom_data_size` is 4096 (defined above). `custom_data` holds the per-type
struct; for items the code does `item_custom_data * icd = ( item_custom_data * )&item->custom_data[0];`.

### struct unit_save_data_v0 (lines 487-497, partial - the rest is never scrolled into view)

```c
//-----------------------------------------------------------------------------
// save structures

struct unit_save_data_v0
{
    dword       unit_version;
    dword       id;
    int         unit_type;
    int         unit_class;
    int         unit_mode;
    dword       unit_flags;
    // ... continues off screen
```

## g_items.cpp

### Includes (lines 1-33, video 14:18; list continues off screen)

```c
#include "main.h"
#include "g_items.h"
#include "g_unique_items.h"
#include "g_blueprint.h"
#include "gge_main.h"
#include "ec_fmod.h"
#include "g_dungeon.h"
#include "g_dungeon_internal.h"
#include "g_rlg.h"
#include "g_player.h"
#include "g_player_inventory.h"
#include "g_ui.h"
#include "g_ui_selection.h"
#include "g_ui_messages.h"
#include "g_ui_popup.h"
#include "g_ui_vendor.h"
#include "g_ui_banker.h"
#include "g_ui_crafting.h"
#include "g_ui_altar.h"
#include "g_ui_survival.h"
#include "g_ui_refrigerator.h"
#include "g_ui_ancients.h"
#include "g_objects.h"
#include "g_buffdebuff.h"
#include "g_quests.h"
#include "g_emitters.h"
#include "g_particles.h"
```

### Rarity and colours (lines 48-71, video 14:20)

```c
#define item_level_needed_to_use_delta          15

enum
{
    rarity_normal = -1,
    rarity_common = 0,
    rarity_uncommon,
    rarity_rare,
    rarity_ultra,
    rarity_unique,
};

#define color_common        make_color( 74, 148, 4 )
#define color_uncommon      make_color( 2, 74, 202 )
#define color_rare          make_color( 131, 2, 202 )
#define color_ultra         make_color( 232, 122, 23 )
#define color_cursed        make_color( 247, 59, 43 )
#define color_ancient       make_color( 195, 11, 236 )
#define color_wand          make_color( 128, 128, 128 )
#define color_unique        make_color( 187, 163, 123 )
#define color_set           color_yellow
#define color_cosmetic      color_cyan
#define color_pet           make_color( 0, 255, 175 )
#define color_char_bound    make_color( 243, 237, 131 )

//-----------------------------------------------------------------------------
// enchantments

enum
{
    enchant_armor_1,
    enchant_armor_2,
    enchant_armor_3,
    // ... continues off screen
```

### Item excel table (lines 175-188; only the tail of the table is shown)

Columns are: parser macro, column name in the sheet, struct, field, then a default value or link table.

```c
    excel_data_int(     "max_damage",       item_definition_data,   max_damage,         0                   ),
    excel_data_int(     "atk_rate",         item_definition_data,   attack_rate,        0                   ),
    excel_data_int(     "compost_cost",     item_definition_data,   compost_cost,       0                   ),
    excel_data_int(     "gold_cost",        item_definition_data,   cost,               0                   ),
    excel_data_int(     "sell_percent",     item_definition_data,   sell_percent,       0                   ),
    excel_data_bool(    "can_trash",        item_definition_data,   can_trash                               ),
    excel_data_link(    "use_sound",        item_definition_data,   use_sound_id,       sound_definitions   ),
    excel_data_link(    "inv_sound",        item_definition_data,   inv_sound_id,       sound_definitions   ),
    excel_data_int(     "gfx_offset",       item_definition_data,   gfx_look,           0                   ),
    excel_data_float(   "weapon_end_x",     item_definition_data,   weapon_end_x                            ),
    excel_data_float(   "weapon_end_y",     item_definition_data,   weapon_end_y                            ),
};

static excel_parse_table item_excel_data = excel_table_define( "xls\\items.txt", "xls\\items.gbe", item_definition_list, item_definitions, num_item_definitions );
```

So `item_definition_data` has at least: `max_damage`, `attack_rate`, `compost_cost`, `cost`, `sell_percent`,
`can_trash`, `use_sound_id`, `inv_sound_id`, `gfx_look`, `weapon_end_x`, `weapon_end_y`, `is_mushroom`.

### Custom unit data defines (lines 190-218, video 14:21)

```c
// custom unit data, defines and type list

#define item_radius             0.45f

#define min_spawn_velocity      0.01f
#define max_spawn_velocity      0.05f
#define spawn_jump_velocity     -0.2f
#define vaccuum_velocity        0.05f

#define min_spawn_life          ( game_framerate / 2 )

#define max_ticks_on_ground     ( game_framerate * 600 )

#define max_wand_affixes                5

#define max_enchant_affixes             4

#define item_data_version               9

#define item_flag_player_dropped        0x0001
#define item_flag_has_cooldown          0x0002
#define item_flag_in_vault              0x0004
#define item_flag_character_bound       0x0010

#define num_item_zero_expansion         25
#define num_item_invalid_expansion      25

#define max_item_bounces        5
```

### struct item_custom_data (lines 220-285, video 14:22-15:30)

```c
struct item_custom_data
{
    bool        identified;

    bool        in_world;
    bool        never_decay;
    int         on_ground_count;

    int         bounces;
    int         stack_count;
    int         spawn_life;

    // item level and quality variables
    int         item_quality;
    dword       prefix_string_id;
    dword       suffix_string_id;
    dword       rare_string_id;
    dword       unique_string_id;
    dword       delivery_string_id;
    dword       unidentified_string_id;
    dword       affix_description_id;

    // wands
    int         weapon_delivery_type;
    int         wand_charges;
    int         wand_recharge_ticks;

    // crystals
    dword       crystal_id;
    int         crystal_shards;

    // armor
    int         slot_shape;

    // mushrooms
    dword       mushroom_type;

    // gold
    int         gold;
    int         cost;
    int         buyback_price;

    // ancient items
    int         cooldown_ticks;
    bool        can_upgrade;

    // food
    int         eat_amount;
    int         max_stack;

    // new/expanded data placed below here

    // a series of data initialized to 0
    dword       item_flags;
    dword       total_slevel;
    byte        ancient_tree[ max_ancient_tree_nodes ];
    int         level_required;
    dword       zero_expansion[ num_item_zero_expansion ];

    // a series of data initialized to -1
    dword       original_slot;
    dword       glow_id;
    dword       next_dropped;
    dword       enchant_id[ max_enchant_affixes ];
    dword       invalid_expansion[ num_item_invalid_expansion ];
};

static unit * item_active_list = NULL;
static int num_active_items = 0;
static unit * item_inactive_list = NULL;
static int num_inactive_items = 0;
static unit * item_delete_list = NULL;

static int ancient_ranking_gold_tbl[ max_ancient_ranks ] = { 500, 5000, 50000, 100000, 150000, 250000, 350000, 500000 };

//-----------------------------------------------------------------------------
// used to keep track of id scroll unit while ID'ing an item

static dword identify_scroll_id = invalid_id;
```

`max_ancient_ranks` is 9 and `max_ancient_tree_nodes` is 16 (see `g_items.h` below); the gold table lists only 8
values, so its last entry is zero-initialised.

### Functions shown in g_items.cpp

```c
// line 3317 - tail of item_set_required_level( unit * item ), video 23:30
    item_custom_data * icd = ( item_custom_data * )&item->custom_data[0];
    assert_return( icd );

    icd->level_required = 0;

    // should I do this?
    bool needs_level_req = false;
    if ( item_definitions[ item_class ].weapon_type != weapon_type_none ) needs_level_req = true;
    if ( ( item_definitions[ item_class ].equipment_slot >= equipment_slot_head ) && ( item_definitions[ item_class ].equipment_slot <= equipment_slot_ring_25 ) ) needs_l... /* off screen */
    if ( item_definitions[ item_class ].crystal_shape != crystal_shape_none ) needs_level_req = true;

    if ( !needs_level_req ) return;

    int ilevel = 0;
    if ( ilevel == 0 ) ilevel = item_class_get_armor_ilevel( item_class );
    if ( ilevel == 0 ) ilevel = item->level;
    icd->level_required = ilevel - item_level_needed_to_use_delta;
    if ( icd->level_required < 0 ) icd->level_required = 0;
}

// line 3339
int item_get_level_req( unit * item )
{
    assert_return_zero( item );
    assert_return_zero( item->unit_type == unit_type_item );

    item_custom_data * icd = ( item_custom_data * )&item->custom_data[0];
    assert_return_zero( icd );

    int highest_req = icd->level_required;

    if ( icd->crystal_id != invalid_id )
    // ... off screen

// line 5166 - end of the previous function
    channel_play( sound_ui_click );

    return true;
}

// line 5174 (lines 5183-5185 are new in this video)
static void mushroom_identify( unit * item )
{
    assert_return( item );
    assert_return( item->unit_type == unit_type_item );

    int ic = item->unit_class;
    assert_return( ic >= 0 );
    assert_return( ic < num_item_definitions );
    assert_return( item_definitions[ic].is_mushroom );

    int mushroom_index = item_class_get_mushroom_index( item->unit_class );
    player_identify_mushroom_color( mushroom_index );
}

// line 5190
bool item_identify( unit * item )
{
    assert_return_false( item );
    assert_return_false( item->unit_type == unit_type_item );

    int ic = item->unit_class;
    assert_return_false( ic >= 0 );
    assert_return_false( ic < num_item_definitions );

    item_custom_data * icd = ( item_custom_data * )&item->custom_data[0];
    assert_return_false( icd );
    // ... off screen

// line 13800 - tail of the item save function (saves the whole item_custom_data as-is)
    assert_return_zero( save_data );
    assert_return_zero( item );
    assert_return_zero( item->unit_type == unit_type_item );

    item_custom_data * icd = ( item_custom_data * )&item->custom_data[0];
    assert_return_zero( icd );

    item_custom_data * item_save = ( item_custom_data * )save_data;
    assert_return_zero( item_save );
    *item_save = *icd;

    return sizeof( item_custom_data );
}

// line 13816
bool item_load( byte * load_data, unit * item, dword * load_size )
{
    assert_return_false( load_data );
    assert_return_false( item );
    assert_return_false( item->unit_type == unit_type_item );

    item_init_common( item );

    item_custom_data * icd_load = ( item_custom_data * )load_data;
    assert_return_false( icd_load );

    item_custom_data * icd = ( item_custom_data * )&item->custom_data[0];
    assert_return_false( icd );

    *icd = *icd_load;

    if ( icd->in_world )
    // ... lines 13833-13863 off screen
    *load_size = sizeof( item_custom_data );

    return true;
}

// line 13871
static void item_do_id_remap( unit * item )
{
    assert_return( item );

    item_custom_data * icd = ( item_custom_data * )&item->custom_data[0];
    assert_return( icd );

    icd->crystal_id = unit_get_remap_id( icd->crystal_id );
}

// line 13883
void item_id_remap( dword item_id )
{
    assert_return( item_id != invalid_id );

    unit * item = unit_from_id( item_id );
    assert_return( item );
    assert_return( item->unit_type == unit_type_item );

    item_do_id_remap( item );
}

// line 13896
void items_remap_ids()
{
    unit * item = item_active_list;
    while ( item )
    {
        if ( !item_is_in_vault( item ) ) item_do_id_remap( item );
        item = item->type_next;
    }
    item = item_inactive_list;
    while ( item )
    {
        if ( !item_is_in_vault( item ) ) item_do_id_remap( item );
        item = item->type_next;
    }
}

// line 13914
static void upgrade_v1_to_v2( unit * item )
{
    assert_return( item );
    assert_return( item->unit_type == unit_type_item );

    item_custom_data * icd = ( item_custom_data * )&item->custom_data[0];
    assert_return( icd );

    icd->total_slevel = 0;

    // remove double jump from items
    if ( item->stats.attribute_flags & attribute_flag_double_jump )
    {
        item->stats.attribute_flags &= ~attribute_flag_double_jump;
        item->stats.attribute_flags |= attribute_flag_triple_jump;
    }

    if ( item->unit_class == item_type_adamantine_pickaxe )
        item->stats.attribute_flags |= attribute_flag_no_blocks;

    // init food slevel
    if ( item_definitions[ item->unit_class ].feed > 0 )
    {
        item->level = player_get_survival_rank();
    }

    item->data_version = 2;
}

// line 13945
static void upgrade_v2_to_v3( unit * item )
{
    assert_return( item );
    assert_return( item->unit_type == unit_type_item );

    item_custom_data * icd = ( item_custom_data * )&item->custom_data[0];
    assert_return( icd );

    item_init_feed_amount( item );

    item->data_version = 3;
}

// line 13960: static void upgrade_v3_to_v4( unit * item ) - body off screen

// line 13992 - tail of upgrade_v4_to_v5
        icd->max_stack = item_definitions[ item->unit_class ].stack_count;
    }

    // recalculate wand recharge times
    if ( ( item_definitions[ item->unit_class ].weapon_type == weapon_type_wand ) || ( item_definitions[ item->unit_class ].weapon_type == weapon_type_staff ) )
    {
        calculate_wand_recharge( item );
        if ( icd->prefix_string_id == string_token_wand_prefix_heavy )
            icd->wand_recharge_ticks = ( icd->wand_recharge_ticks * 125 ) / 100;
        if ( icd->prefix_string_id ==  string_token_wand_prefix_light )
            icd->wand_recharge_ticks = ( icd->wand_recharge_ticks * 2 ) / 3;
        if ( icd->prefix_string_id == string_token_wand_prefix_brutal )
            icd->wand_recharge_ticks = ( icd->wand_recharge_ticks * 3 ) / 2;
    }

    item->data_version = 5;
}

// line 14012
static void upgrade_v5_to_v6( unit * item )
{
    assert_return( item );
    assert_return( item->unit_type == unit_type_item );

    if ( item->stats.attribute_flags & attribute_flag_boring )
    {
        item->stats.attribute_flags &= ~attribute_flag_boring;
        item->stats.armor_penetration = magic_bonus( 10, 25, item->level );
    }

    item->data_version = 6;
}

// line 14056 - tail of upgrade_v7_to_v8
        icd->cost = item_definitions[ item->unit_class ].cost;
    }

    item->data_version = 8;
}

// line 14065 (the mushroom block, lines 14087-14102, is what is being written in this video)
static void upgrade_v8_to_v9( unit * item )
{
    assert_return( item );
    assert_return( item->unit_type == unit_type_item );

    item_custom_data * icd = (item_custom_data * )& item->custom_data[ 0 ];
    assert_return( icd );

    // set crafted armor ilevels
    int ilevel = item_class_get_armor_ilevel( item->unit_class );
    if ( ilevel != 0 ) item->level = ilevel;

    // cosmic, aspire and holiday are special case
    if ( item->stats.aspire_set || item->stats.cosmic_set || item->stats.holiday_set )
    {
        int ilevel = player_get_level();
        item->level = ilevel;
    }

    item_set_required_level( item );
    item_set_character_bound( item );

    if ( item_definitions[ item->unit_class ].is_mushroom )
    {
        int mushroom_index = item_class_get_mushroom_index( item->unit_class );
        dword mushroom_info = player_get_mushroom_type_info( mushroom_index );
        if ( mushroom_info & mushroom_type_identified )
        {
            icd->prefix_string_id = string_token_mushroom_brown + mushroom_index;
            icd->rare_string_id = string_token_mushroom_poison + ( mushroom_info & mushroom_type_mask );
        }
        else
        {
            icd->identified = false;
            icd->prefix_string_id = string_token_object_unknown_mushroom;
        }
        icd->mushroom_type = mushroom_info & mushroom_type_mask;
    }

    item->data_version = 9;
}

// line 14109
void items_finish_load()
{
    unit * item = item_inactive_list;
    while ( item != NULL )
    {
        unit * next = item->type_next;
        if ( item->data_version == 1 ) upgrade_v1_to_v2( item );
        if ( item->data_version == 2 ) upgrade_v2_to_v3( item );
        if ( item->data_version == 3 ) upgrade_v3_to_v4( item );
        if ( item->data_version == 4 ) upgrade_v4_to_v5( item );
        if ( item->data_version == 5 ) upgrade_v5_to_v6( item );
        if ( item->data_version == 6 ) upgrade_v6_to_v7( item );
        // ... off screen
```

Save format takeaways: an item is saved as a raw copy of `item_custom_data` (`sizeof( item_custom_data )` bytes),
`unit.data_version` holds the item version (currently `item_data_version` 9), and old saves are migrated one version
at a time in `items_finish_load()`.

## g_items.h (video 22:05-22:35; the file is scrolled quickly, gaps are marked)

```c
//-----------------------------------------------------------------------------
// ShooterRPG
// by David Brevik
// Graybeard Games
// (C) 2016, Copyright 2016, All rights reserved.

#ifndef _G_ITEMS_H_
#define _G_ITEMS_H_

//-----------------------------------------------------------------------------
// defines

#define item_icon_texture_icon_scale    16.0f

#define crafted_ilevel                  3
#define stat_scaling_ilevel             50              // many of the items are based off the old max_ilevel of 50. To scale the stats properly and keep items how they a... /* off screen */
#define max_ilevel                      100

#define item_sound_volume               0.75f

#define max_ancient_ranks               9
#define max_ancient_tree_nodes          16

#define ancient_rarity_mask             0x3
#define ancient_rarity_nightmare        0x8

#define max_coffee_stacks               5

//-----------------------------------------------------------------------------
// magic drop rates
#define unique_percent          25          // 0% - 0.25% (1/4%)
#define ultra_percent           100         // 1/4% - 1% (3/4%)
#define rare_percent            700         // 2% - 7% (5%)
#define uncommon_percent        2000        // 8% - 20% (12%)
#define common_percent          5000        // 26% - 50% (30%)

enum
{
    item_quality_normal = 0,
    item_quality_cursed,
    item_quality_common,
    item_quality_uncommon,
    item_quality_rare,
    item_quality_ultra,
    item_quality_unique,
    item_quality_ancient,
    item_quality_set,

    num_item_qualities
};

enum
{
    crystal_shape_none = 0,
    crystal_shape_bar,
    crystal_shape_triangle,
    crystal_shape_square,
    crystal_shape_diamond,
    crystal_shape_octagon,

    num_crystal_shapes
};

enum
{
    season_currency_halloween = 0,
    season_currency_harvest,
    season_currency_winter,

    num_season_currencies
};

enum
{
    currency_type_gold = 0,
    currency_type_seasonal,
    currency_type_compost,

    num_currency_types
};

enum
{
    reveal_group_starting_all = 0,
    reveal_group_initial_tools = 1,
    reveal_group_forge = 2,
    reveal_group_pre_gatekeeper = 3,
    reveal_group_post_gatekeeper = 4,

    reveal_group_copper = 5,
    reveal_group_mithril = 6,
    reveal_group_rubium = 7,
    reveal_group_gold = 8,
    reveal_group_adamantine = 9,
    reveal_group_verdant = 10,
    reveal_group_cobalt = 11,
    reveal_group_crimson = 12,
    reveal_group_ash = 13,
    reveal_group_nether = 14,
    reveal_group_gem_green = 15,
    reveal_group_gem_blue = 16,
    reveal_group_gem_purple = 17,
    reveal_group_gem_orange = 18,
    // creative mode stuff
    reveal_group_tools_normal = 40,
    reveal_group_tools_creative,
    reveal_group_hoe_creative,
    reveal_group_blocks_normal = 50,
    reveal_group_blocks_creative,
    // recipes
    reveal_group_chef_blueprint = 103,
    reveal_group_bank_blueprint = 104,
    reveal_group_runes = 105,
    reveal_group_boss_1 = 106,
    // ... lines 115-123 not captured
    reveal_group_gold_fertilizer = 119,
    reveal_group_adamantine_pickaxe = 120,
    reveal_group_potion_lesser_heal = 121,
    reveal_group_potion_minor_heal = 122,
    reveal_group_potion_healing = 123,
    reveal_group_potion_major_heal = 124,
    reveal_group_bar_crystal_bulb = 125,
    reveal_group_tri_crystal_bulb = 126,
    reveal_group_square_crystal_bulb = 127,
    reveal_group_diamond_crystal_bulb = 128,
    reveal_group_octagon_crystal_bulb = 129,
    reveal_group_nickel_ore_bulb = 130,
    reveal_group_lodestone_ore_bulb = 131,
    reveal_group_ochre_ore_bulb = 132,
    reveal_group_russet_ore_bulb = 133,
    reveal_group_money_tree_bulb = 134,
    reveal_group_starting_wand = 135,
    reveal_group_improved_wand = 136,
    reveal_group_starting_sword = 137,
    reveal_group_improved_sword = 138,
    reveal_group_composter = 139,
    reveal_group_refrigerator = 140,
    reveal_group_starting_dual = 141,
    reveal_group_improved_dual = 142,
    reveal_group_starting_mace = 143,
    reveal_group_improved_mace = 144,
    reveal_group_graveyard = 145,
    reveal_group_cooked_meat_chunk = 146,
    reveal_group_bronzed_ore_bulb = 147,
    reveal_group_vibrant_ore_bulb = 148,
    reveal_group_spring = 149,
    reveal_group_adv_spring = 150,
    reveal_group_summer = 151,
    // ... lines 157-243 not captured (ends with a melee delivery enum:)
    melee_dt_projectile,

    num_melee_delivery_types
};

// line 249
enum
{
    mushroom_type_poison = 0,
    mushroom_type_blindness,
    mushroom_type_confusion,
    mushroom_type_cure_all,
    mushroom_type_feast,
    mushroom_type_first_aid,
    mushroom_type_meal,
    mushroom_type_snack,

    mushroom_type_badlands_light,
    mushroom_type_badlands_hps,
    mushroom_type_badlands_levitate,
    mushroom_type_badlands_magic,
    mushroom_type_badlands_burning,
    mushroom_type_badlands_greater_poison,
    mushroom_type_badlands_poison_resist,
    mushroom_type_badlands_fire_resist,

    num_mushroom_types
};

#define num_original_mushroom_types             ( mushroom_type_badlands_light )

enum
{
    mushroom_color_brown = 0,
    mushroom_color_yellow,
    mushroom_color_green,
    mushroom_color_purple,
    mushroom_color_cyan,
    mushroom_color_blue,
    mushroom_color_white,
    mushroom_color_black,

    mushroom_color_badlands_red,
    mushroom_color_badlands_orange,
    mushroom_color_badlands_forest,
    mushroom_color_badlands_sage,
    mushroom_color_badlands_brown,
    mushroom_color_badlands_darkpink,
    mushroom_color_badlands_gray,
    mushroom_color_badlands_black,

    num_mushroom_colors
};

#define num_original_mushroom_colors            ( mushroom_color_badlands_red )

#define mushroom_type_identified    0x80000000
#define mushroom_type_mask          0x0000ffff

enum    // line 302, contents off screen
```

`item_custom_data.mushroom_type` and `player_get_mushroom_type_info()` use `mushroom_type_identified` as the top bit
and `mushroom_type_mask` for the type index.

## g_objects.cpp (video 20:23, 21:48)

```c
// line 4646 - tail of the previous function
    while ( object )
    {
        unit * next = object->type_next;
        if ( object != selected ) object_delete( object );
        object = next;
    }
}

// line 4654 (inside an #if block, closed by #endif on line 4692)
void object_spawn_debug()
{
    int object_class = object_type_mushroom_bl_red;

    ivector2d cursor_pos;
    cursor_get_position( &cursor_pos.x, &cursor_pos.y );

    vector2d world_pos;
    view_to_dungeon( &cursor_pos, &world_pos, true );

    int sx = itruncf( world_pos.x );
    int sy = itruncf( world_pos.y ) - 1;

    vector2d start;
    start.x = (float)sx + 0.5f;
    start.y = (float)sy + 1.0f - epsilon;

    float width = object_definitions[ object_class ].world_width;
    float height = object_definitions[ object_class ].world_height;

    bool okay = false;
    while ( !okay )
    {
        dword result = dungeon_test_box_area( world_main, start, width, height );
        if ( result == boxcollide_hit_none )
            okay = true;
        else
            start.y += 1.0f;
    }

    dword debug_id = object_create( world_main, start, object_class );
    assert_return( debug_id != invalid_id );
    unit * object = unit_from_id( debug_id );
    object_custom_data * ocd = ( object_custom_data * )&object->custom_data[0];
    assert_return( ocd );
    ocd->obj_data = item_type_bar_crystal;
    ocd->object_flags |= object_flag_normal_dirt;
}
#endif
```

`object_custom_data` therefore has at least `obj_data` and `object_flags` fields.

## g_dungeon.cpp - badlands_spawn_mushrooms() (lines 4865-4892, video 20:10)

```c
    {
        int y_min = ( z * zone_height );
        if ( z == 0 ) y_min = 2;
        //int y_max = y_min + zone_height;
        for ( int m = 0; m < num_mushrooms_per_zone; m++ )
        {
            int x = 0;
            int y = 0;
            bool found = false;
            while ( !found )
            {
                x = random( irlg_width );
                y = random( zone_height ) + y_min;
                if ( world_list[ world_badlands ].dungeon[ y ][ x ].tile[ layer_foreground ] == rlg_tile_empty )
                {
                    while ( world_list[ world_badlands ].dungeon[ y + 1 ][ x ].tile[ layer_foreground ] == rlg_tile_empty ) y++;
                    if ( world_list[ world_badlands ].dungeon[ y ][ x ].list == NULL ) found = true;
                    if ( world_list[ world_badlands ].dungeon[ y + 1 ][ x ].tile[ layer_foreground ] >= num_rlg_tiles ) found = false;
                }
            }

            vector2d spawn_loc;
            spawn_loc.x = (float )x + 0.5f;
            spawn_loc.y = (float )y + 1.0f - epsilon;
            int mushroom_obj_type = picker_choose() + object_type_mushroom_bl_red;
            object_create( world_badlands, spawn_loc, mushroom_obj_type );
        }
    }
}

//-----------------------------------------------------------------------------
// Upgrade dungeon to add gems for enchanting (run once only, ever)
//-----------------------------------------------------------------------------

static void gem_patch_picker_add( int x, int y )
{
    if ( y < 0 ) return;
    if ( y >= irlg_height ) return;
    int xx = x & irlg_width_mask;
    // ... off screen
```

World layout hints: `world_list[ world ].dungeon[ y ][ x ]` cells have `.tile[ layer ]` and a `.list` unit pointer;
worlds seen are `world_main` and `world_badlands`.

## strings.xls (Excel, video 20:34-23:20)

Columns: A = string token (used in code as `string_token_<name>`, generated into `strings_hdr.h`), B = string id,
C = text. In text, `#n` is a line break and `#y` / `#w` look like colour codes (yellow / white) - that is my reading,
not stated in the video. Rows not listed were not on screen. `?` = token hidden behind the webcam or a dialog.

### Rows 9-38 (menus, character select / create)

| Row | Token | Id | Text |
|---|---|---|---|
| 9 | console_error | 6 | "ERROR: Unknown command " |
| 10 | console_error_param | 7 | "ERROR: Incorrect parameter" |
| 11 | beta_closed | 8 | The Beta is currently: OFFLINE |
| 12 | beta_closed_thanks | 9 | Thanks for playing. It will be available again soon. |
| 13 | mm_play | 10 | Play |
| 14 | mm_news | 11 | Patch Notes |
| 15 | mm_new_news | 12 | New! |
| 16 | mm_community | 3235 | Community |
| 17 | mm_options | 13 | Options |
| 18 | mm_exit | 14 | Exit |
| 19 | news_next | 15 | Older |
| 20 | news_back | 16 | Back |
| 21 | cs_new | 17 | Create New Character |
| 22 | cs_back | 18 | Back |
| 23 | cs_title | 19 | Select Character or Create New |
| 24 | cs_level | 20 | "Level " |
| 25 | cs_dead | 21 | "Dead at Level " |
| 26 | cs_game_style | 22 | "Game Style: " |
| 27 | cs_hardcore_error | 23 | You can't load a dead hardcore character |
| 28 | cs_confirm | 1860 | Are you sure you want to delete this character? |
| 29 | cs_version | 24 | Old save file version (Beta character wipe) |
| 30 | cc_character_race | 3309 | Choose Race |
| 31 | cc_race_human | 3310 | Human |
| 32 | cc_race_human_desc | 3311 | Humans get extra Magic Find and better Resistances from Vigor |
| 33 | cc_race_ogre | 3312 | Ogre |
| 34 | cc_race_ogre_desc | 3313 | Ogre gets extra Armor, Melee attack rate increase, Wand attack rate decrease, Jump higher, fall slower |
| 35 | cc_hair | 25 | Hair Style |
| 36 | cc_hair_color | 26 | Hair Color |
| 37 | cc_head | 27 | Head |
| 38 | cc_eye_color | 28 | Eye Color |
| 39 | ? | 29 | Skin Color |
| 40 | ? | 30 | Accessories |
| 41 | ? | 31 | Chest |
| 42 | ? | 32 | Choose Class |
| 43 | ? | 33 | Randomize |
| 44 | ? | 34 | Enter Name |
| 45 | ? | 35 | Your name must be between one and 16 letters |

Row 1 is the header (column A starts `string_`); rows 2-8 are `game_*` / `console_*` tokens hidden by the Find dialog
(visible fragments: "... Graybeard Games. All Rights Reserved.", "... Activated. Type 'Help' for help.").

### Rows 388-447 (objects and mushrooms)

| Row | Token | Id | Text |
|---|---|---|---|
| 388 | object_ore_plant | 1445 | Ore Lode |
| 397 | object_small_door | 215 | Small Door |
| 398 | object_shrine | 216 | Shrine |
| 399 | object_altar | 217 | Altar |
| 400 | object_secret_portal | 218 | Dimensional Portal |
| 401 | object_boss_door | 1822 | Dimensional Door |
| 402 | object_story_stand | 3227 | A page of notes |
| 403 | object_badlands_portal | 3277 | Badlands Portal |
| 404 | object_prickly_pear_bush | 3289 | Prickly Pear Cactus |
| 405 | story_new_page | 3228 | You have discovered a new page of notes! |
| 406 | story_new_page_help | 3229 | "View the found pages by pressing: '" |
| 407 | story_old_page | 3233 | I seem to already have these notes... |
| 408 | object_halloween_door | 1823 | Pumpkin Festival |
| 409 | object_harvest_door | 1938 | Harvest Festival |
| 410 | object_unknown_mushroom | 219 | Unknown |
| 411 | object_mushroom | 220 | Mushroom |
| 412 | mushroom_brown | 221 | Sorrel Mushroom |
| 413 | mushroom_yellow | 222 | Mustard Mushroom |
| 414 | mushroom_green | 223 | Verdant Mushroom |
| 415 | mushroom_purple | 224 | Periwinkle Mushroom |
| 416 | mushroom_cyan | 225 | Turquoise Mushroom |
| 417 | mushroom_blue | 226 | Royal Mushroom |
| 418 | mushroom_white | 227 | Day Mushroom |
| 419 | mushroom_black | 228 | Night Mushroom |
| 420 | mushroom_poison | 229 | of Poison |
| 421 | mushroom_blindness | 230 | of Blindness |
| 422 | mushroom_confusion | 231 | of Confusion |
| 423 | mushroom_cure_all | 232 | of Curing |
| 424 | mushroom_sustenance | 233 | Feast |
| 425 | mushroom_first_aid | 234 | of First Aid |
| 426 | mushroom_food | 235 | Meal |
| 427 | mushroom_snack | 236 | Snack |
| 428 | mushroom_bl_red | 3452 | Red Rust Mushroom |
| 429 | mushroom_bl_orange | 3453 | Bleeding Orange Mushroom |
| 430 | mushroom_bl_forest | 3454 | Green Scaled Mushroom |
| 431 | mushroom_bl_sage | 3455 | Touch of Spring Mushroom |
| 432 | mushroom_bl_brown | 3456 | Ogre Toe Mushroom |
| 433 | mushroom_bl_darkpink | 3457 | Ogre's Kiss Mushroom |
| 434 | mushroom_bl_gray | 3458 | Rock Love Mushroom |
| 435 | mushroom_bl_black | 3459 | Starlight Mushroom |
| 436 | mushroom_light | 3460 | of Light |
| 437 | mushroom_hps | 3461 | of Giants |
| 438 | mushroom_levitate | 3462 | of Levitation |
| 439 | mushroom_magic | 3463 | of Magic |
| 440 | mushroom_burning | 3464 | of Burning |
| 441 | ? | 3465 | of Ogre Poison |
| 442 | ? | 3466 | of Poison Resist |
| 443 | ? | 3467 | of Fire Resist |
| 444 | ? | 2121 | Breaded Mushrooms |
| 445 | ? | 2122 | Garlic Mushrooms |
| 446 | ? | 2123 | Spicy Mushrooms |
| 447 | ? | 2125 | Feeds and gives you 10 Poison and Fire Resist Rating for 15 minutes |

The token order for colours and effects matches the `mushroom_color_*` and `mushroom_type_*` enums, which is what
`string_token_mushroom_brown + mushroom_index` and `string_token_mushroom_poison + type` rely on.

### Rows 933-970 (items)

| Row | Token | Id | Text |
|---|---|---|---|
| 933 | item_vibrant_ore | 1694 | Vibrantium Ore |
| 934 | item_vibrant_bar | 1695 | Vibrantium Bar |
| 935 | item_ash_ore | 2762 | Ash Ore |
| 936 | item_ash_bar | 2763 | Ash Bar |
| 937 | item_aqua_ore | 2764 | Aqua Ore |
| 938 | item_aqua_bar | 2765 | Aqua Bar |
| 939 | item_nether_ore | 2766 | Nether Ore |
| 940 | item_nether_bar | 2767 | Nether Bar |
| 941 | item_sunrise_ore | 2768 | Sunrise Ore |
| 942 | item_sunrise_bar | 2769 | Sunrise Bar |
| 943 | item_forge | 485 | Portable Forge |
| 944 | item_anvil | 486 | Anvil |
| 945 | item_workbench | 487 | Handy Workbench |
| 946 | item_bed | 488 | Soft Bed |
| 947 | item_vault | 1428 | Quick Vault |
| 948 | item_vault_desc | 1429 | Gain access to your bank for 1 minute |
| 949 | item_homestone | 489 | Homestone |
| 950 | homestone_buff | 3242 | Homestone Aura#n#nStanding near the Homestone#nslows your digestion |
| 951 | item_refrigerator | 1457 | Refrigerator |
| 952 | ui_fridge_food_only | 1504 | Only food, crops and seeds can be placed in the refrigerator |
| 953 | item_composter | 1458 | Composter |
| 954 | item_bookcase | 3321 | Enchanting Station |
| 955 | item_enchant_book_empty | 3334 | Empty Enchantment Tome |
| 956 | item_enchant_book_green | 3322 | Beginner Enchantment Tome |
| 957 | enchant_book_green_desc | 3323 | Enchant Items from iLevel 1 to iLevel 35 |
| 958 | item_enchant_book_blue | 3324 | Intermediate Enchantment Tome |
| 959 | enchant_book_blue_desc | 3325 | Enchant Items from iLevel 36 to iLevel 65 |
| 960 | item_enchant_book_purple | 3326 | Advanced Enchantment Tome |
| 961 | enchant_book_purple_desc | 3327 | Enchant Items from iLevel 66 to iLevel 90 |
| 962 | item_enchant_book_orange | 3328 | Master Enchantment Tome |
| 963 | enchant_book_orange_desc | 3329 | Enchant Items from iLevel 91 to iLevel 100 |
| 964 | item_gem_green | 3330 | Emerald |
| 965 | item_gem_blue | 3331 | Saphire |
| 966 | item_gem_purple | 3332 | Amethyst |
| 967 | item_gem_orange | 3333 | Carnelian |
| 968 | item_iron_helm | 490 | Iron Skullcap |
| 969 | item_iron_breastplate | 491 | Iron Mail |
| 970 | item_iron_leggings | 492 | Iron Leggings |
| 971 | ? | 493 | Iron Boots |
| 972 | ? | 494 | Copper Helm |
| 973 | ? | 495 | Copper Breastplate |
| 974 | ? | 496 | Copper Greaves |
| 975 | ? | 497 | Copper Boots |
| 976 | ? | 498 | Mithril Helmet |
| 977 | ? | 499 | Mithril Chestguard |

### Rows 2127-2171 (ancients, blueprints, world books)

| Row | Token | Id | Text |
|---|---|---|---|
| 2127 | item_ancient_bat | 1826 | Gone Batty |
| 2128 | ancient_desc_bat | 1827 | Illusion yourself into a Bat and fly around.#nAttacking, taking damage, or mining ends the enchantment |
| 2129 | item_ancient_unbind_key | 3487 | Mythical Key of Ancient Unbinding |
| 2130 | ancient_unbind_key_desc | 3488 | Use this key to unbind an ancient item and allow it to placed in the Shared Vault |
| 2131 | item_blueprint_planter_box | 2516 | Blueprint: Planter Box |
| 2132 | item_blueprint_chef | 817 | Blueprint: General Store |
| 2133 | item_chef_recipe | 818 | Recipe: General Store |
| 2134 | item_chef_recipe_desc | 819 | Learn the blueprint for making a general vendor |
| 2135 | item_blueprint_banker | 820 | Blueprint: Trusty Bank |
| 2136 | item_banker_recipe | 821 | Recipe: Trusty Bank Blueprint |
| 2137 | item_banker_recipe_desc | 822 | Learn the blueprint for making a bank |
| 2138 | item_blueprint_wizard | 823 | Blueprint: Wizard of Space and Time |
| 2139 | item_blueprint_hats | 824 | Blueprint: The Hat Shack |
| 2140 | item_blueprint_pet_shop | 825 | Blueprint: Pets, Pets, Pets |
| 2141 | item_blueprint_graveyard | 1626 | Blueprint: Twisted Tree |
| 2142 | item_desc_bank_grave_bag | 1627 | Recipes for the Trusty Bank and the Twisted Tree Blueprints |
| 2143 | item_bank_grave_bag | 1628 | A bag of blueprint recipes |
| 2144 | item_desc_bank_farm_bag | 1908 | Recipes for the Trusty Bank, Composter and Refrigerator |
| 2145 | item_bank_farm_bag | 1909 | A bag of blueprint recipes |
| 2146 | item_blueprint_chicken_coop | 1779 | Blueprint: Chicken Coop |
| 2147 | item_chicken_coop_recipe | 1780 | Recipe: Chicken Coop |
| 2148 | item_coop_recipe_desc | 1781 | Learn the blueprint for making a chicken coop |
| 2149 | item_blueprint_mushroom | 2130 | Blueprint: Mushroom Station |
| 2150 | item_blueprint_greenhouse | 2513 | Blueprint: Greenhouse |
| 2151 | item_blueprint_barn | 2879 | Blueprint: Barn |
| 2152 | item_barn_recipe | 3188 | Recipe: Barn |
| 2153 | item_barn_recipe_desc | 3189 | Learn the blueprint for making a barn |
| 2154 | item_blueprint_beehive | 2889 | Blueprint: Beehive |
| 2155 | item_blueprint_badlands | 3280 | Blueprint: Badlands Station |
| 2156 | item_blueprint_townhall | 3355 | Blueprint: Town Hall |
| 2157 | item_time_space_normal | 826 | Normal World Book |
| 2158 | time_space_normal_desc | 827 | Regenerate and respawn the dungeon in normal difficulty (Level 1-35) |
| 2159 | item_time_space_nightmare | 828 | Nightmare World Book |
| 2160 | time_space_nightmare_desc | 829 | Regenerate and respawn the dungeon in nightmare difficulty (Level 35-65) |
| 2161 | nightmare_debuff | 1711 | Nightmare Difficulty#n-50 to all resist ratings |
| 2162 | item_time_space_hell | 830 | Hell World Book |
| 2163 | time_space_hell_desc | 831 | Regenerate and respawn the dungeon in hell difficulty (Level 65-90) |
| 2164 | hell_debuff | 2011 | Hell Difficulty#n-100 to all resist ratings |
| 2165 | ? | 2012 | Inferno World Book |
| 2166 | ? | 2013 | Regenerate and respawn the dungeon in inferno difficulty (Level 90-100) |
| 2167 | ? | 2014 | Inferno Difficulty#n-150 to all resist ratings |
| 2168 | ? | 832 | You must be standing on the surface to read this |
| 2169 | ? | 833 | The world feels different |
| 2170 | ? | 834 | CAUTION: Deletes all items, blocks, and objects in the dungeon |
| 2171 | ? | 2145 | The Offical Document of Do-Over |

### Rows 2593-2623 (tutorial quests; the id column is scrolled out of view)

| Row | Token | Text |
|---|---|---|
| 2593 | quest_tools_text | #yTOOLS FOR THE JOB#n#wYou are going to need to construct tools#nto survive and fight in this war.#n#n#yMake a few tools#n |
| 2594 | quest_tools_objective_1 | Craft a Simple Pickaxe |
| 2595 | quest_tools_objective_2 | Craft an Axe |
| 2596 | quest_level_up_title | Getting Stronger |
| 2597 | quest_level_up_text | #yGETTING STRONGER#n#wCongratulations! You have gained a level.#nYou earned a point to spend on your#nattributes.#n#n#ySpend a leveling point#n |
| 2598 | quest_level_up_obj_1 | Click on the level up button |
| 2599 | quest_instant_title | Instant use items |
| 2600 | quest_instant_text | #yINSTANT USE ITEMS#n#wSometimes items can be used without#nchanging which slot is highlighted. These#ninstant use items have a special border#naround the slot. Pressing the key#nassociated with the slot uses the item.#n#n |
| 2601 | quest_instant_obj_1 | Place the Learning Potion into a slot#non left-half of your main bar |
| 2602 | quest_instant_obj_2 | Quaff the potion by pressing the key |
| 2603 | quest_ancient_title | Ancient Items |
| 2604 | quest_ancient_text | #yAncient Items#n#wAncient items are skills. Place them in your#nmain bar to use them like Instant Items.#nYou can rank them up by spending gold.#n#n |
| 2605 | quest_ancient_obj_1 | Activate your Ancient Item |
| 2606 | quest_ancient_complete | Get your next class Ancient Item at#nLevel 5 |
| 2607 | quest_cooking_title | Cooking your food |
| 2608 | quest_cooking_text | #yCOOKING YOUR FOOD#n#wIt is important to cook food when you can.#nRaw food does not feed you as much.#nMonsters can drop meat.#nYou can harvest and eat raw mushrooms#n#n |
| 2609 | quest_cooking_obj_1 | Craft and place a Campfire |
| 2610 | quest_cooking_obj_2 | Cook any recipe |
| 2611 | quest_cooking_complete | Meat can come from killing rats and monsters#nin the dungeon. Mushrooms can be consumed,#nbut not cooked. |
| 2612 | quest_forge_title | Your home base |
| 2613 | quest_forge_text | #yYOUR HOME BASE#n#wYou are going to need a base of#noperations. Find a location and fill it with#ndevices.#n#n#yMake a forge#n |
| 2614 | quest_forge_objective_1 | Dig up some cobblestone |
| 2615 | quest_forge_objective_2 | Craft and place a forge |
| 2616 | quest_forge_objective_3 | Craft and place a workbench |
| 2617 | quest_farming_title | Keeping yourself fed |
| 2618 | quest_farming_text | #yKEEPING YOURSELF FED#n#wYou will become hungry as you adventure.#nYou will need to create a farm and grow#ncrops.#n#n |
| 2619 | quest_farming_objective_1 | Craft a planter box blueprint |
| 2620 | quest_farming_objective_2 | Clear an area and place the planter box#nin the world |
| 2621 | quest_farming_objective_3 | Plant any seed in the dirt mound |
| 2622 | quest_survival_title | Surviving Skills |
| 2623 | quest_survival_text | #ySURVIVING SKILLS#n#wAs you plant, harvest, mine, and cook#nyou will gain survival experience. As you#nadvance your survival... (cut off) |

Following rows (tokens hidden): "Open your Survival Panel by clicking#non the green plus or using ('#v')", "Your
survival rank will increase the amount#nof feed that crops and cooked food#nprovide. So, it is important to work on
it.", "Commander Vile", "#yCOMMANDER VILE#n#wPrepare for the fight below by#noutfitting yourself for battle and
killing#nCommander Vile#n#n", "Craft some iron armor", "Craft an improved weapon", "Kill Commander Vile on the surface".

### Rows 2713-2750 (level quests, survival quests, vendors)

| Row | Token | Text |
|---|---|---|
| 2713 | quest_level_60_complete | Congratulations on reaching level 60!#nYou get an ancient unbinding key. |
| 2714 | quest_level_70_title | Seventy |
| 2715 | quest_level_70_complete | Congratulations on reaching level 70!#nYou get an ancient unbinding key. |
| 2716 | quest_level_80_title | Eighty |
| 2717 | quest_level_80_complete | Congratulations on reaching level 80!#nYou get an ancient unbinding key. |
| 2718 | quest_level_90_title | Ninety |
| 2719 | quest_level_90_complete | Congratulations on reaching level 90!#nYou get an ancient unbinding key. |
| 2720 | quest_survival_seeds_title | Urgent! Finding Seeds |
| 2721 | quest_survival_seeds_text | #yURGENT! FINDING SEEDS#n#wYou must prepare to feed yourself soon!#nThis food will not last long. Quickly venture#nbelow and search for more seeds.#nMonsters will drop them and you can find#nbags of seeds as well.#n#n |
| 2722 | quest_survival_seeds_obj_1 | Gather seeds that drop#nfrom monsters below |
| 2723 | quest_survival_seeds_obj_2 | Plant some more seeds |
| 2724 | quest_survival_meat_title | Finding Meat |
| 2725 | quest_survival_meat_text | #yFINDING MEAT#n#wMeat is an easy way to feed yourself#nwhile you wait for crops to grow.#nLarge meat drops off monsters and small meat chunks drop off rats#n#n |
| 2726 | quest_survival_meat_obj_1 | Obtain 3 meat from#nmonsters, rats or spits |
| 2727 | quest_survival_meat_obj_2 | Cook any meat recipe |
| 2728 | quest_survival_mushroom_title | Gathering Mushrooms |
| 2729 | quest_survival_mushroom_text | #yGATHERING MUSHROOMS#n#wMushrooms can provide a bit of food,#nbut many have a small drawback. There are#ntwo mushrooms that provide a lot of sustinance: meal and feast.#n#n |
| 2730 | quest_survival_mush_obj_1 | Find a meal mushroom |
| 2731 | quest_complete | Quest Complete! |
| 2732 | quest_reward | Reward: |
| 2733 | quest_btn_ok | Ok |
| 2734 | quest_btn_complete | Complete |
| 2735 | quest_btn_cancel | Cancel |
| 2736 | quest_objectives_complete | Objectives Complete |
| 2737 | vendor_title_seeds | General Divo |
| 2738 | vendor_seeds_description | Seeds and Recipes |
| 2739 | vendor_title_banker | Banker K.A. |
| 2740 | vendor_banker_desc | Your Bank |
| 2741 | vendor_bank_transfer | Use shift + right mouse button to quick transfer |
| 2742 | vendor_title_wizard | The Legendary Wizard Soule |
| 2743 | vendor_wizard_desc | Space and Time |
| 2744-2750 | ? | Hurk / Hats, hats, hats! / Kiki / Meow / Mighty Runewords / Runewords Galore / Classy Runewords |

Rows 2707-2711 are partly hidden: "...reaching level 15!#nYou will get your next class ancient item#nat level 20.",
"...reaching level 20!#nYou will get your final class ancient item#nat level 25.", "...reaching level 50!#nYou get an
ancient unbinding key."

### Rows 2908-2976 (survival skill tree; id column out of view)

| Row | Token | Text |
|---|---|---|
| 2908 | survival_seeds_extra_plant_desc | Gain a 10% chance at planting two crops per seed |
| 2909 | survival_seeds_compost | Seed Composting |
| 2910 | survival_seeds_compost_desc | Gain the ability to compost seeds |
| 2911 | survival_seeds_exp | Planting Experience |
| 2912 | survival_seeds_exp_desc | Gain 25% more survival experience while planting |
| 2913 | survival_seeds_extra_seeds | Bonus Seeds |
| 2914 | survival_seeds_extra_seeds_desc | Gain an addition 15% chance of a second seed when extracting from a crop |
| 2915 | survival_meat_recipes | Meat Entrees |
| 2916 | survival_meat_recipes_desc | Unlock useful meat recipes |
| 2917 | survival_meat_stack_size | Efficient Butcher |
| 2918 | survival_meat_stack_size_desc | Double the stack-size of all meat and meat dishes |
| 2919 | survival_meat_feed_bonus | Meat Steaks |
| 2920 | survival_meat_feed_bonus_desc | Gain 25% more feed from meat and meat dishes |
| 2921 | survival_meat_compost | Meat Compost |
| 2922 | survival_meat_compost_desc | Gain the ability to compost meat |
| 2923 | survival_mushroom_recipes | Mushroom Dishes |
| 2924 | survival_mushroom_recipes_desc | Unlock recipes to cook mushrooms |
| 2925 | survival_mushroom_harvest | Extra Forage |
| 2926 | survival_mushroom_harvest_desc | Gain a chance to pick an extra mushroom when harvesting |
| 2927 | survival_mushroom_stack_size | Mushroom Storage |
| 2928 | survival_mushroom_stack_size_desc | Double the stack-size of mushrooms |
| 2929 | survival_mushroom_feed | Mushroom nutrition |
| 2930 | survival_mushroom_feed_desc | Gain 25% more feed from mushrooms and mushroom dishes |
| 2931 | survival_mushroom_debuff | Mithridatism |
| 2932 | survival_mushroom_debuff_desc | Reduce negative mushroom effect lengths by 50% |
| 2933 | survival_cooking_spring | Spring Recipes |
| 2934 | survival_cooking_spring_desc | Learn to cook a variety of spring foods |
| 2935 | survival_cooking_adv_spring | Advanced Spring Recipes |
| 2936 | survival_cooking_adv_spring_desc | Learn to cook a variety of spring food recipes that provide benefits |
| 2937 | survival_cooking_summer | Summer Recipes |
| 2938 | survival_cooking_summer_desc | Learn to cook a variety of summer foods |
| 2939 | survival_cooking_adv_summer (token partly hidden) | Advanced Summer Recipes |
| 2940 | survival_cooking_adv_summer_desc | Learn to cook a variety of summer food recipes that provide benefits |
| 2941 | survival_cooking_autumn | Autumn Recipes |
| 2942 | survival_cooking_autumn_desc | Learn to cook a variety of autumn foods |
| 2943 | survival_cooking_adv_autumn | Advanced Autumn Recipes |
| 2944 | survival_cooking_adv_autumn_desc | Learn to cook a variety of autumn food recipes that provide benefits |
| 2945 | survival_cooking_buff_extend | Sous Chef |
| 2946 | survival_cooking_buff_extend_desc | Extend all food buffs by 25% |
| 2947 | survival_cooking_buff_resist_25 | Head Chef |
| 2948 | survival_cooking_buff_resist_25_desc | Add (+25 resistance rating to all) to any food buffs |
| 2949 | survival_cooking_buff_resist_50 | Executive Chef |
| 2950 | survival_cooking_buff_resist_50_desc | Add (+50 resistance rating to all) to any food buffs |
| 2951 | survival_planter_box | Extra Planter Box |
| 2952 | survival_planter_box_desc | Learn how craft a second planter box |
| 2953 | survival_refrigerator | Refrigerator |
| 2954 | survival_refrigerator_desc | Learn how to make a refrigerator to store food |
| 2955 | survival_mushroom_station | Mushroom Station |
| 2956 | survival_mushroom_station_desc | Learn how to make a mushroom station to grow mushrooms |
| 2957 | survival_greenhouse | Greenhouse |
| 2958 | survival_greenhouse_desc | Learn how to make a Greenhouse building to grow anything, anytime |
| 2959 | survival_health_potions | Healing Potions |
| 2960 | survival_health_potions_desc | Learn how to craft healing potions |
| 2961 | survival_health_nightmare | Nightmare Healing |
| 2962 | survival_health_nightmare_desc | Learn how to make Nightmare healing potions |
| 2963 | survival_health_hell | Hell Healing |
| 2964 | survival_health_hell_desc | Learn how to make Hell healing potions |
| 2965 | survival_health_inferno | Inferno Healing |
| 2966 | survival_health_inferno_desc | Learn how to make Inferno healing potions |
| 2967 | survival_invulnerable | Invulnerable |
| 2968 | survival_invulnerable_desc | Learn how to make a potion that makes you invulnerable for 3 seconds |
| 2969 | survival_health_potion_stack | Healthy Dose |
| 2970 | survival_health_potion_stack_desc | Double the stack-size of all healing potions |
| 2971-2976 | ? | Healthier / Increase the effectiveness of health potions by 10% of their base / Healthiest / Increase the effectiveness of health potions by 25% of their base / Healing Reload / Automatically replace potions of the same type in your active bar from your inventory |

## Solution layout (Solution Explorer sidebar - small text, lower confidence than the code above)

- Header Files / `xls`: `ambient_hdr.h`, `block_textures_hdr.h`, `character_classes_hdr.h`, `damage_types_hdr.h`,
  `emitters_hdr.h`, `equipment_slots_hdr.h`, `items_hdr.h`, `missiles_hdr.h`, `monster_sprites_hdr.h`, `monsters_hdr.h`,
  `objects_hdr.h`, `props_hdr.h`, `quests_hdr.h`, `recipes_hdr.h`, `runewords_hdr.h`, `sounds_hdr.h`, `strings_hdr.h`,
  `tiles_hdr.h`, `treasure_hdr.h`; plus `main.h`, `resource.h`.
- Source Files / `engine_core` (contents not expanded).
- Source Files / `game` (each has a matching `.h` under Header Files / `game`): `g_ambient`, `g_beta_closed`,
  `g_blueprint`, `g_buffdebuff`, `g_char_create`, `g_char_select`, `g_character_class`, `g_collision`, `g_community`,
  `g_damage_types`, `g_diamondsquare`, `g_dungeon`, `g_dungeon_geo`, `g_effects`, `g_emitters`, `g_environment`,
  `g_game`, `g_intro`, `g_items`, `g_main_menu`, `g_missiles`, `g_monsters`, `g_news`, `g_npcs`, `g_objects`,
  `g_options_menu`, `g_particles`, `g_player`, `g_player_gfx`, `g_player_inventory`, `g_player_levels`, `g_quests`,
  `g_rlg`, `g_rlg_layout`, `g_save_load`, `g_story`, `g_teams`, `g_treasure`, `g_ui`, `g_ui_altar`, `g_ui_ancients`,
  `g_ui_banker`, `g_ui_bars`, `g_ui_blink`, `g_ui_boss_hp_bar`, `g_ui_breath`, `g_ui_core`, `g_ui_collector`,
  `g_ui_crafting`, `g_ui_cursor`, `g_ui_font`, `g_ui_hotbar`, `g_ui_inventory`, `g_ui_levelup`, `g_ui_map`,
  `g_ui_menu`, `g_ui_messages`, `g_ui_mouseitem`, `g_ui_popup`, `g_ui_quest`, `g_ui_radial`, `g_ui_refrigerator`,
  `g_ui_selection`, `g_ui_status` (list continues below the visible area).
- Headers also seen: `g_unique_items.h`, `g_units.h`, `g_dungeon_internal.h`, `g_player_data.h`.
- Output window: the debug target is `ILB.exe`.

## Not captured

- `g_units.h` after line 497 (the rest of `unit_save_data_v0` and any later save structs) is never shown.
- `g_items.h` lines 115-123 and 157-243, and everything after line 303.
- `g_items.cpp` lines 34-47 and 81-174 (enchant enum, most of the item excel table).
- Values of `num_damage_types`, `game_framerate`, and the layouts of `safestring`, `vector2d`, `box`.
- Types such as `dword`, `byte`, `safestring` are engine typedefs whose definitions are not on screen.
