// snd_mixer.c - the game's sound, on SDL audio (hand-written). Replaces FMOD in the window build.
//
// The game uses 14 FMOD calls: create a sound from a file image in memory (154 .wav and 14 .ogg files in the
// pack), play it on a channel, and set a channel's volume / pause / loop count or stop it. Those calls are
// implemented here with the same names and arguments, on a small software mixer: every playing channel is
// resampled to the output rate and summed in the SDL audio callback. Vorbis is decoded by stb_vorbis
// (third_party/stb_vorbis.c, public domain), streamed while it plays.
#ifdef ILB_WINDOW

typedef unsigned int size_t;
typedef struct _IO_FILE FILE;
extern FILE *stderr;
int fprintf( FILE *, const char *, ... );
void *calloc( size_t, size_t );
void *malloc( size_t );
void free( void * );
void *memcpy( void *, const void *, size_t );
void *memset( void *, int, size_t );
int memcmp( const void *, const void *, size_t );

// SDL2 (the audio part of its interface; SDL_AudioSpec as laid out on 32-bit)
typedef struct { int freq; unsigned short format; unsigned char channels, silence; unsigned short samples, padding; unsigned int size;
                 void ( *callback )( void *userdata, unsigned char *stream, int len ); void *userdata; } SDL_AudioSpec;
unsigned int SDL_OpenAudioDevice( const char *device, int capture, const SDL_AudioSpec *desired, SDL_AudioSpec *obtained, int allowed_changes );
void SDL_PauseAudioDevice( unsigned int device, int pause_on );
void SDL_LockAudioDevice( unsigned int device );
void SDL_UnlockAudioDevice( unsigned int device );
const char *SDL_GetError( void );

// stb_vorbis
typedef struct stb_vorbis stb_vorbis;
typedef struct { unsigned int sample_rate; int channels; unsigned int setup_memory_required, setup_temp_memory_required, temp_memory_required; int max_frame_size; } stb_vorbis_info;
stb_vorbis *stb_vorbis_open_memory( const unsigned char *data, int len, int *error, const void *alloc_buffer );
stb_vorbis_info stb_vorbis_get_info( stb_vorbis *f );
int stb_vorbis_get_samples_float_interleaved( stb_vorbis *f, int channels, float *buffer, int num_floats );
int stb_vorbis_seek_start( stb_vorbis *f );
void stb_vorbis_close( stb_vorbis *f );

#define STDCALL __attribute__(( stdcall ))
#define OUTPUT_RATE 48000
#define MAX_CHANNELS 64
#define BLOCK_FRAMES 2048

typedef struct
{
    int is_vorbis, loops;
    int in_snapshot;                    // existed when the game's start state was recorded: never freed (see snd_mixer_snapshot)
    int channels, rate, frames;         // wav: decoded
    float *samples;                     // wav: interleaved, channels per frame
    unsigned char *file;                // vorbis: the file image
    int file_size;
} sound;

typedef struct
{
    sound *source;
    int playing, paused, loop_count;    // loop_count: -1 forever, 0 play once
    float volume;
    double position;                    // in source frames (wav), or within the decoded block (vorbis)
    stb_vorbis *decoder;
    int rate, source_channels, block_frames;
    float block[ ( BLOCK_FRAMES + 1 ) * 2 ];    // vorbis: the frames being played, always stereo; [0] is the frame before
    // What the GAME is told about this channel goes by its own clock, not by the sound card's: the game asks
    // "is it still playing?" and makes random choices when the answer turns to no, and two copies of the game
    // fed the same input must get the same answer on the same simulation step.
    int game_playing, game_paused, game_forever;
    unsigned int game_end_tick, game_pause_tick, game_length;   // steps of 16 ms (the game's counter at 0x1703C3C)
} channel;

extern unsigned char ILB_image[];
#define GAME_TICK ( *(unsigned int *)( ILB_image + ( 0x1703C3C - 0x400000 ) ) )
float stb_vorbis_stream_length_in_seconds( stb_vorbis *f );

static int game_still_playing( channel *ch )
{
    if ( ch->game_playing && !ch->game_paused && !ch->game_forever && GAME_TICK >= ch->game_end_tick )
        ch->game_playing = 0;
    return ch->game_playing;
}

static channel channels[ MAX_CHANNELS ];
#define MAX_SOUNDS 8192
static sound *all_sounds[ MAX_SOUNDS ];
static int all_sounds_count;
static unsigned int device;

