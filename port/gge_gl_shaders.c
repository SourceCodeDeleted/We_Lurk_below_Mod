// gge_gl_shaders.c - the game's 13 shader pairs as GLSL, for the OpenGL renderer (hand-written).
//
// Rewritten from the compiled Direct3D shaders in ILB.gbp (shaders/dx11/*.vso, *.pso), instruction by
// instruction; `tools/dxbc_dis.py` prints the originals and dump/shaders_dis.txt holds that listing.
// Names of constants and inputs are the originals' (they are stored in the compiled files).
//
// Conventions kept from the originals:
//  - vertex colours arrive as 4 bytes B,G,R,A and every vertex shader swaps them (".zyxw");
//  - every pixel shader writes two outputs: the picture, and what the bloom pass should blur (colours
//    brighter than 0.9 luma; black for everything else and for all UI);
//  - global_data and matrix_buffer are the two constant blocks, with the originals' layouts.
#include "gge_gl.h"

#define HEADER \
    "#version 330 core\n" \
    "layout(std140) uniform matrix_buffer { vec4 view_matrix[4]; vec4 projection_matrix[4]; };\n" \
    "layout(std140) uniform global_data { float brightness; float contrast; float alpha_black; float alpha_level;\n" \
    "                                     float screen_width; float screen_height; float horizontal; float empty2; };\n"

// position * view_matrix * projection_matrix, vectors as rows (what the 20 mul/mad instructions compute)
#define WORLD_POSITION \
    "vec4 world_position( vec3 p ) {\n" \
    "    vec4 v = p.x * view_matrix[0] + p.y * view_matrix[1] + p.z * view_matrix[2] + view_matrix[3];\n" \
    "    return v.x * projection_matrix[0] + v.y * projection_matrix[1] + v.z * projection_matrix[2] + v.w * projection_matrix[3];\n" \
    "}\n"

// pixels on screen -> clip space, y down
#define SCREEN_POSITION \
    "vec4 screen_position( vec4 p ) {\n" \
    "    vec2 xy = p.xy / vec2( screen_width, screen_height ) * 2.0;\n" \
    "    return vec4( xy * vec2( 1.0, -1.0 ) + vec2( -1.0, 1.0 ), p.zw );\n" \
    "}\n"

#define PIXEL_HEADER HEADER \
    "uniform sampler2D shader_texture;\n" \
    "uniform sampler2D shader_texture1;\n" \
    "layout(location = 0) out vec4 target0;\n" \
    "layout(location = 1) out vec4 target1;\n" \
    "vec4 bright_pass( vec4 c ) { return 0.9 >= dot( c.rgb, vec3( 0.2126, 0.7152, 0.0722 ) ) ? vec4( 0.0, 0.0, 0.0, 1.0 ) : c; }\n" \
    "vec3 graded( vec3 c ) { return ( c + brightness ) * contrast; }\n"

// ---------------------------------------------------------------- vertex shaders (6 different ones)
// attribute locations: 0 position, 1 texture coordinate, 2 blend weight, 3 colour
static const char vs_plain[] = HEADER WORLD_POSITION
    "layout(location = 0) in vec3 in_position;\n"
    "layout(location = 3) in vec4 in_color;\n"
    "out vec4 color;\n"
    "void main() { gl_Position = world_position( in_position ); color = in_color.zyxw; }\n";

static const char vs_sprite[] = HEADER WORLD_POSITION          // sprite, overlay
    "layout(location = 0) in vec3 in_position;\n"
    "layout(location = 1) in vec2 in_texcoord;\n"
    "layout(location = 2) in float in_blendweight;\n"
    "layout(location = 3) in vec4 in_color;\n"
    "out vec2 texcoord; out float blendweight; out vec4 color;\n"
    "void main() { gl_Position = world_position( in_position ); texcoord = in_texcoord; blendweight = in_blendweight; color = in_color.zyxw; }\n";

