// gge_gl_backend.c - the OpenGL implementation of what the rendering layer needs (hand-written).
// See gge_gl_backend.h. No Direct3D here: objects are this file's own, drawing is OpenGL 4.5.
//
// Conventions the game's layer was written for, and how they are met:
//  - texture row 0 is the top row and v = 0 is the top: textures are uploaded as they come, which gives that;
//  - rendering into a texture puts clip-space y = +1 in row 0: glClipControl( upper left ) while a texture is
//    the target, the default (lower left) for the window;
//  - clip-space z runs 0..1 (glClipControl again); the game layers its world by depth, so the depth states
//    and the depth buffer are honoured.
#include "gge_gl.h"
#include "gge_gl_backend.h"
#include "gge_gl_shader_ids.h"

typedef unsigned int size_t;
typedef struct _IO_FILE FILE;
extern FILE *stderr;
int fprintf( FILE *, const char *, ... );
void *calloc( size_t, size_t );
void *malloc( size_t );
void free( void * );
void *memcpy( void *, const void *, size_t );
int strcmp( const char *, const char * );
void SDL_GL_SwapWindow( void *window );
void SDL_GL_GetDrawableSize( void *window, int *w, int *h );
extern void *ILB_window;

// the operation in progress and its main numbers, for a debugger when the graphics driver faults
const char *gge_gl_last_call;
unsigned int gge_gl_last_numbers[ 4 ];
#define CALL( a, b, c, d ) ( gge_gl_last_call = __func__, gge_gl_last_numbers[ 0 ] = (unsigned int)( a ), gge_gl_last_numbers[ 1 ] = (unsigned int)( b ), \
                             gge_gl_last_numbers[ 2 ] = (unsigned int)( c ), gge_gl_last_numbers[ 3 ] = (unsigned int)( d ) )

enum { kind_device = 1, kind_buffer, kind_texture, kind_view, kind_blend, kind_sampler, kind_layout, kind_shader, kind_other };

typedef struct gl_object
{
    int kind;
    int references;
    unsigned int name;              // the OpenGL object
    // buffer: size, bind flags (1 vertex, 2 index, 4 constants), CPU copy
    // texture: width, height, CPU copy while mapped; backbuffer: name 0, is_backbuffer
    unsigned int size, bind_flags, width, height;
    int is_backbuffer, is_depth;
    unsigned char *copy;
    struct gl_object *texture;      // view: the texture it shows
    // blend state
    int blend_enable;
    unsigned int src, dst, op, src_alpha, dst_alpha, op_alpha, write_mask;
    // depth state
    int depth_enable, depth_write;
    unsigned int depth_func;
    // input layout
    int num_elements;
    struct { int location, components; unsigned int type; int normalized; unsigned int offset; } elements[ 8 ];
    // shader
    const char *shader_name;
    // the start-state snapshot (gge_gl_snapshot): every object is in one list; those that existed when the
    // picture was taken are never really destroyed, so that the game's tables can be put back to that moment
    struct gl_object *next_in_all;
    int in_snapshot, references_in_snapshot;
} gl_object;
static gl_object *all_objects;

static struct
{
    gl_object *layout, *vertex_buffer, *index_buffer, *pixel_shader, *blend, *depth, *depth_view;
    gl_object *constants[ 4 ], *textures[ 4 ], *samplers[ 4 ], *targets[ 2 ];
    unsigned int stride, vertex_offset, index_offset, index_type;
    unsigned int vao, framebuffer;
    int target_is_window;
    float viewport[ 4 ];
    int dirty_targets;
} state;

static gl_object *new_object( int kind )
{
    gl_object *o = calloc( 1, sizeof( gl_object ) );
    o->kind = kind;
    o->references = 1;
    o->next_in_all = all_objects;
    all_objects = o;
    return o;
}

// An object that is going away may still be the one bound for drawing (the game frees its render targets
// while their views are still set as textures); nothing may keep pointing at it.
static void forget( const gl_object *o )
{
    gl_object **bound = (gl_object **)&state;
    // the pointers at the start of the state: layout .. depth_view, constants, textures, samplers, targets
    for ( unsigned int i = 0; i < ( 7 + 4 + 4 + 4 + 2 ); i++ )
        if ( bound[ i ] == o )
            bound[ i ] = 0;
    state.dirty_targets = 1;
}

