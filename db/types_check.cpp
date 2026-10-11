// Compile-time checks of db/types.h and db/definitions.h against offsets and sizes read from ILB.exe.
// Run: clang++ -target i686-pc-windows-msvc -fsyntax-only -Wno-everything db/types_check.cpp
#include "types.h"
#include "definitions.h"
#include "custom_data.h"

#define OFFSET(type, field, value) static_assert(__builtin_offsetof(type, field) == value, #type "." #field)
#define SIZE(type, value) static_assert(sizeof(type) == value, "sizeof " #type)

SIZE(safestring, 2052);
SIZE(unit_stats, 472);
OFFSET(unit, flags, 2060);
OFFSET(unit, unit_type, 2080);
OFFSET(unit, unit_class, 2084);
OFFSET(unit, unit_mode, 2088);
OFFSET(unit, model_id, 2092);
OFFSET(unit, anim, 2096);
OFFSET(unit, stats, 2140);
OFFSET(unit, stats.attribute_flags, 2384);
OFFSET(unit, stats.aspire_set, 2536);
OFFSET(unit, stats.cosmic_set, 2540);
OFFSET(unit, stats.holiday_set, 2548);
OFFSET(unit, level, 2612);
OFFSET(unit, world_id, 2616);
OFFSET(unit, position, 2620);
OFFSET(unit, collision_shape, 2660);
OFFSET(unit, emitter_ids, 2704);
OFFSET(unit, type_next, 2780);
OFFSET(unit, data_version, 2788);
OFFSET(unit, custom_data, 2792);
SIZE(unit, 8936);             // init_units: memset( pool, 0, 0x4000 * 8936 )

OFFSET(item_custom_data, in_world, 1);
OFFSET(item_custom_data, stack_count, 12);
OFFSET(item_custom_data, slot_shape, 72);
OFFSET(item_custom_data, max_stack, 104);
OFFSET(item_custom_data, total_slevel, 112);
OFFSET(item_custom_data, num_crystal_slots, 136);
OFFSET(item_custom_data, crystal_ancient_affix_id, 280);
SIZE(item_custom_data, 364);   // item_save() returns this

OFFSET(item_definition_data, feed, 192);
#include "definitions_check.inc"

SIZE(monster_custom_data, 2112);  // monster_save
SIZE(object_custom_data, 332);    // object_save
SIZE(npc_custom_data, 200);       // npc_save