// ---------------------------------------------------------------------------------------- mixing
// next decoded block of a streamed sound into ch->block[1..]; 0 when it has ended (after looping if asked)
static int next_block( channel *ch )
{
    float decoded[ BLOCK_FRAMES * 2 ];
    int got = stb_vorbis_get_samples_float_interleaved( ch->decoder, 2, decoded, BLOCK_FRAMES * 2 );
    if ( got == 0 && ch->loop_count != 0 )
    {
        if ( ch->loop_count > 0 ) ch->loop_count--;
        stb_vorbis_seek_start( ch->decoder );
        got = stb_vorbis_get_samples_float_interleaved( ch->decoder, 2, decoded, BLOCK_FRAMES * 2 );
    }
    if ( got == 0 )
        return 0;
    if ( ch->block_frames > 0 )
    {
        ch->block[ 0 ] = ch->block[ ch->block_frames * 2 ];
        ch->block[ 1 ] = ch->block[ ch->block_frames * 2 + 1 ];
    }
    memcpy( ch->block + 2, decoded, got * 2 * sizeof( float ) );
    ch->block_frames = got;
    return 1;
}

static void mix_channel( channel *ch, float *out, int frames )
{
    double step = (double)ch->rate / OUTPUT_RATE;
    sound *s = ch->source;
    for ( int i = 0; i < frames; i++ )
    {
        float left, right;
        if ( s->is_vorbis )
        {
            while ( ch->position >= ch->block_frames )
            {
                ch->position -= ch->block_frames;
                if ( !next_block( ch ) ) { ch->playing = 0; return; }
            }
            int at = (int)ch->position;                 // block[ at ] is the frame before block[ at + 1 ]
            float t = (float)( ch->position - at );
            left = ch->block[ at * 2 ] * ( 1 - t ) + ch->block[ at * 2 + 2 ] * t;
            right = ch->block[ at * 2 + 1 ] * ( 1 - t ) + ch->block[ at * 2 + 3 ] * t;
        }
        else
        {
            if ( ch->position >= s->frames )
            {
                if ( ch->loop_count == 0 && !s->loops ) { ch->playing = 0; return; }
                if ( ch->loop_count > 0 ) ch->loop_count--;
                ch->position -= s->frames;
            }
            int at = (int)ch->position, next = at + 1 < s->frames ? at + 1 : at;
            float t = (float)( ch->position - at );
            const float *a = s->samples + at * s->channels, *b = s->samples + next * s->channels;
            left = a[ 0 ] * ( 1 - t ) + b[ 0 ] * t;
            right = s->channels > 1 ? a[ 1 ] * ( 1 - t ) + b[ 1 ] * t : left;
        }
        out[ i * 2 ] += left * ch->volume;
        out[ i * 2 + 1 ] += right * ch->volume;
        ch->position += step;
    }
}

static void audio_callback( void *userdata, unsigned char *stream, int len )
{
    float *out = (float *)stream;
    int frames = len / ( 2 * sizeof( float ) );
    memset( stream, 0, len );
    for ( int i = 0; i < MAX_CHANNELS; i++ )
        if ( channels[ i ].playing && !channels[ i ].paused )
            mix_channel( &channels[ i ], out, frames );
    for ( int i = 0; i < frames * 2; i++ )
        out[ i ] = out[ i ] > 1.0f ? 1.0f : out[ i ] < -1.0f ? -1.0f : out[ i ];
}

// ---------------------------------------------------------------------------------------- the system
int STDCALL FMOD_System_Create( void **system )
{
    static int the_system;
    *system = &the_system;
    return 0;
}

int STDCALL FMOD__System__getVersion( void *system, unsigned int *version )
{
    *version = 0x00020000;              // the game only checks for 0x11005 or newer
    return 0;
}

int STDCALL FMOD__System__init( void *system, int max_channels, unsigned int flags, void *extra )
{
    SDL_AudioSpec want = { 0 }, have;
    want.freq = OUTPUT_RATE;
    want.format = 0x8120;               // 32-bit float, native byte order
    want.channels = 2;
    want.samples = 1024;
    want.callback = audio_callback;
    device = SDL_OpenAudioDevice( 0, 0, &want, &have, 0 );
    if ( device == 0 )
    {
        fprintf( stderr, "sound: no audio device (%s); the game runs silent\n", SDL_GetError() );
        return 0;
    }
    SDL_PauseAudioDevice( device, 0 );
    return 0;
}

