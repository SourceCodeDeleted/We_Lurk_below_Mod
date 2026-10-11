//-----------------------------------------------------------------------------
// ec_fmod.h
//
// Sound through FMOD. Every row of the sounds table has up to four alternative files; playing a
// sound picks one at random and returns a channel id (0..31, or invalid_id).
//
// Original names: fmod_sound_engine_init, sounds_load_all, sounds_free_all, channel_play,
// channel_loop, channel_set_volume, channel_stop, channel_is_playing, channel_pause, and from
// assert text fmod_system, fmod_sound, channels, fmod_channel, max_channels, max_sound_alts,
// num_sound_types, stype. Chosen here: fmod_sound_engine_close, channels_update, the sound_type_*
// names and the other channel fields.

#ifndef _EC_FMOD_H_
#define _EC_FMOD_H_

#define max_channels        32
#define max_sound_alts      4

enum
{
    sound_type_sfx = 0,     // scaled by config.sfx_volume
    sound_type_music,       // config.music_volume; files are music\*.ogg
    sound_type_combat,      // config.combat_volume

    num_sound_types
};

bool fmod_sound_engine_init();
void fmod_sound_engine_close();
void sounds_load_all();
void sounds_free_all();
dword channel_play( dword sound_id, float volume );
dword channel_loop( dword sound_id, float volume );
void channel_set_volume( dword channel_id, float volume );
void channel_stop( dword channel_id );
bool channel_is_playing( dword channel_id );
void channel_pause( dword channel_id, bool paused );
void channels_update();

#endif