unsigned int gge_gl_Release( void *object )
{
    gl_object *o = object;
    if ( o == 0 || o->references <= 0 || --o->references > 0 )
        return o && o->references > 0 ? o->references : 0;
    if ( o->in_snapshot )               // gone as far as the game knows; kept for gge_gl_restore
    {
        forget( o );
        return 0;
    }
    for ( gl_object **link = &all_objects; *link; link = &( *link )->next_in_all )
        if ( *link == o ) { *link = o->next_in_all; break; }
    if ( o->kind == kind_buffer && o->name ) gl.DeleteBuffers( 1, &o->name );
    if ( o->kind == kind_texture && o->name ) gl.DeleteTextures( 1, &o->name );
    if ( o->kind == kind_sampler && o->name ) gl.DeleteSamplers( 1, &o->name );
    if ( o->kind == kind_view ) gge_gl_Release( o->texture );
    forget( o );
    free( o->copy );
    free( o );
    return 0;
}

// The start-state snapshot (draft/native/native_input.c): remember which objects exist and how often each is
// referenced; and put that back later - objects made since are destroyed, the remembered ones are as alive
// as they were. What is IN the remembered textures and buffers is whatever was last put there.
void gge_gl_snapshot( void )
{
    for ( gl_object *o = all_objects; o; o = o->next_in_all )
    {
        o->in_snapshot = 1;
        o->references_in_snapshot = o->references;
    }
}

void gge_gl_restore( void )
{
    gl_object **link = &all_objects;
    memset( &state, 0, (char *)&state.stride - (char *)&state );       // nothing bound
    state.dirty_targets = 1;
    while ( *link )
    {
        gl_object *o = *link;
        if ( o->in_snapshot )
        {
            o->references = o->references_in_snapshot;
            link = &o->next_in_all;
            continue;
        }
        *link = o->next_in_all;
        if ( o->kind == kind_buffer && o->name ) gl.DeleteBuffers( 1, &o->name );
        if ( o->kind == kind_texture && o->name ) gl.DeleteTextures( 1, &o->name );
        if ( o->kind == kind_sampler && o->name ) gl.DeleteSamplers( 1, &o->name );
        free( o->copy );
        free( o );
    }
}

int gge_gl_create_device( void *adapter, int driver, void *software, unsigned int flags, const void *levels, unsigned int num_levels,
                          unsigned int sdk, const void *swap_chain_desc, void **swap_chain, void **device, void *level, void **context )
{
    *swap_chain = new_object( kind_device );
    *device = new_object( kind_device );
    *context = new_object( kind_device );
    gl.GenVertexArrays( 1, &state.vao );
    gl.BindVertexArray( state.vao );
    gl.GenFramebuffers( 1, &state.framebuffer );
    gl.Disable( 0x0B71 );           // GL_DEPTH_TEST
    gl.Disable( 0x0B44 );           // GL_CULL_FACE
    gl.PixelStorei( 0x0CF5, 1 );    // GL_UNPACK_ALIGNMENT
    state.target_is_window = 1;
    return 0;
}

// ---------------------------------------------------------------------------------------- buffers
// desc: ByteWidth, Usage, BindFlags, CPUAccessFlags, MiscFlags, StructureByteStride
static unsigned int buffer_target( const gl_object *b )
{
    return ( b->bind_flags & 2 ) ? 0x8893 : ( b->bind_flags & 4 ) ? 0x8A11 : 0x8892;   // ELEMENT_ARRAY, UNIFORM, ARRAY
}

int gge_gl_CreateBuffer( void *device, const void *desc, const void *initial, void **buffer )
{
    CALL( ( (const unsigned int *)desc )[ 0 ], ( (const unsigned int *)desc )[ 2 ], initial, 0 );
    const unsigned int *d = desc;
    gl_object *b = new_object( kind_buffer );
    b->size = d[ 0 ];
    b->bind_flags = d[ 2 ];
    b->copy = calloc( 1, b->size ? b->size : 1 );
    if ( initial && *(const void *const *)initial )
        memcpy( b->copy, *(const void *const *)initial, b->size );          // D3D11_SUBRESOURCE_DATA.pSysMem
    gl.GenBuffers( 1, &b->name );
    gl.BindBuffer( buffer_target( b ), b->name );
    gl.BufferData( buffer_target( b ), b->size, b->copy, 0x88E8 );          // GL_DYNAMIC_DRAW
    *buffer = b;
    return 0;
}

