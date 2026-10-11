// Native test of src/ec_fmod.cpp with a scripted FMOD: checks channel bookkeeping and volume rules.
#include "main.h"
#include "ec_fileio.h"
#include "ec_config.h"
#include "ec_fmod.h"
#include "g_strings.h"
#include "g_sounds.h"
#include "fake_platform.h"
#include <fmod.hpp>

string_definition_data * string_definitions = NULL;
int num_string_definitions = 0;
sound_definition_data * sound_definitions = NULL;
int num_sound_definitions = 0;

// --- scripted FMOD -----------------------------------------------------------
struct fake_channel { float volume; bool paused; bool playing; int loops; };
static fake_channel fake_channels[ 64 ];
static int fake_channels_used = 0;
static int fake_sounds_created = 0, fake_sounds_released = 0;
namespace FMOD
{
    static System * the_system = (System *)0x1000;
    FMOD_RESULT System_Create( System ** system ) { *system = the_system; return FMOD_OK; }
    FMOD_RESULT System::getVersion( unsigned int * version ) { *version = 0x00020000; return FMOD_OK; }
    FMOD_RESULT System::init( int, unsigned int, void * ) { return FMOD_OK; }
    FMOD_RESULT System::close() { return FMOD_OK; }
    FMOD_RESULT System::release() { return FMOD_OK; }
    FMOD_RESULT System::update() { return FMOD_OK; }
    FMOD_RESULT System::createSound( const char *, FMOD_MODE, FMOD_CREATESOUNDEXINFO *, Sound ** sound ) { *sound = (Sound *)(size_t)( ++fake_sounds_created ); return FMOD_OK; }
    FMOD_RESULT System::playSound( Sound *, ChannelGroup *, bool paused, Channel ** channel )
    {
        fake_channel * c = &fake_channels[ fake_channels_used++ ];
        *c = { 1.0f, paused, true, 0 };
        *channel = (Channel *)c;
        return FMOD_OK;
    }
    FMOD_RESULT Sound::release() { fake_sounds_released++; return FMOD_OK; }
    FMOD_RESULT Channel::setVolume( float volume ) { ( (fake_channel *)this )->volume = volume; return FMOD_OK; }
    FMOD_RESULT Channel::setPaused( bool paused ) { ( (fake_channel *)this )->paused = paused; return FMOD_OK; }
    FMOD_RESULT Channel::setLoopCount( int count ) { ( (fake_channel *)this )->loops = count; return FMOD_OK; }
    FMOD_RESULT Channel::stop() { ( (fake_channel *)this )->playing = false; return FMOD_OK; }
    FMOD_RESULT Channel::isPlaying( bool * playing ) { *playing = ( (fake_channel *)this )->playing; return FMOD_OK; }
}

static int failures = 0;
#define CHECK( cond ) do { if ( !( cond ) ) { printf( "FAILED line %d: %s\n", __LINE__, #cond ); failures++; } } while ( 0 )

int main( int argc, char ** argv )
{
    if ( argc < 2 ) { printf( "usage: test_sound <game dir>\n" ); return 2; }
    assert_system_init();
    memory_system_init();
    debug_log_start();
    strcpy( game_path, argv[ 1 ] );
    CHECK( packfile_load() );

    // three table rows using real files from the pack: an effect with two alternatives, music, combat
    static sound_definition_data table[ 3 ];
    strcpy( table[ 0 ].sound, "effect" );   table[ 0 ].stype = sound_type_sfx;    table[ 0 ].volume_mod = 0.5f; table[ 0 ].num_alts = 2;
    strcpy( table[ 1 ].sound, "music" );    table[ 1 ].stype = sound_type_music;  table[ 1 ].volume_mod = 1.0f; table[ 1 ].num_alts = 1; table[ 1 ].loops = true;
    strcpy( table[ 2 ].sound, "combat" );   table[ 2 ].stype = sound_type_combat; table[ 2 ].volume_mod = 1.0f; table[ 2 ].num_alts = 1;
    const char * wav = argc > 2 ? argv[ 2 ] : "ui_click", * ogg = argc > 3 ? argv[ 3 ] : "surface";
    strcpy( table[ 0 ].filename[ 0 ], wav ); strcpy( table[ 0 ].filename[ 1 ], wav );
    strcpy( table[ 1 ].filename[ 0 ], ogg ); strcpy( table[ 2 ].filename[ 0 ], wav );
    sound_definitions = table;
    num_sound_definitions = 3;

    config.sfx_volume = 0.8f; config.music_volume = 0.5f; config.combat_volume = 0.0f;

    CHECK( fmod_sound_engine_init() );
    sounds_load_all();
    CHECK( fake_sounds_created == 4 );

    dword a = channel_play( 0, 1.0f );
    CHECK( a == 0 );
    CHECK( fake_channels[ 0 ].volume > 0.399f && fake_channels[ 0 ].volume < 0.401f );   // 1.0 * sfx 0.8 * mod 0.5
    CHECK( !fake_channels[ 0 ].paused );
    CHECK( channel_play( 0, 1.0f ) == invalid_id );        // same sound again in the same frame
    channels_update();
    dword b = channel_play( 0, 1.0f );
    CHECK( b == 1 );                                        // allowed again after an update
    CHECK( channel_play( 2, 1.0f ) == invalid_id );         // combat volume is zero
    CHECK( channel_play( 0, 0.0f ) == invalid_id );
    CHECK( channel_play( invalid_id, 1.0f ) == invalid_id );
    CHECK( channel_play( 7, 1.0f ) == invalid_id );         // asserts: no such sound

    dword m = channel_loop( 1, 1.0f );
    CHECK( m == 2 && fake_channels[ 2 ].loops == -1 && fake_channels[ 2 ].volume == 0.5f );
    channel_set_volume( m, 0.5f );
    CHECK( fake_channels[ 2 ].volume == 0.25f );
    channel_pause( m, true );
    CHECK( fake_channels[ 2 ].paused );

    fake_channels[ 0 ].playing = false;                     // the first effect finishes
    channels_update();
    CHECK( !channel_is_playing( a ) && channel_is_playing( b ) && channel_is_playing( m ) );
    CHECK( channel_play( 0, 1.0f ) == 0 );                  // its slot is free again

    channel_stop( m );
    CHECK( !channel_is_playing( m ) && !fake_channels[ 2 ].playing );

    sounds_free_all();
    CHECK( fake_sounds_released == 4 );
    fmod_sound_engine_close();
    packfile_close();

    printf( failures ? "%d check(s) FAILED\n" : "sound: all checks passed\n", failures );
    return failures != 0;
}
