//-----------------------------------------------------------------------------
// ec_fmod.cpp
// Reconstructed from ILB.exe: sounds_load_all 0x406240, channel_play 0x406480, channel_loop
// 0x4067F0, channel_set_volume 0x406A40, channel_stop 0x406BB0, channel_is_playing 0x406C30,
// channels_update 0x406CD0. Taken from inlined copies: fmod_sound_engine_init (in the client
// start-up at 0x5817D0), fmod_sound_engine_close (end of main), sounds_free_all (in the exit mode
// at 0x57FFD0), channel_pause (in 0x45ED50).
//
// Not reconstructed yet: 0x406710, which plays a sound with its volume falling off with distance
// from the player (it needs the player and world-wrap code).

#include "main.h"
#include "ec_fileio.h"
#include "ec_config.h"
#include "ec_fmod.h"
#include "g_sounds.h"
#include <windows.h>
#include <fmod.hpp>

// The array holds exactly one row per line of the sounds table as it was when the game was built
// (2208 bytes = 138 x 4 pointers); the original takes the count from the generated sounds_hdr.h.
#define num_sounds      138

struct channel_data     // 20 bytes
{
    FMOD::Channel * fmod_channel;
    dword           sound_id;
    bool            looping;
    float           volume;
    bool            just_started;   // set by channel_play, cleared by the next channels_update
};

static FMOD::System * fmod_system = NULL;                                   // 0x17035B4
static char sound_filename[ 1024 ];                                         // 0x17035B8
static channel_data channels[ max_channels ];                               // 0x17039B8
static int num_active_channels = 0;                                         // 0x1703C38
static FMOD::Sound * fmod_sound[ num_sounds ][ max_sound_alts ];            // 0x1703C40

//-----------------------------------------------------------------------------

static void fmod_fatal_error( dword string_token_id )
{
    safestring_create_from_id( &temp_string, string_token_id );
    MessageBoxW( NULL, temp_string.text, L"Fatal Error", 0 );
}

//-----------------------------------------------------------------------------