// ---------------------------------------------------------------------------------------- textures
// desc: Width, Height, MipLevels, ArraySize, Format, SampleDesc.Count, .Quality, Usage, BindFlags, CPUAccess, Misc
int gge_gl_CreateTexture2D( void *device, const void *desc, const void *initial, void **texture )
{
    CALL( ( (const unsigned int *)desc )[ 0 ], ( (const unsigned int *)desc )[ 1 ], ( (const unsigned int *)desc )[ 4 ], initial );
    const unsigned int *d = desc;
    gl_object *t = new_object( kind_texture );
    t->width = d[ 0 ];
    t->height = d[ 1 ];
    t->bind_flags = d[ 8 ];
    gl.GenTextures( 1, &t->name );
    gl.BindTexture( 0x0DE1, t->name );                                      // GL_TEXTURE_2D
    if ( d[ 4 ] == 55 || d[ 4 ] == 45 || d[ 4 ] == 40 )                     // D16, D24S8, D32F: a depth buffer, only ever attached
    {
        t->is_depth = 1;
        gl.TexImage2D( 0x0DE1, 0, d[ 4 ] == 55 ? 0x81A5 : 0x81A6, t->width, t->height, 0, 0x1902, 0x1405, 0 );    // DEPTH_COMPONENT16 / 24
    }
    else
    {
        // A texture made without contents starts out as zeros, as Direct3D's do in practice. OpenGL leaves them
        // undefined, and some drivers (Intel's on Windows) hand out junk: a new world's maps that the game
        // fills in only later then covered the picture.
        int is_float = d[ 4 ] == 2;                                         // R32G32B32A32_FLOAT: the game's render targets
        const void *pixels = initial ? *(const void *const *)initial : 0;
        void *zeros = 0;
        if ( pixels == 0 && t->width > 0 && t->height > 0 )
            pixels = zeros = calloc( (size_t)t->width * t->height, is_float ? 16 : 4 );
        if ( is_float )
            gl.TexImage2D( 0x0DE1, 0, 0x8814, t->width, t->height, 0, 0x1908, 0x1406, pixels );    // RGBA32F
        else
            gl.TexImage2D( 0x0DE1, 0, 0x8058, t->width, t->height, 0, 0x80E1, 0x1401, pixels );    // RGBA8 from BGRA bytes
        free( zeros );
    }
    gl.TexParameteri( 0x0DE1, 0x2801, 0x2600 );                             // MIN_FILTER nearest (no mip maps exist)
    gl.TexParameteri( 0x0DE1, 0x2800, 0x2600 );
    gl.TexParameteri( 0x0DE1, 0x813D, 0 );                                  // MAX_LEVEL 0
    *texture = t;
    return 0;
}

static int new_view( void *resource, void **view )
{
    gl_object *v = new_object( kind_view );
    v->texture = resource;
    if ( v->texture ) v->texture->references++;         // a view keeps its texture alive, as in the original
    *view = v;
    return 0;
}

int gge_gl_CreateShaderResourceView( void *device, void *resource, const void *desc, void **view ) { return new_view( resource, view ); }
int gge_gl_CreateRenderTargetView( void *device, void *resource, const void *desc, void **view )   { return new_view( resource, view ); }
int gge_gl_CreateDepthStencilView( void *device, void *resource, const void *desc, void **view )   { return new_view( resource, view ); }
// desc: DepthEnable, DepthWriteMask, DepthFunc, StencilEnable, ...
int gge_gl_CreateDepthStencilState( void *device, const void *desc, void **state_out )
{
    static const unsigned int funcs[] = { 0x0207, 0x0200 /*NEVER*/, 0x0201 /*LESS*/, 0x0202 /*EQUAL*/, 0x0203 /*LEQUAL*/,
                                          0x0204 /*GREATER*/, 0x0205 /*NOTEQUAL*/, 0x0206 /*GEQUAL*/, 0x0207 /*ALWAYS*/ };
    const unsigned int *d = desc;
    gl_object *o = new_object( kind_other );
    o->depth_enable = d[ 0 ] != 0;
    o->depth_write = d[ 1 ] != 0;
    o->depth_func = funcs[ d[ 2 ] < 9 ? d[ 2 ] : 8 ];
    *state_out = o;
    return 0;
}
int gge_gl_CreateRasterizerState( void *device, const void *desc, void **state_out )               { *state_out = new_object( kind_other ); return 0; }

// the window's own picture, as a texture object the game makes a render target view of
int gge_gl_GetBuffer( void *swap_chain, unsigned int index, const void *interface_id, void **surface )
{
    gl_object *t = new_object( kind_texture );
    t->is_backbuffer = 1;
    *surface = t;
    return 0;
}

// ---------------------------------------------------------------------------------------- states
static unsigned int blend_factor( unsigned int d3d )
{
    static const unsigned int table[] = { 0, 0 /*ZERO*/, 1 /*ONE*/, 0x0300, 0x0301, 0x0302, 0x0303, 0x0304, 0x0305, 0x0306, 0x0307 };
    return d3d < 11 ? table[ d3d ] : 1;
}

