// Hand-written start-up and runtime glue for linking the mechanical draft into a program.
// It replaces what the original gets from SDL2main and the statically linked part of the C runtime.
#include "ILB_draft_decls.h"

void ILB_image_init( void );
double sin( double ); double cos( double ); double sqrt( double );
void *malloc( size_t ); void free( void * ); void abort( void );
long long _ftol2_helper( double );

// the function at 0x581B40 is the game's SDL_main; the work database still calls it gdi_init
// (its second parameter is argv, which the decompiler typed as a registry key)
int main( int argc, char **argv )
{
#ifdef ILB_TRACE
    void ILB_trace_init( void );
    ILB_trace_init();           // trace build: record crashes in ILB_trace.txt (draft/glue/trace.c)
#endif
    ILB_image_init();           // fill the data image and fix its pointers before any game code runs

    // what the original's start-up code (__scrt_initialize_default_local_stdio_options, 0x5832E2) does to the
    // program's own printf / scanf option words: Microsoft's legacy behaviour, where %s in the wide
    // functions (swprintf, ...) is a wide string. Without it every wide-formatted path and message is cut
    // to one character.
    *(unsigned int *)ILB_ADDR( 0x60A020 ) |= 0x24;
    *(unsigned int *)ILB_ADDR( 0x60A008 ) |= 2;
    SDL_SetMainReady();
    return gdi_init( argc, (HKEY)argv );
}

// the math imports take their argument in a register in the original; here they are ordinary functions
double libm_sse2_sin_precise( double x )    { return sin( x ); }
double libm_sse2_cos_precise( double x )    { return cos( x ); }
double libm_sse2_sqrt_precise( double x )   { return sqrt( x ); }

// IDA's names for statically linked runtime routines
void *__cdecl sub_582D30( unsigned int size )   { return malloc( size ); }      // operator new
void __cdecl j_j_free( void *block )            { free( block ); }
int __cdecl j___ftol2_sse_excpt_0( double x )   { return (int)x; }