static const char vs_world_textured[] = HEADER WORLD_POSITION  // alpha_sprite, block
    "layout(location = 0) in vec3 in_position;\n"
    "layout(location = 1) in vec2 in_texcoord;\n"
    "layout(location = 3) in vec4 in_color;\n"
    "out vec2 texcoord; out vec4 color;\n"
    "void main() { gl_Position = world_position( in_position ); texcoord = in_texcoord; color = in_color.zyxw; }\n";

static const char vs_screen_textured[] = HEADER SCREEN_POSITION    // ui, title_ui, alpha_sprite_ui, map, parallax, blur, bloom_blend
    "layout(location = 0) in vec4 in_position;\n"
    "layout(location = 1) in vec2 in_texcoord;\n"
    "layout(location = 3) in vec4 in_color;\n"
    "out vec2 texcoord; out vec4 color;\n"
    "void main() { gl_Position = screen_position( in_position ); texcoord = in_texcoord; color = in_color.zyxw; }\n";

static const char vs_screen_plain[] = HEADER SCREEN_POSITION       // alpha_ui
    "layout(location = 0) in vec4 in_position;\n"
    "layout(location = 3) in vec4 in_color;\n"
    "out vec4 color;\n"
    "void main() { gl_Position = screen_position( in_position ); color = in_color.zyxw; }\n";

// ---------------------------------------------------------------- pixel shaders
static const char ps_plain[] = PIXEL_HEADER
    "in vec4 color;\n"
    "void main() { target0 = vec4( graded( color.rgb ), color.a ); target1 = bright_pass( color ); }\n";

static const char ps_alpha_ui[] = PIXEL_HEADER
    "in vec4 color;\n"
    "void main() { target0 = vec4( graded( color.rgb ), color.a ); target1 = vec4( 0.0, 0.0, 0.0, 1.0 ); }\n";

// sprite, overlay: a blend weight of 0 means "flat vertex colour", otherwise texture * colour
static const char ps_sprite[] = PIXEL_HEADER
    "in vec2 texcoord; in float blendweight; in vec4 color;\n"
    "void main() {\n"
    "    vec4 t = texture( shader_texture, texcoord );\n"
    "    if ( t.a == 0.0 ) discard;\n"
    "    vec4 c = blendweight == 0.0 ? color : vec4( t.rgb * color.rgb, t.a );\n"
    "    target1 = bright_pass( c );\n"
    "    target0 = vec4( graded( c.rgb ), c.a );\n"
    "}\n";

static const char ps_alpha_sprite[] = PIXEL_HEADER
    "in vec2 texcoord; in vec4 color;\n"
    "void main() {\n"
    "    vec4 t = texture( shader_texture, texcoord );\n"
    "    if ( t.a == 0.0 ) discard;\n"
    "    vec4 c = t * color;\n"
    "    target0 = vec4( graded( t.rgb * color.rgb ), c.a );\n"
    "    target1 = bright_pass( c );\n"
    "}\n";

static const char ps_block[] = PIXEL_HEADER
    "in vec2 texcoord; in vec4 color;\n"
    "void main() {\n"
    "    vec4 t = texture( shader_texture, texcoord );\n"
    "    if ( t.a == 0.0 ) discard;\n"
    "    vec3 c = t.rgb * color.rgb;\n"
    "    target1 = vec4( 0.9 >= dot( c, vec3( 0.2126, 0.7152, 0.0722 ) ) ? vec3( 0.0 ) : c, 1.0 );\n"
    "    target0 = vec4( graded( c ), 1.0 );\n"
    "}\n";

// ui, parallax: texture * colour, fully transparent texels dropped, nothing for the bloom pass
static const char ps_ui[] = PIXEL_HEADER
    "in vec2 texcoord; in vec4 color;\n"
    "void main() {\n"
    "    vec4 t = texture( shader_texture, texcoord );\n"
    "    if ( t.a == 0.0 ) discard;\n"
    "    target0 = vec4( graded( t.rgb * color.rgb ), t.a );\n"
    "    target1 = vec4( 0.0, 0.0, 0.0, 1.0 );\n"
    "}\n";