static unsigned int blend_op( unsigned int d3d )
{
    static const unsigned int table[] = { 0x8006, 0x8006 /*ADD*/, 0x800A /*SUBTRACT*/, 0x800B /*REVERSE*/, 0x8007 /*MIN*/, 0x8008 /*MAX*/ };
    return d3d < 6 ? table[ d3d ] : 0x8006;
}

// desc: AlphaToCoverage, IndependentBlend, then per target: enable, src, dst, op, src alpha, dst alpha, op alpha, mask
int gge_gl_CreateBlendState( void *device, const void *desc, void **state_out )
{
    const unsigned int *target = (const unsigned int *)desc + 2;
    gl_object *b = new_object( kind_blend );
    b->blend_enable = target[ 0 ] != 0;
    b->src = blend_factor( target[ 1 ] );
    b->dst = blend_factor( target[ 2 ] );
    b->op = blend_op( target[ 3 ] );
    b->src_alpha = blend_factor( target[ 4 ] );
    b->dst_alpha = blend_factor( target[ 5 ] );
    b->op_alpha = blend_op( target[ 6 ] );
    b->write_mask = target[ 7 ] & 0xFF;
    *state_out = b;
    return 0;
}

// desc: Filter, AddressU, AddressV, AddressW, ...
int gge_gl_CreateSamplerState( void *device, const void *desc, void **state_out )
{
    const unsigned int *d = desc;
    gl_object *s = new_object( kind_sampler );
    unsigned int filter = d[ 0 ] == 0 ? 0x2600 : 0x2601;                    // point : linear
    unsigned int wrap_u = d[ 1 ] == 1 ? 0x2901 : d[ 1 ] == 2 ? 0x8370 : 0x812F;         // repeat, mirror, clamp to edge
    unsigned int wrap_v = d[ 2 ] == 1 ? 0x2901 : d[ 2 ] == 2 ? 0x8370 : 0x812F;
    gl.GenSamplers( 1, &s->name );
    gl.SamplerParameteri( s->name, 0x2801, filter );
    gl.SamplerParameteri( s->name, 0x2800, filter );
    gl.SamplerParameteri( s->name, 0x2802, wrap_u );
    gl.SamplerParameteri( s->name, 0x2803, wrap_v );
    *state_out = s;
    return 0;
}

// elements: SemanticName, SemanticIndex, Format, InputSlot, AlignedByteOffset, InputSlotClass, StepRate (28 bytes)
int gge_gl_CreateInputLayout( void *device, const void *elements, unsigned int num_elements, const void *shader, unsigned int shader_size, void **layout )
{
    gl_object *l = new_object( kind_layout );
    unsigned int running = 0;
    for ( unsigned int i = 0; i < num_elements && i < 8; i++ )
    {
        const unsigned int *e = (const unsigned int *)elements + 7 * i;
        const char *semantic = (const char *)e[ 0 ];
        unsigned int format = e[ 2 ], size;
        int location = strcmp( semantic, "TEXCOORD" ) == 0 ? 1 : strcmp( semantic, "BLENDWEIGHT" ) == 0 ? 2 : strcmp( semantic, "COLOR" ) == 0 ? 3 : 0;
        l->elements[ i ].location = location;
        l->elements[ i ].type = 0x1406;                                     // GL_FLOAT
        l->elements[ i ].normalized = 0;
        switch ( format )
        {
            case 2:  l->elements[ i ].components = 4; size = 16; break;     // R32G32B32A32_FLOAT
            case 6:  l->elements[ i ].components = 3; size = 12; break;     // R32G32B32_FLOAT
            case 16: l->elements[ i ].components = 2; size = 8; break;      // R32G32_FLOAT
            case 41: l->elements[ i ].components = 1; size = 4; break;      // R32_FLOAT
            case 28: case 87:                                               // 4 bytes, 0..1
                l->elements[ i ].components = 4; l->elements[ i ].type = 0x1401; l->elements[ i ].normalized = 1; size = 4; break;
            default:
                fprintf( stderr, "gge_gl: vertex format %u for %s is not handled\n", format, semantic );
                l->elements[ i ].components = 4; size = 16; break;
        }
        l->elements[ i ].offset = e[ 4 ] == 0xFFFFFFFFu ? running : e[ 4 ];     // D3D11_APPEND_ALIGNED_ELEMENT
        running = l->elements[ i ].offset + size;
        l->num_elements++;
    }
    *layout = l;
    return 0;
}

