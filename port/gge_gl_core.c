// gge_gl_core.c - OpenGL access for the renderer (hand-written): the function table and shader programs.
//
// OpenGL is reached only through the `gl` table, filled from SDL_GL_GetProcAddress once a context exists.
// (The headless debug build will fill the same table with do-nothing functions.)
#include "gge_gl.h"

typedef struct _IO_FILE FILE;
extern FILE *stderr;
int fprintf( FILE *, const char *, ... );
int strcmp( const char *, const char * );
void *SDL_GL_GetProcAddress( const char *name );

gge_gl_functions gl;
gge_gl_program gge_gl_programs[ GGE_GL_MAX_PROGRAMS ];
int gge_gl_num_programs;

#define LOAD( name ) gl.name = SDL_GL_GetProcAddress( "gl" #name ); if ( gl.name == 0 ) { fprintf( stderr, "gge_gl: no gl" #name "\n" ); ok = 0; }

int gge_gl_load( void )
{
    int ok = 1;
    LOAD( CreateShader ) LOAD( ShaderSource ) LOAD( CompileShader ) LOAD( GetShaderiv ) LOAD( GetShaderInfoLog ) LOAD( DeleteShader )
    LOAD( CreateProgram ) LOAD( AttachShader ) LOAD( LinkProgram ) LOAD( GetProgramiv ) LOAD( GetProgramInfoLog ) LOAD( UseProgram )
    LOAD( GetUniformBlockIndex ) LOAD( UniformBlockBinding ) LOAD( GetUniformLocation ) LOAD( Uniform1i )
    LOAD( GetString )
    LOAD( DepthFunc ) LOAD( DepthMask ) LOAD( ClearDepthf ) LOAD( CheckFramebufferStatus ) LOAD( GetError )
    LOAD( GenBuffers ) LOAD( BindBuffer ) LOAD( BufferData ) LOAD( BufferSubData ) LOAD( DeleteBuffers )
    LOAD( BindBufferBase ) LOAD( GenTextures ) LOAD( BindTexture ) LOAD( TexImage2D ) LOAD( TexSubImage2D )
    LOAD( TexParameteri ) LOAD( DeleteTextures ) LOAD( ActiveTexture ) LOAD( GenSamplers ) LOAD( SamplerParameteri )
    LOAD( BindSampler ) LOAD( DeleteSamplers ) LOAD( GenFramebuffers ) LOAD( BindFramebuffer ) LOAD( FramebufferTexture2D )
    LOAD( DrawBuffers ) LOAD( GenVertexArrays ) LOAD( BindVertexArray ) LOAD( EnableVertexAttribArray ) LOAD( DisableVertexAttribArray )
    LOAD( VertexAttribPointer ) LOAD( Enable ) LOAD( Disable ) LOAD( BlendFuncSeparate ) LOAD( BlendEquationSeparate )
    LOAD( ColorMask ) LOAD( Viewport ) LOAD( ClearColor ) LOAD( Clear ) LOAD( DrawArrays )
    LOAD( DrawElementsBaseVertex ) LOAD( ClipControl ) LOAD( PixelStorei ) LOAD( ReadPixels )
    return ok;
}

// The headless debug build has no OpenGL: every entry of the table becomes a function that does nothing
// and returns 0, so the renderer code above it runs unchanged.
static int nothing( void ) { return 0; }

void gge_gl_load_nothing( void )
{
    void **entry = (void **)&gl;
    for ( unsigned int i = 0; i < sizeof( gl ) / sizeof( void * ); i++ )
        entry[ i ] = (void *)nothing;
}

static unsigned int compile( unsigned int kind, const char *source, const char *what, const char *name )
{
    unsigned int shader = gl.CreateShader( kind );
    int status = 0;
    gl.ShaderSource( shader, 1, &source, 0 );
    gl.CompileShader( shader );
    gl.GetShaderiv( shader, 0x8B81, &status );              // GL_COMPILE_STATUS
    if ( !status )
    {
        char log[ 2048 ];
        gl.GetShaderInfoLog( shader, sizeof( log ), 0, log );
        fprintf( stderr, "gge_gl: %s shader of '%s' does not compile:\n%s\n", what, name, log );
        return 0;
    }
    return shader;
}

// Builds every shader pair. The two constant blocks get fixed binding points (0 matrix_buffer, 1 global_data,
// the slots the game uses), the two textures units 0 and 1.
int gge_gl_build_programs( void )
{
    int failed = 0;
    gge_gl_num_programs = 0;
    for ( const gge_gl_shader_source *s = gge_gl_shader_sources; s->name; s++ )
    {
        gge_gl_program *p = &gge_gl_programs[ gge_gl_num_programs++ ];
        unsigned int vs = compile( 0x8B31, s->vertex_source, "vertex", s->name );      // GL_VERTEX_SHADER
        unsigned int ps = compile( 0x8B30, s->pixel_source, "pixel", s->name );        // GL_FRAGMENT_SHADER
        int status = 0;
        p->name = s->name;
        p->program = 0;
        if ( vs == 0 || ps == 0 ) { failed++; continue; }
        p->program = gl.CreateProgram();
        gl.AttachShader( p->program, vs );
        gl.AttachShader( p->program, ps );
        gl.LinkProgram( p->program );
        gl.GetProgramiv( p->program, 0x8B82, &status );     // GL_LINK_STATUS
        if ( !status )
        {
            char log[ 2048 ];
            gl.GetProgramInfoLog( p->program, sizeof( log ), 0, log );
            fprintf( stderr, "gge_gl: program '%s' does not link:\n%s\n", s->name, log );
            p->program = 0;
            failed++;
            continue;
        }
        unsigned int block = gl.GetUniformBlockIndex( p->program, "matrix_buffer" );
        if ( block != 0xFFFFFFFFu ) gl.UniformBlockBinding( p->program, block, 0 );
        block = gl.GetUniformBlockIndex( p->program, "global_data" );
        if ( block != 0xFFFFFFFFu ) gl.UniformBlockBinding( p->program, block, 1 );
        gl.UseProgram( p->program );
        int location = gl.GetUniformLocation( p->program, "shader_texture" );
        if ( location >= 0 ) gl.Uniform1i( location, 0 );
        location = gl.GetUniformLocation( p->program, "shader_texture1" );
        if ( location >= 0 ) gl.Uniform1i( location, 1 );
    }
    gl.UseProgram( 0 );
    fprintf( stderr, "gge_gl: %s, %d of %d shader programs built\n", (const char *)gl.GetString( 0x1F02 ),    // GL_VERSION
             gge_gl_num_programs - failed, gge_gl_num_programs );
    return failed == 0;
}

const gge_gl_program *gge_gl_find_program( const char *name )
{
    for ( int i = 0; i < gge_gl_num_programs; i++ )
        if ( strcmp( gge_gl_programs[ i ].name, name ) == 0 )
            return &gge_gl_programs[ i ];
    return 0;
}