bool fmod_sound_engine_init()
{
    for ( int i = 0; i < max_channels; i++ )
    {
        channels[ i ].fmod_channel = NULL;
        channels[ i ].sound_id = invalid_id;
        channels[ i ].just_started = false;
    }
    num_active_channels = 0;
    memset( fmod_sound, 0, sizeof( fmod_sound ) );

    assert_return_false( fmod_system == NULL );

    if ( FMOD::System_Create( &fmod_system ) != FMOD_OK )
    {
        fmod_fatal_error( 82 );
        return false;
    }

    unsigned int version;
    if ( fmod_system->getVersion( &version ) != FMOD_OK || version < 0x00011005 )
    {
        fmod_fatal_error( 83 );
        return false;
    }

    if ( fmod_system->init( max_channels, FMOD_INIT_NORMAL, NULL ) != FMOD_OK )
    {
        fmod_fatal_error( 84 );
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

void fmod_sound_engine_close()
{
    if ( fmod_system )
    {
        fmod_system->close();
        fmod_system->release();
        fmod_system = NULL;
    }
}

//-----------------------------------------------------------------------------

void sounds_load_all()
{
    assert_return( num_sound_definitions > 0 );

    for ( int i = 0; i < num_sound_definitions; i++ )
    {
        assert_return( sound_definitions[i].num_alts >= 1 );
        assert_return( sound_definitions[i].num_alts <= max_sound_alts );

        for ( int a = 0; a < sound_definitions[ i ].num_alts; a++ )
        {
            assert_return( fmod_sound[i][a] == NULL );

            sprintf( sound_filename, ( sound_definitions[ i ].stype == sound_type_music ) ? "music\\%s.ogg" : "audio\\%s.wav",
                sound_definitions[ i ].filename[ a ] );

            dword file_size;
            byte * file_data = pack_file_load( sound_filename, &file_size );

            FMOD_CREATESOUNDEXINFO info;
            memset( &info, 0, sizeof( info ) );
            info.cbsize = sizeof( info );
            info.length = file_size;

            FMOD_RESULT result = fmod_system->createSound( (const char *)file_data,
                FMOD_OPENMEMORY | ( sound_definitions[ i ].loops ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF ), &info, &fmod_sound[ i ][ a ] );
            pack_file_close( file_data );
            assert_return( result == FMOD_OK );
        }
    }
}

//-----------------------------------------------------------------------------

void sounds_free_all()
{
    for ( int i = 0; i < num_sound_definitions; i++ )
    {
        for ( int a = 0; a < sound_definitions[ i ].num_alts; a++ )
        {
            assert_return( fmod_sound[i][a] );
            fmod_sound[ i ][ a ]->release();
            fmod_sound[ i ][ a ] = NULL;
        }
    }
}

//-----------------------------------------------------------------------------

static float sound_type_volume( int stype )
{
    if ( stype == sound_type_sfx ) return config.sfx_volume;
    if ( stype == sound_type_combat ) return config.combat_volume;
    return config.music_volume;
}

static dword channel_find_free()
{
    for ( dword i = 0; i < max_channels; i++ )
        if ( channels[ i ].fmod_channel == NULL ) return i;
    return invalid_id;
}

//-----------------------------------------------------------------------------
// Plays a sound once. A sound that was already started since the last channels_update is not
// started again.

dword channel_play( dword sound_id, float volume )
{
    if ( !( volume >= 0.0f ) ) { assert_exp( volume >= 0.0f ); return invalid_id; }
    if ( !( volume <= 1.0f ) ) { assert_exp( volume <= 1.0f ); return invalid_id; }

    if ( sound_id == invalid_id ) return invalid_id;
    if ( !( sound_id < (dword)num_sound_definitions ) ) { assert_exp( sound_id < (dword)num_sound_definitions ); return invalid_id; }
    if ( sound_definitions == NULL || volume == 0.0f ) return invalid_id;

    int stype = sound_definitions[ sound_id ].stype;
    if ( !( stype >= 0 ) ) { assert_exp( stype >= 0 ); return invalid_id; }
    if ( !( stype < num_sound_types ) ) { assert_exp( stype < num_sound_types ); return invalid_id; }

    float type_volume = sound_type_volume( stype );
    if ( type_volume == 0.0f ) return invalid_id;
    volume = volume * type_volume * sound_definitions[ sound_id ].volume_mod;
    if ( volume == 0.0f ) return invalid_id;

    for ( int i = 0; i < max_channels; i++ )
    {
        if ( channels[ i ].fmod_channel != NULL && channels[ i ].sound_id == sound_id && channels[ i ].just_started ) return invalid_id;
    }

    dword channel_id = channel_find_free();
    if ( channel_id == invalid_id ) return invalid_id;

    if ( !( sound_definitions[ sound_id ].num_alts > 0 ) ) { assert_exp( sound_definitions[ sound_id ].num_alts > 0 ); return invalid_id; }
    int choice = rand() % sound_definitions[ sound_id ].num_alts;
    if ( !fmod_sound[sound_id][choice] ) { assert_exp( fmod_sound[sound_id][choice] ); return invalid_id; }

    FMOD_RESULT result = fmod_system->playSound( fmod_sound[ sound_id ][ choice ], NULL, true, &channels[ channel_id ].fmod_channel );
    if ( !( result == FMOD_OK ) ) { assert_exp( result == FMOD_OK ); return invalid_id; }

    channels[ channel_id ].sound_id = sound_id;
    channels[ channel_id ].volume = volume;
    channels[ channel_id ].just_started = true;
    result = channels[ channel_id ].fmod_channel->setVolume( volume );
    if ( !( result == FMOD_OK ) ) { assert_exp( result == FMOD_OK ); return invalid_id; }

    channels[ channel_id ].looping = false;
    result = channels[ channel_id ].fmod_channel->setPaused( false );
    if ( !( result == FMOD_OK ) ) { assert_exp( result == FMOD_OK ); return invalid_id; }

    return channel_id;
}

//-----------------------------------------------------------------------------

dword channel_loop( dword sound_id, float volume )
{
    if ( sound_id == invalid_id ) return invalid_id;
    if ( !( sound_id < (dword)num_sound_definitions ) ) { assert_exp( sound_id < (dword)num_sound_definitions ); return invalid_id; }
    if ( volume == 0.0f ) return invalid_id;

    int stype = sound_definitions[ sound_id ].stype;
    if ( !( stype >= 0 ) ) { assert_exp( stype >= 0 ); return invalid_id; }
    if ( !( stype < num_sound_types ) ) { assert_exp( stype < num_sound_types ); return invalid_id; }

    float type_volume = sound_type_volume( stype );
    if ( type_volume == 0.0f ) return invalid_id;
    volume = volume * type_volume * sound_definitions[ sound_id ].volume_mod;
    if ( volume == 0.0f ) return invalid_id;

    dword channel_id = channel_find_free();
    if ( channel_id == invalid_id ) return invalid_id;

    if ( !( sound_definitions[ sound_id ].num_alts > 0 ) ) { assert_exp( sound_definitions[ sound_id ].num_alts > 0 ); return invalid_id; }
    int choice = rand() % sound_definitions[ sound_id ].num_alts;
    if ( !fmod_sound[sound_id][choice] ) { assert_exp( fmod_sound[sound_id][choice] ); return invalid_id; }

    FMOD_RESULT result = fmod_system->playSound( fmod_sound[ sound_id ][ choice ], NULL, true, &channels[ channel_id ].fmod_channel );
    if ( !( result == FMOD_OK ) ) { assert_exp( result == FMOD_OK ); return invalid_id; }

    channels[ channel_id ].sound_id = sound_id;
    channels[ channel_id ].volume = volume;
    channels[ channel_id ].just_started = false;
    result = channels[ channel_id ].fmod_channel->setVolume( volume );
    if ( !( result == FMOD_OK ) ) { assert_exp( result == FMOD_OK ); return invalid_id; }

    channels[ channel_id ].looping = true;
    result = channels[ channel_id ].fmod_channel->setLoopCount( -1 );
    if ( !( result == FMOD_OK ) ) { assert_exp( result == FMOD_OK ); return invalid_id; }

    result = channels[ channel_id ].fmod_channel->setPaused( false );
    if ( !( result == FMOD_OK ) ) { assert_exp( result == FMOD_OK ); return invalid_id; }

    return channel_id;
}

//-----------------------------------------------------------------------------

void channel_set_volume( dword channel_id, float volume )
{
    if ( channel_id == invalid_id ) return;

    assert_return( channel_id < max_channels );
    assert_return( channels[ channel_id ].fmod_channel );
    assert_return( volume >= 0.0f );
    assert_return( volume <= 1.0f );

    dword id = channels[ channel_id ].sound_id;
    assert_return( id < (dword)num_sound_definitions );

    int stype = sound_definitions[ id ].stype;
    assert_return( stype >= 0 );
    assert_return( stype < num_sound_types );

    float type_volume = sound_type_volume( stype );
    if ( type_volume != 0.0f )
    {
        channels[ channel_id ].volume = volume * type_volume * sound_definitions[ id ].volume_mod;
        FMOD_RESULT result = channels[ channel_id ].fmod_channel->setVolume( channels[ channel_id ].volume );
        assert_return( result == FMOD_OK );
    }
}

//-----------------------------------------------------------------------------

void channel_stop( dword channel_id )
{
    if ( channel_id == invalid_id ) return;

    assert_return( channel_id < max_channels );
    assert_return( channels[channel_id].fmod_channel );

    channels[ channel_id ].fmod_channel->stop();
    channels[ channel_id ].fmod_channel = NULL;
}

//-----------------------------------------------------------------------------

bool channel_is_playing( dword channel_id )
{
    assert_return_false( channel_id != invalid_id );
    assert_return_false( channel_id < max_channels );

    if ( channels[ channel_id ].fmod_channel == NULL ) return false;

    bool playing;
    channels[ channel_id ].fmod_channel->isPlaying( &playing );
    return playing;
}

//-----------------------------------------------------------------------------

void channel_pause( dword channel_id, bool paused )
{
    if ( channel_id == invalid_id ) return;

    assert_return( channel_id < max_channels );

    if ( channels[ channel_id ].fmod_channel ) channels[ channel_id ].fmod_channel->setPaused( paused );
}

//-----------------------------------------------------------------------------
// Once per frame: lets FMOD run, releases channels whose sound has finished and counts the rest.

void channels_update()
{
    fmod_system->update();
    num_active_channels = 0;

    for ( dword i = 0; i < max_channels; i++ )
    {
        if ( channels[ i ].fmod_channel == NULL ) continue;

        if ( !channels[ i ].looping )
        {
            channels[ i ].just_started = false;
            if ( !channel_is_playing( i ) )
            {
                channels[ i ].fmod_channel = NULL;
                continue;
            }
        }
        num_active_channels++;
    }
}