static int new_shader( const void *shader, unsigned int size, int pixel, void **out )
{
    gl_object *s = new_object( kind_shader );
    unsigned int hash = 0x811C9DC5;
    for ( unsigned int i = 0; i < size; i++ )
        hash = ( hash ^ ( (const unsigned char *)shader )[ i ] ) * 0x01000193;
    for ( unsigned int i = 0; i < sizeof( gge_gl_shader_ids ) / sizeof( gge_gl_shader_ids[ 0 ] ); i++ )
        if ( gge_gl_shader_ids[ i ].hash == hash && gge_gl_shader_ids[ i ].size == size && gge_gl_shader_ids[ i ].pixel == pixel )
            s->shader_name = gge_gl_shader_ids[ i ].name;
    if ( s->shader_name == 0 )
        fprintf( stderr, "gge_gl: a %s shader of %u bytes is not one of the game's known shaders\n", pixel ? "pixel" : "vertex", size );
    *out = s;
    return 0;
}

int gge_gl_CreateVertexShader( void *device, const void *shader, unsigned int size, void *linkage, void **out ) { return new_shader( shader, size, 0, out ); }
int gge_gl_CreatePixelShader( void *device, const void *shader, unsigned int size, void *linkage, void **out )  { return new_shader( shader, size, 1, out ); }

// ---------------------------------------------------------------------------------------- binding
int gge_gl_IASetInputLayout( void *context, void *layout )         { state.layout = layout; return 0; }
int gge_gl_IASetPrimitiveTopology( void *context, int topology )   { return 0; }                 // always triangle lists
int gge_gl_VSSetShader( void *context, void *shader, void *instances, unsigned int count ) { return 0; }    // the pixel shader names the pair
int gge_gl_PSSetShader( void *context, void *shader, void *instances, unsigned int count ) { state.pixel_shader = shader; return 0; }
int gge_gl_OMSetBlendState( void *context, void *blend, const float *factor, unsigned int mask ) { state.blend = blend; return 0; }
int gge_gl_OMSetDepthStencilState( void *context, void *depth, unsigned int reference ) { state.depth = depth; return 0; }
int gge_gl_RSSetState( void *context, void *rasterizer )           { return 0; }

int gge_gl_IASetVertexBuffers( void *context, unsigned int slot, unsigned int count, void *const *buffers, const unsigned int *strides, const unsigned int *offsets )
{
    state.vertex_buffer = buffers[ 0 ];
    state.stride = strides[ 0 ];
    state.vertex_offset = offsets[ 0 ];
    return 0;
}

int gge_gl_IASetIndexBuffer( void *context, void *buffer, int format, unsigned int offset )
{
    state.index_buffer = buffer;
    state.index_type = format == 57 ? 0x1403 : 0x1405;                      // R16_UINT : R32_UINT
    state.index_offset = offset;
    return 0;
}

static void set_constants( unsigned int slot, unsigned int count, void *const *buffers )
{
    for ( unsigned int i = 0; i < count && slot + i < 4; i++ )
        state.constants[ slot + i ] = buffers[ i ];
}
int gge_gl_VSSetConstantBuffers( void *context, unsigned int slot, unsigned int count, void *const *buffers ) { set_constants( slot, count, buffers ); return 0; }
int gge_gl_PSSetConstantBuffers( void *context, unsigned int slot, unsigned int count, void *const *buffers ) { set_constants( slot, count, buffers ); return 0; }

int gge_gl_PSSetShaderResources( void *context, unsigned int slot, unsigned int count, void *const *views )
{
    for ( unsigned int i = 0; i < count && slot + i < 4; i++ )
        state.textures[ slot + i ] = views[ i ];
    return 0;
}

int gge_gl_PSSetSamplers( void *context, unsigned int slot, unsigned int count, void *const *samplers )
{
    for ( unsigned int i = 0; i < count && slot + i < 4; i++ )
        state.samplers[ slot + i ] = samplers[ i ];
    return 0;
}

// viewport: TopLeftX, TopLeftY, Width, Height, MinDepth, MaxDepth (floats); applied with the render target
int ILB_input_view_offset( void );

int gge_gl_RSSetViewports( void *context, unsigned int count, const void *viewports )
{
    memcpy( state.viewport, viewports, sizeof( state.viewport ) );
    state.dirty_targets = 1;
    return 0;
}

int gge_gl_OMSetRenderTargets( void *context, unsigned int count, void *const *views, void *depth_view )
{
    state.targets[ 0 ] = count > 0 ? views[ 0 ] : 0;
    state.targets[ 1 ] = count > 1 ? views[ 1 ] : 0;
    state.depth_view = depth_view;
    state.dirty_targets = 1;
    return 0;
}

