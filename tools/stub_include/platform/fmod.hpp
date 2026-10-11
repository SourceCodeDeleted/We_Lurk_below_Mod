#pragma once
// minimal FMOD Core API stand-in (see README.txt)
typedef int FMOD_RESULT;
typedef unsigned int FMOD_MODE;
#define FMOD_OK             0
#define FMOD_INIT_NORMAL    0
#define FMOD_LOOP_OFF       0x00000001
#define FMOD_LOOP_NORMAL    0x00000002
#define FMOD_OPENMEMORY     0x00000800
struct FMOD_CREATESOUNDEXINFO { int cbsize; unsigned int length; unsigned char rest[ 140 ]; };
namespace FMOD
{
    class Sound { public: FMOD_RESULT release(); };
    class ChannelGroup;
    class Channel
    {
    public:
        FMOD_RESULT setVolume( float volume );
        FMOD_RESULT setPaused( bool paused );
        FMOD_RESULT setLoopCount( int count );
        FMOD_RESULT stop();
        FMOD_RESULT isPlaying( bool * playing );
    };
    class System
    {
    public:
        FMOD_RESULT getVersion( unsigned int * version );
        FMOD_RESULT init( int maxchannels, unsigned int flags, void * extradriverdata );
        FMOD_RESULT close();
        FMOD_RESULT release();
        FMOD_RESULT update();
        FMOD_RESULT createSound( const char * name_or_data, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO * info, Sound ** sound );
        FMOD_RESULT playSound( Sound * sound, ChannelGroup * group, bool paused, Channel ** channel );
    };
    FMOD_RESULT System_Create( System ** system );
}
