//-----------------------------------------------------------------------------
// ec_config.h
//
// Settings and key bindings, stored as text in ilb_config.txt in the save folder: one
// "name value" pair per line.
//
// The setting names are the ones written to the file (table at 0x5D2C80), used here as the field
// and enum names. config_data, config, config_load, config_save and config_set_value are names
// chosen here; get_next_line, get_next_word, config_decode, init_config_file_memory,
// free_config_file_memory and config_file_add_line are original.

#ifndef _EC_CONFIG_H_
#define _EC_CONFIG_H_

#define current_config_version      4

enum
{
    config_version,
    config_fullscreen,
    config_borderless,
    config_vsync,
    config_window_width,
    config_window_height,
    config_camera_dist,
    config_gamma,
    config_screen_shake,
    config_quality_lighting,
    config_bloom,
    config_music_volume,
    config_combat_volume,
    config_sfx_volume,
    config_show_helm,
    config_auto_walk_up_one,
    config_always_show_item_names,
    config_default_style,
    config_hp_bar,
    config_map_mode,
    config_map_transparency,
    config_latest_news,
    config_first_tips,
    config_rain,
    config_pickup_seeds,
    config_gamepad,
    config_xbox_input,
    config_direction_dig,
    config_cloud_saves,
    config_verbose_loading,
    config_damage_numbers,
    config_key_move_left,
    config_key_move_right,
    config_key_move_up,
    config_key_crouch,
    config_key_jump,
    config_key_interact,
    config_key_recharge,
    config_key_sleep,
    config_key_shift,
    config_key_ctrl,
    config_key_hotbar_1,
    config_key_hotbar_alt_1,
    config_key_hotbar_2,
    config_key_hotbar_alt_2,
    config_key_hotbar_3,
    config_key_hotbar_alt_3,
    config_key_hotbar_4,
    config_key_hotbar_alt_4,
    config_key_hotbar_5,
    config_key_hotbar_alt_5,
    config_key_hotbar_6,
    config_key_hotbar_alt_6,
    config_key_hotbar_7,
    config_key_hotbar_alt_7,
    config_key_hotbar_8,
    config_key_hotbar_alt_8,
    config_key_hotbar_9,
    config_key_hotbar_alt_9,
    config_key_hotbar_10,
    config_key_hotbar_alt_10,
    config_key_hotbar_11,
    config_key_hotbar_alt_11,
    config_key_hotbar_12,
    config_key_hotbar_alt_12,
    config_key_inventory,
    config_key_inventory_alt,
    config_key_crafting,
    config_key_crafting_alt,
    config_key_minimap,
    config_key_minimap_alt,
    config_key_minimap_up,
    config_key_minimap_down,
    config_key_survival,
    config_key_enter,
    config_key_escape,
    config_key_recall,
    config_key_story_log,
    config_key_achievement_panel,
    config_key_directional_dig,
    config_key_quest_log,

    num_config_strings
};

#define first_config_key    config_key_move_left
#define num_config_keys     ( num_config_strings - first_config_key )

struct config_data      // 0x60A028, 296 bytes
{
    int         version;
    bool        fullscreen;
    bool        borderless;
    bool        vsync;
    int         window_width;
    int         window_height;
    float       gamma;
    int         quality_lighting;
    bool        screen_shake;
    bool        bloom;
    float       music_volume;
    float       combat_volume;
    float       sfx_volume;
    bool        show_helm;
    bool        auto_walk_up_one;
    bool        always_show_item_names;
    int         latest_news;
    bool        first_tips;
    bool        rain;
    bool        pickup_seeds;
    bool        gamepad;
    int         xbox_input;
    bool        direction_dig;
    bool        cloud_saves;
    bool        verbose_loading;
    int         damage_numbers;
    bool        hp_bar;
    int         default_style;
    int         map_mode;
    float       map_transparency;
    float       unknown_50;             // these three are not stored in the file; the first two are passed
    float       unknown_54;             // to every shader, the third picks the map's texture filtering
    float       unknown_58;
    float       camera_dist;
    int         keys[ num_config_keys ];    // SDL keycodes, indexed by config_key_* - first_config_key
};

// one line of a text file being built for appdata_file_text_save; the struct name is original
#define max_config_line_size        1024
struct config_strings
{
    char        string[ max_config_line_size ];
};

extern config_data config;

int get_next_word( char * source, char * source_end, char * dest );
void config_decode( byte * file_data, dword file_size );
void config_file_add_line( char * line );
void config_set_default_keys();
void config_save( config_data * data );
void config_load();

#endif