// makes the current render targets and viewport OpenGL's
static void apply_targets( void )
{
    static const unsigned int both[ 2 ] = { 0x8CE0, 0x8CE1 };              // COLOR_ATTACHMENT0, 1
    gl_object *first = state.targets[ 0 ] ? state.targets[ 0 ]->texture : 0;
    int window_height = 0, window_width = 0;
    static int complained;
    if ( !state.dirty_targets )
        return;
    state.dirty_targets = 0;
    state.target_is_window = first == 0 || first->is_backbuffer;
    if ( state.target_is_window )
    {
        gl.BindFramebuffer( 0x8D40, 0 );
        gl.ClipControl( 0x8CA1, 0x935F );                                   // lower left, depth 0..1
        SDL_GL_GetDrawableSize( ILB_window, &window_width, &window_height );
        // two copies of a network game each show one of the two views: the picture is shifted so that this
        // copy's half lands on the window (draft/native/native_input.c, "one view per copy")
        gl.Viewport( (int)state.viewport[ 0 ] - ILB_input_view_offset(), window_height - (int)( state.viewport[ 1 ] + state.viewport[ 3 ] ),
                     (int)state.viewport[ 2 ], (int)state.viewport[ 3 ] );
        return;
    }
    gl.BindFramebuffer( 0x8D40, state.framebuffer );
    gl.FramebufferTexture2D( 0x8D40, 0x8CE0, 0x0DE1, first->name, 0 );
    gl.FramebufferTexture2D( 0x8D40, 0x8CE1, 0x0DE1, state.targets[ 1 ] && state.targets[ 1 ]->texture ? state.targets[ 1 ]->texture->name : 0, 0 );
    gl.FramebufferTexture2D( 0x8D40, 0x8D00, 0x0DE1,                        // DEPTH_ATTACHMENT
                             state.depth_view && state.depth_view->texture && state.depth_view->texture->is_depth ? state.depth_view->texture->name : 0, 0 );
    if ( gl.CheckFramebufferStatus( 0x8D40 ) != 0x8CD5 && !complained )     // FRAMEBUFFER_COMPLETE
    {
        complained = 1;
        fprintf( stderr, "gge_gl: a render target set is not usable (status %x; colour %ux%u)\n",
                 gl.CheckFramebufferStatus( 0x8D40 ), first->width, first->height );
    }
    gl.DrawBuffers( state.targets[ 1 ] ? 2 : 1, both );
    gl.ClipControl( 0x8CA2, 0x935F );                                       // upper left: row 0 is the top, as the game expects
    gl.Viewport( (int)state.viewport[ 0 ], (int)state.viewport[ 1 ], (int)state.viewport[ 2 ], (int)state.viewport[ 3 ] );
}

int gge_gl_ClearRenderTargetView( void *context, void *view, const float *color )
{
    CALL( view, 0, 0, 0 );
    gl_object *v = view, *saved0 = state.targets[ 0 ], *saved1 = state.targets[ 1 ];
    // cleared through the frame buffer: make this view the only target for the moment
    state.targets[ 0 ] = v;
    state.targets[ 1 ] = 0;
    state.dirty_targets = 1;
    apply_targets();
    gl.ColorMask( 1, 1, 1, 1 );
    gl.ClearColor( color[ 0 ], color[ 1 ], color[ 2 ], color[ 3 ] );
    gl.Clear( 0x4000 );
    state.targets[ 0 ] = saved0;
    state.targets[ 1 ] = saved1;
    state.dirty_targets = 1;
    return 0;
}

// the game clears the depth buffer of the targets it has just set
int gge_gl_ClearDepthStencilView( void *context, void *view, unsigned int flags, float depth, unsigned char stencil )
{
    apply_targets();
    gl.DepthMask( 1 );
    gl.ClearDepthf( depth );
    gl.Clear( 0x0100 | 0x0400 );            // depth and stencil
    return 0;
}

// ---------------------------------------------------------------------------------------- data
// mapped: pData, RowPitch, DepthPitch. The game writes into this object's copy; Unmap sends it to OpenGL.
int gge_gl_Map( void *context, void *resource, unsigned int subresource, int type, unsigned int flags, void *mapped )
{
    CALL( resource, type, 0, 0 );
    gl_object *r = resource;
    unsigned int *m = mapped;
    if ( r->kind == kind_texture && r->copy == 0 )
        r->copy = calloc( 1, r->width * r->height * 4 ? r->width * r->height * 4 : 4 );
    m[ 0 ] = (unsigned int)r->copy;
    m[ 1 ] = r->kind == kind_texture ? r->width * 4 : r->size;
    m[ 2 ] = 0;
    return 0;
}