int STDCALL FMOD__System__update( void *system )    { return 0; }
// Stop every sound and forget what the game was told about them. Used when two copies of the game start a
// session together: both must begin with the same (empty) picture of what is playing.
void snd_mixer_forget_all( void )
{
    if ( device ) SDL_LockAudioDevice( device );
    for ( int i = 0; i < MAX_CHANNELS; i++ )
        channels[ i ].playing = channels[ i ].game_playing = channels[ i ].game_paused = 0;
    if ( device ) SDL_UnlockAudioDevice( device );
}

int STDCALL FMOD__System__close( void *system )     { if ( device ) SDL_PauseAudioDevice( device, 1 ); return 0; }
int STDCALL FMOD__System__release( void *system )   { return 0; }

// ---------------------------------------------------------------------------------------- sounds
static unsigned int le32( const unsigned char *p ) { return p[ 0 ] | p[ 1 ] << 8 | p[ 2 ] << 16 | (unsigned int)p[ 3 ] << 24; }

// RIFF/WAVE, 16-bit PCM or 32-bit float
static int load_wav( sound *s, const unsigned char *data, unsigned int size )
{
    unsigned int at = 12, format = 0, bits = 0;
    if ( size < 44 || memcmp( data, "RIFF", 4 ) != 0 || memcmp( data + 8, "WAVE", 4 ) != 0 )
        return 0;
    while ( at + 8 <= size )
    {
        unsigned int chunk = le32( data + at + 4 );
        const unsigned char *body = data + at + 8;
        if ( chunk > size - at - 8 ) chunk = size - at - 8;
        if ( memcmp( data + at, "fmt ", 4 ) == 0 )
        {
            format = body[ 0 ] | body[ 1 ] << 8;
            s->channels = body[ 2 ] | body[ 3 ] << 8;
            s->rate = le32( body + 4 );
            bits = body[ 14 ] | body[ 15 ] << 8;
        }
        else if ( memcmp( data + at, "data", 4 ) == 0 && s->channels > 0 && ( bits == 16 || bits == 32 ) )
        {
            unsigned int count = chunk / ( bits / 8 );
            s->frames = count / s->channels;
            s->samples = malloc( ( count ? count : 1 ) * sizeof( float ) );
            for ( unsigned int i = 0; i < count; i++ )
            {
                if ( bits == 16 )
                    s->samples[ i ] = (short)( body[ i * 2 ] | body[ i * 2 + 1 ] << 8 ) / 32768.0f;
                else if ( format == 3 )
                    memcpy( &s->samples[ i ], body + i * 4, 4 );
                else
                    s->samples[ i ] = (int)le32( body + i * 4 ) / 2147483648.0f;
            }
            return s->frames > 0;
        }
        at += 8 + chunk + ( chunk & 1 );
    }
    return 0;
}

// mode: 0x800 = the "name" is the file image in memory, with its length in the extra info (second field);
// 2 = loops. That is the only way the game calls it.
int STDCALL FMOD__System__createSound( void *system, const void *name_or_data, unsigned int mode, const unsigned int *extra, void **out )
{
    const unsigned char *data = name_or_data;
    unsigned int size = extra ? extra[ 1 ] : 0;
    sound *s = calloc( 1, sizeof( sound ) );
    if ( s && all_sounds_count < MAX_SOUNDS ) all_sounds[ all_sounds_count++ ] = s;
    *out = 0;
    s->loops = ( mode & 2 ) != 0;
    if ( size >= 4 && memcmp( data, "OggS", 4 ) == 0 )
    {
        s->is_vorbis = 1;
        s->file = malloc( size );
        memcpy( s->file, data, size );          // the game frees its copy; a stream reads this one while playing
        s->file_size = size;
    }
    else if ( !load_wav( s, data, size ) )
    {
        fprintf( stderr, "sound: a sound of %u bytes is neither WAV nor Ogg Vorbis\n", size );
        free( s );
        return 1;
    }
    *out = s;
    return 0;
}

int STDCALL FMOD__Sound__release( void *sound_object )
{
    sound *s = sound_object;
    if ( s == 0 ) return 0;
    if ( device ) SDL_LockAudioDevice( device );
    for ( int i = 0; i < MAX_CHANNELS; i++ )
        if ( channels[ i ].source == s )
        {
            channels[ i ].playing = 0;
            if ( channels[ i ].decoder ) stb_vorbis_close( channels[ i ].decoder );
            channels[ i ].decoder = 0;
            channels[ i ].source = 0;
        }
    if ( device ) SDL_UnlockAudioDevice( device );
    if ( s->in_snapshot )               // the game's tables may be put back to a moment when this sound existed
        return 0;
    for ( int i = 0; i < all_sounds_count; i++ )
        if ( all_sounds[ i ] == s ) { all_sounds[ i ] = all_sounds[ --all_sounds_count ]; break; }
    free( s->samples );
    free( s->file );
    free( s );
    return 0;
}