// alpha_sprite_ui, title_ui: as ui, with the vertex alpha applied and no discard
static const char ps_alpha_sprite_ui[] = PIXEL_HEADER
    "in vec2 texcoord; in vec4 color;\n"
    "void main() {\n"
    "    vec4 t = texture( shader_texture, texcoord );\n"
    "    target0 = vec4( graded( t.rgb * color.rgb ), t.a * color.a );\n"
    "    target1 = vec4( 0.0, 0.0, 0.0, 1.0 );\n"
    "}\n";

// map: unexplored (transparent) texels are drawn with alpha_black; the whole map uses alpha_level
static const char ps_map[] = PIXEL_HEADER
    "in vec2 texcoord; in vec4 color;\n"
    "void main() {\n"
    "    vec4 t = texture( shader_texture, texcoord );\n"
    "    float a = t.a == 0.0 ? alpha_black : t.a;\n"
    "    if ( a == 0.0 ) discard;\n"
    "    target0 = vec4( graded( t.rgb * color.rgb ), alpha_level );\n"
    "    target1 = vec4( 0.0, 0.0, 0.0, 1.0 );\n"
    "}\n";

static const char ps_bloom_blend[] = PIXEL_HEADER
    "in vec2 texcoord; in vec4 color;\n"
    "void main() { target0 = vec4( texture( shader_texture, texcoord ).rgb + texture( shader_texture1, texcoord ).rgb, 1.0 ); target1 = vec4( 0.0 ); }\n";

// blur: 9-tap Gaussian along x (horizontal != 0) or y, one texel apart
static const char ps_blur[] = PIXEL_HEADER
    "in vec2 texcoord; in vec4 color;\n"
    "const float weight[5] = float[5]( 0.227027, 0.194595, 0.121622, 0.054054, 0.016216 );\n"
    "void main() {\n"
    "    ivec2 size = textureSize( shader_texture, 0 );\n"
    "    vec2 step = horizontal != 0.0 ? vec2( 1.0 / float( size.x ), 0.0 ) : vec2( 0.0, 1.0 / float( size.y ) );\n"
    "    vec3 sum = texture( shader_texture, texcoord ).rgb * weight[0];\n"
    "    for ( int i = 1; i < 5; i++ )\n"
    "        sum += ( texture( shader_texture, texcoord + step * float( i ) ).rgb + texture( shader_texture, texcoord - step * float( i ) ).rgb ) * weight[i];\n"
    "    target0 = vec4( sum, 1.0 );\n"
    "    target1 = vec4( 0.0 );\n"
    "}\n";

// by the name the game asks for (the material type's shader file name, without extension)
const gge_gl_shader_source gge_gl_shader_sources[] =
{
    { "plain",              vs_plain,           ps_plain },
    { "sprite",             vs_sprite,          ps_sprite },
    { "overlay",            vs_sprite,          ps_sprite },
    { "alpha_sprite",       vs_world_textured,  ps_alpha_sprite },
    { "block",              vs_world_textured,  ps_block },
    { "ui",                 vs_screen_textured, ps_ui },
    { "parallax",           vs_screen_textured, ps_ui },
    { "title_ui",           vs_screen_textured, ps_alpha_sprite_ui },
    { "alpha_sprite_ui",    vs_screen_textured, ps_alpha_sprite_ui },
    { "map",                vs_screen_textured, ps_map },
    { "bloom_blend",        vs_screen_textured, ps_bloom_blend },
    { "blur",               vs_screen_textured, ps_blur },
    { "alpha_ui",           vs_screen_plain,    ps_alpha_ui },
    { 0, 0, 0 }
};