int gge_gl_Unmap( void *context, void *resource, unsigned int subresource )
{
    CALL( resource, ( (gl_object *)resource )->kind, ( (gl_object *)resource )->size, ( (gl_object *)resource )->width );
    gl_object *r = resource;
    if ( r->kind == kind_texture )
    {
        gl.BindBuffer( 0x88EC, 0 );                                         // no pixel unpack buffer: data comes from memory
        gl.BindTexture( 0x0DE1, r->name );
        gl.TexSubImage2D( 0x0DE1, 0, 0, 0, r->width, r->height, 0x80E1, 0x1401, r->copy );
    }
    else
    {
        gl.BindBuffer( buffer_target( r ), r->name );
        gl.BufferSubData( buffer_target( r ), 0, r->size, r->copy );
    }
    return 0;
}

int gge_gl_UpdateSubresource( void *context, void *resource, unsigned int subresource, const void *box, const void *data, unsigned int row_pitch, unsigned int depth_pitch )
{
    CALL( resource, ( (gl_object *)resource )->kind, ( (gl_object *)resource )->size, data );
    gl_object *r = resource;
    if ( r->kind == kind_texture )
    {
        gl.BindTexture( 0x0DE1, r->name );
        gl.TexSubImage2D( 0x0DE1, 0, 0, 0, r->width, r->height, 0x80E1, 0x1401, data );
        return 0;
    }
    memcpy( r->copy, data, r->size );
    gl.BindBuffer( buffer_target( r ), r->name );
    gl.BufferSubData( buffer_target( r ), 0, r->size, r->copy );
    return 0;
}

// ---------------------------------------------------------------------------------------- drawing
static int prepare_draw( void )
{
    const gge_gl_program *program = state.pixel_shader && state.pixel_shader->shader_name ? gge_gl_find_program( state.pixel_shader->shader_name ) : 0;
    if ( program == 0 || program->program == 0 || state.layout == 0 || state.vertex_buffer == 0 )
        return 0;
    apply_targets();
    gl.UseProgram( program->program );
    for ( unsigned int slot = 0; slot < 2; slot++ )
        if ( state.constants[ slot ] )
            gl.BindBufferBase( 0x8A11, slot, state.constants[ slot ]->name );
    for ( unsigned int unit = 0; unit < 2; unit++ )
    {
        gl_object *view = state.textures[ unit ];
        gl.ActiveTexture( 0x84C0 + unit );
        gl.BindTexture( 0x0DE1, view && view->texture ? view->texture->name : 0 );
        gl.BindSampler( unit, state.samplers[ 0 ] ? state.samplers[ 0 ]->name : 0 );    // the game uses one sampler for both
    }
    if ( state.blend && state.blend->blend_enable )
    {
        gl.Enable( 0x0BE2 );
        gl.BlendFuncSeparate( state.blend->src, state.blend->dst, state.blend->src_alpha, state.blend->dst_alpha );
        gl.BlendEquationSeparate( state.blend->op, state.blend->op_alpha );
    }
    else
        gl.Disable( 0x0BE2 );
    if ( state.depth && state.depth->depth_enable )
    {
        gl.Enable( 0x0B71 );
        gl.DepthFunc( state.depth->depth_func );
        gl.DepthMask( state.depth->depth_write );
    }
    else
        gl.Disable( 0x0B71 );
    gl.BindVertexArray( state.vao );
    gl.BindBuffer( 0x8892, state.vertex_buffer->name );
    for ( int location = 0; location < 4; location++ )
        gl.DisableVertexAttribArray( location );
    for ( int i = 0; i < state.layout->num_elements; i++ )
    {
        gl.EnableVertexAttribArray( state.layout->elements[ i ].location );
        gl.VertexAttribPointer( state.layout->elements[ i ].location, state.layout->elements[ i ].components, state.layout->elements[ i ].type,
                                state.layout->elements[ i ].normalized, state.stride,
                                (const void *)( state.vertex_offset + state.layout->elements[ i ].offset ) );
    }
    return 1;
}

int gge_gl_Draw( void *context, unsigned int count, unsigned int start )
{
    CALL( count, start, state.stride, state.vertex_buffer ? state.vertex_buffer->size : 0 );
    if ( prepare_draw() )
        gl.DrawArrays( 0x0004, start, count );                              // GL_TRIANGLES
    return 0;
}