// The start-state snapshot (draft/native/native_input.c): sounds loaded up to now stay for good.
void snd_mixer_snapshot( void )
{
    for ( int i = 0; i < all_sounds_count; i++ )
        all_sounds[ i ]->in_snapshot = 1;
}

// ---------------------------------------------------------------------------------------- channels
int STDCALL FMOD__System__playSound( void *system, void *sound_object, void *group, int paused, void **out )
{
    sound *s = sound_object;
    channel *ch = 0;
    if ( out ) *out = 0;
    if ( s == 0 ) return 1;
    if ( device ) SDL_LockAudioDevice( device );
    for ( int i = 0; i < MAX_CHANNELS && ch == 0; i++ )     // free by the game's clock, so the same slot in every run
        if ( !game_still_playing( &channels[ i ] ) )
            ch = &channels[ i ];
    if ( ch )
    {
        if ( ch->decoder ) stb_vorbis_close( ch->decoder );
        memset( ch, 0, sizeof( *ch ) );
        ch->source = s;
        ch->volume = 1.0f;
        ch->paused = paused != 0;
        ch->loop_count = s->loops ? -1 : 0;
        ch->rate = s->rate;
        if ( s->is_vorbis )
        {
            int error = 0;
            ch->decoder = stb_vorbis_open_memory( s->file, s->file_size, &error, 0 );
            if ( ch->decoder )
                ch->rate = stb_vorbis_get_info( ch->decoder ).sample_rate;
        }
        ch->playing = !s->is_vorbis || ch->decoder != 0;
        if ( ch->playing )
        {
            double seconds = s->is_vorbis ? stb_vorbis_stream_length_in_seconds( ch->decoder ) : ( s->rate > 0 ? (double)s->frames / s->rate : 0.0 );
            ch->game_playing = 1;
            ch->game_paused = ch->paused;
            ch->game_pause_tick = GAME_TICK;
            ch->game_length = (unsigned int)( seconds * 62.5 ) + 1;
            ch->game_end_tick = GAME_TICK + ch->game_length;
            ch->game_forever = ch->loop_count < 0;
        }
    }
    if ( device ) SDL_UnlockAudioDevice( device );
    if ( ch == 0 || !ch->playing ) return 1;
    if ( out ) *out = ch;
    return 0;
}

static channel *valid( void *handle )
{
    channel *ch = handle;
    return ch >= channels && ch < channels + MAX_CHANNELS ? ch : 0;
}

int STDCALL FMOD__ChannelControl__setVolume( void *handle, float volume )
{
    channel *ch = valid( handle );
    if ( ch ) ch->volume = volume;
    return ch ? 0 : 1;
}

int STDCALL FMOD__ChannelControl__setPaused( void *handle, int paused )
{
    channel *ch = valid( handle );
    if ( ch )
    {
        ch->paused = ( paused & 0xFF ) != 0;
        if ( ch->paused && !ch->game_paused )
            ch->game_pause_tick = GAME_TICK;
        else if ( !ch->paused && ch->game_paused )
            ch->game_end_tick += GAME_TICK - ch->game_pause_tick;       // the pause does not count
        ch->game_paused = ch->paused;
    }
    return ch ? 0 : 1;
}

int STDCALL FMOD__ChannelControl__stop( void *handle )
{
    channel *ch = valid( handle );
    if ( ch ) ch->playing = ch->game_playing = 0;
    return ch ? 0 : 1;
}

int STDCALL FMOD__ChannelControl__isPlaying( void *handle, unsigned char *playing )
{
    channel *ch = valid( handle );
    *playing = ch && game_still_playing( ch );
    return ch ? 0 : 1;
}

int STDCALL FMOD__Channel__setLoopCount( void *handle, int count )
{
    channel *ch = valid( handle );
    if ( ch )
    {
        ch->loop_count = count;
        ch->game_forever = count < 0;
        if ( count > 0 )
            ch->game_end_tick += (unsigned int)count * ch->game_length;
    }
    return ch ? 0 : 1;
}

#endif
