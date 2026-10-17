// gge_gl.h - shared declarations of the OpenGL renderer (hand-written), which replaces the game's
// Direct3D rendering layer (the gge_* functions).
#pragma once

typedef struct
{
    const char *name;               // the material type's shader file name, without extension
    const char *vertex_source;
    const char *pixel_source;
} gge_gl_shader_source;

extern const gge_gl_shader_source gge_gl_shader_sources[];

// ---- OpenGL functions the renderer uses, loaded at run time (gge_gl_core.c)
// (on 32-bit Windows OpenGL functions clean up their own arguments; everywhere else it is the plain C convention)
#ifdef _WIN32
#define GGE_GL_CALL __stdcall
#else
#define GGE_GL_CALL
#endif
typedef struct
{
    unsigned int ( GGE_GL_CALL *CreateShader )( unsigned int type );
    void ( GGE_GL_CALL *ShaderSource )( unsigned int shader, int count, const char *const *string, const int *length );
    void ( GGE_GL_CALL *CompileShader )( unsigned int shader );
    void ( GGE_GL_CALL *GetShaderiv )( unsigned int shader, unsigned int pname, int *params );
    void ( GGE_GL_CALL *GetShaderInfoLog )( unsigned int shader, int size, int *length, char *log );
    void ( GGE_GL_CALL *DeleteShader )( unsigned int shader );
    unsigned int ( GGE_GL_CALL *CreateProgram )( void );
    void ( GGE_GL_CALL *AttachShader )( unsigned int program, unsigned int shader );
    void ( GGE_GL_CALL *LinkProgram )( unsigned int program );
    void ( GGE_GL_CALL *GetProgramiv )( unsigned int program, unsigned int pname, int *params );
    void ( GGE_GL_CALL *GetProgramInfoLog )( unsigned int program, int size, int *length, char *log );
    void ( GGE_GL_CALL *UseProgram )( unsigned int program );
    unsigned int ( GGE_GL_CALL *GetUniformBlockIndex )( unsigned int program, const char *name );
    void ( GGE_GL_CALL *UniformBlockBinding )( unsigned int program, unsigned int block, unsigned int binding );
    int ( GGE_GL_CALL *GetUniformLocation )( unsigned int program, const char *name );
    void ( GGE_GL_CALL *Uniform1i )( int location, int value );
    const unsigned char *( GGE_GL_CALL *GetString )( unsigned int name );
    void ( GGE_GL_CALL *GenBuffers )( int n, unsigned int *out );
    void ( GGE_GL_CALL *BindBuffer )( unsigned int target, unsigned int buffer );
    void ( GGE_GL_CALL *BufferData )( unsigned int target, int size, const void *data, unsigned int usage );
    void ( GGE_GL_CALL *BufferSubData )( unsigned int target, int offset, int size, const void *data );
    void ( GGE_GL_CALL *DeleteBuffers )( int n, const unsigned int *names );
    void ( GGE_GL_CALL *BindBufferBase )( unsigned int target, unsigned int index, unsigned int buffer );
    void ( GGE_GL_CALL *GenTextures )( int n, unsigned int *out );
    void ( GGE_GL_CALL *BindTexture )( unsigned int target, unsigned int texture );
    void ( GGE_GL_CALL *TexImage2D )( unsigned int target, int level, int internal, int width, int height, int border, unsigned int format, unsigned int type, const void *data );
    void ( GGE_GL_CALL *TexSubImage2D )( unsigned int target, int level, int x, int y, int width, int height, unsigned int format, unsigned int type, const void *data );
    void ( GGE_GL_CALL *TexParameteri )( unsigned int target, unsigned int name, int value );
    void ( GGE_GL_CALL *DeleteTextures )( int n, const unsigned int *names );
    void ( GGE_GL_CALL *ActiveTexture )( unsigned int unit );
    void ( GGE_GL_CALL *GenSamplers )( int n, unsigned int *out );
    void ( GGE_GL_CALL *SamplerParameteri )( unsigned int sampler, unsigned int name, int value );
    void ( GGE_GL_CALL *BindSampler )( unsigned int unit, unsigned int sampler );
    void ( GGE_GL_CALL *DeleteSamplers )( int n, const unsigned int *names );
    void ( GGE_GL_CALL *GenFramebuffers )( int n, unsigned int *out );
    void ( GGE_GL_CALL *BindFramebuffer )( unsigned int target, unsigned int framebuffer );
    void ( GGE_GL_CALL *FramebufferTexture2D )( unsigned int target, unsigned int attachment, unsigned int textarget, unsigned int texture, int level );
    void ( GGE_GL_CALL *DrawBuffers )( int n, const unsigned int *buffers );
    void ( GGE_GL_CALL *GenVertexArrays )( int n, unsigned int *out );
    void ( GGE_GL_CALL *BindVertexArray )( unsigned int array );
    void ( GGE_GL_CALL *EnableVertexAttribArray )( unsigned int index );
    void ( GGE_GL_CALL *DisableVertexAttribArray )( unsigned int index );
    void ( GGE_GL_CALL *VertexAttribPointer )( unsigned int index, int size, unsigned int type, unsigned char normalized, int stride, const void *offset );
    void ( GGE_GL_CALL *Enable )( unsigned int cap );
    void ( GGE_GL_CALL *Disable )( unsigned int cap );
    void ( GGE_GL_CALL *BlendFuncSeparate )( unsigned int src, unsigned int dst, unsigned int src_alpha, unsigned int dst_alpha );
    void ( GGE_GL_CALL *BlendEquationSeparate )( unsigned int rgb, unsigned int alpha );
    void ( GGE_GL_CALL *ColorMask )( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
    void ( GGE_GL_CALL *Viewport )( int x, int y, int width, int height );
    void ( GGE_GL_CALL *ClearColor )( float r, float g, float b, float a );
    void ( GGE_GL_CALL *Clear )( unsigned int mask );
    void ( GGE_GL_CALL *DrawArrays )( unsigned int mode, int first, int count );
    void ( GGE_GL_CALL *DrawElementsBaseVertex )( unsigned int mode, int count, unsigned int type, const void *indices, int base_vertex );
    void ( GGE_GL_CALL *ClipControl )( unsigned int origin, unsigned int depth );
    void ( GGE_GL_CALL *PixelStorei )( unsigned int name, int value );
    void ( GGE_GL_CALL *DepthFunc )( unsigned int func );
    void ( GGE_GL_CALL *DepthMask )( unsigned char write );
    void ( GGE_GL_CALL *ClearDepthf )( float depth );
    unsigned int ( GGE_GL_CALL *CheckFramebufferStatus )( unsigned int target );
    unsigned int ( GGE_GL_CALL *GetError )( void );
    void ( GGE_GL_CALL *ReadPixels )( int x, int y, int width, int height, unsigned int format, unsigned int type, void *data );
    // a joiner's picture drawn at the host's size and scaled into its window (gge_gl_backend.c)
    void ( GGE_GL_CALL *BlitFramebuffer )( int x0, int y0, int x1, int y1, int dx0, int dy0, int dx1, int dy1, unsigned int mask, unsigned int filter );
    void ( GGE_GL_CALL *GenRenderbuffers )( int count, unsigned int *names );
    void ( GGE_GL_CALL *BindRenderbuffer )( unsigned int target, unsigned int name );
    void ( GGE_GL_CALL *RenderbufferStorage )( unsigned int target, unsigned int format, int width, int height );
    void ( GGE_GL_CALL *FramebufferRenderbuffer )( unsigned int target, unsigned int attachment, unsigned int kind, unsigned int name );
} gge_gl_functions;

extern gge_gl_functions gl;

typedef struct
{
    const char *name;
    unsigned int program;
} gge_gl_program;

#define GGE_GL_MAX_PROGRAMS 16
extern gge_gl_program gge_gl_programs[ GGE_GL_MAX_PROGRAMS ];
extern int gge_gl_num_programs;

int gge_gl_load( void );                // after a context exists; 0 if a needed function is missing
void gge_gl_load_nothing( void );       // headless build: every OpenGL call does nothing
int gge_gl_build_programs( void );      // compiles and links all shader pairs; 0 if any failed
const gge_gl_program *gge_gl_find_program( const char *name );