int gge_gl_DrawIndexed( void *context, unsigned int count, unsigned int start, int base_vertex )
{
    CALL( count, start, state.stride, state.vertex_buffer ? state.vertex_buffer->size : 0 );
    if ( !prepare_draw() || state.index_buffer == 0 )
        return 0;
    gl.BindBuffer( 0x8893, state.index_buffer->name );
    gl.DrawElementsBaseVertex( 0x0004, count, state.index_type,
                               (const void *)( state.index_offset + start * ( state.index_type == 0x1403 ? 2 : 4 ) ), base_vertex );
    return 0;
}

__attribute__(( weak )) int ILB_native_hide_picture( void ) { return 0; }      // the real one: draft/native/native_platform.c

// ILB_SCREENSHOTS=<folder>: for tests, every 90th picture shown is also written there as shot_<n>.bmp, at
// half size. Nothing else on the screen is in it, only what the game drew.
char *getenv( const char * );
FILE *fopen( const char *, const char * );
unsigned int fwrite( const void *, unsigned int, unsigned int, FILE * );
int fclose( FILE * );
int snprintf( char *, unsigned int, const char *, ... );
void *malloc( unsigned int );
void free( void * );

static int screenshot_wanted;
void gge_gl_screenshot_next( void ) { screenshot_wanted = 1; }     // the next picture as well, whatever its number

static void screenshot( void )
{
    static unsigned int presents, taken;
    const char *folder = getenv( "ILB_SCREENSHOTS" );
    int width = 0, height = 0, wanted = screenshot_wanted;
    screenshot_wanted = 0;
    if ( folder == 0 || ( presents++ % 90 != 0 && !wanted ) || gl.ReadPixels == 0 )
        return;
    SDL_GL_GetDrawableSize( ILB_window, &width, &height );
    if ( width < 2 || height < 2 || width > 8192 || height > 8192 )
        return;
    unsigned char *pixels = (unsigned char *)malloc( (unsigned int)( width * height * 4 ) );
    if ( pixels == 0 ) return;
    gl.BindFramebuffer( 0x8D40, 0 );                // the window's own picture
    gl.PixelStorei( 0x0D05, 1 );                    // GL_PACK_ALIGNMENT
    gl.ReadPixels( 0, 0, width, height, 0x80E1, 0x1401, pixels );     // GL_BGRA, GL_UNSIGNED_BYTE; bottom row first, as BMP has it
    int w = width / 2, h = height / 2, row = ( w * 3 + 3 ) & ~3;
    unsigned char header[ 54 ] = { 'B', 'M' };
    unsigned int file_size = 54 + (unsigned int)( row * h ), *field;
    field = (unsigned int *)( header + 2 );  *field = file_size;
    field = (unsigned int *)( header + 10 ); *field = 54;
    field = (unsigned int *)( header + 14 ); *field = 40;
    field = (unsigned int *)( header + 18 ); *field = (unsigned int)w;
    field = (unsigned int *)( header + 22 ); *field = (unsigned int)h;
    header[ 26 ] = 1; header[ 28 ] = 24;
    char path[ 1024 ];
    snprintf( path, sizeof path, "%s/shot_%04u.bmp", folder, taken++ );
    FILE *file = fopen( path, "wb" );
    if ( file )
    {
        unsigned char *line = (unsigned char *)malloc( (unsigned int)row );
        fwrite( header, 1, 54, file );
        for ( int y = 0; line && y < h; y++ )
        {
            for ( int x = 0; x < row; x++ ) line[ x ] = 0;
            for ( int x = 0; x < w; x++ )
            {
                const unsigned char *p = pixels + ( ( 2 * y ) * width + 2 * x ) * 4;
                line[ 3 * x ] = p[ 0 ]; line[ 3 * x + 1 ] = p[ 1 ]; line[ 3 * x + 2 ] = p[ 2 ];
            }
            fwrite( line, 1, (unsigned int)row, file );
        }
        free( line );
        fclose( file );
    }
    free( pixels );
}

int gge_gl_Present( void *swap_chain, unsigned int sync_interval, unsigned int flags )
{
    CALL( 0, 0, 0, 0 );
    int hide = ILB_native_hide_picture();
    if ( hide == 2 )                                // a host being switched into or out of a session: keep the last picture
    {
        state.dirty_targets = 1;
        return 0;
    }
    if ( hide )                                     // a run on its scripted way into a game: black until loading
    {
        gl.BindFramebuffer( 0x8D40, 0 );
        gl.ColorMask( 1, 1, 1, 1 );
        gl.ClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
        gl.Clear( 0x4000 );
    }
    screenshot();
    SDL_GL_SwapWindow( ILB_window );
    state.dirty_targets = 1;
    return 0;
}
