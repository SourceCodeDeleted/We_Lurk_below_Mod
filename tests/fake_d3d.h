// In-memory stand-in for the Direct3D 11 device used by the native graphics tests. Include after
// fake_platform.h, from one translation unit per test.
#pragma once
#include "gge_main.h"
#include "gge_view.h"
#include <vector>

static int live_objects = 0, d3d_errors = 0;

template <class T> struct counted : T
{
    HRESULT QueryInterface( void *, void ** ) { return 0; }
    unsigned long AddRef() { return 1; }
    unsigned long Release() { live_objects--; delete this; return 0; }
    virtual ~counted() {}
};
struct fake_texture : counted<ID3D11Texture2D>
{
    int width, height, pitch;       // pitch in pixels, wider than the texture on purpose
    std::vector<dword> pixels;
};
struct fake_buffer : counted<ID3D11Buffer> { D3D11_BUFFER_DESC desc; std::vector<unsigned char> bytes; int discards = 0; };
struct fake_blend : counted<ID3D11BlendState> { D3D11_BLEND_DESC desc; };
struct fake_sampler : counted<ID3D11SamplerState> { D3D11_SAMPLER_DESC desc; };
struct fake_shader_v : counted<ID3D11VertexShader> { size_t size; };
struct fake_shader_p : counted<ID3D11PixelShader> { size_t size; };
struct fake_layout : counted<ID3D11InputLayout> { UINT num_elements; UINT last_offset; };
struct fake_depth_state : counted<ID3D11DepthStencilState> { D3D11_DEPTH_STENCIL_DESC desc; };
struct fake_raster : counted<ID3D11RasterizerState> { D3D11_RASTERIZER_DESC desc; };
struct fake_plain_texture : counted<ID3D11Texture2D> { D3D11_TEXTURE2D_DESC desc; };

struct fake_device : ID3D11Device
{
    std::vector<fake_texture *> textures;
    std::vector<fake_buffer *> buffers;
    std::vector<fake_blend *> blends;
    std::vector<fake_sampler *> samplers;
    std::vector<size_t> vertex_shader_sizes, pixel_shader_sizes;
    std::vector<fake_layout *> layouts;
    std::vector<fake_plain_texture *> depth_textures;
    std::vector<fake_depth_state *> depth_states;
    std::vector<fake_raster *> rasters;
    int target_views = 0, depth_views = 0;

    HRESULT QueryInterface( void *, void ** ) { return 0; }
    unsigned long AddRef() { return 1; }
    unsigned long Release() { return 0; }
    HRESULT CreateTexture2D( const D3D11_TEXTURE2D_DESC * desc, const D3D11_SUBRESOURCE_DATA *, ID3D11Texture2D ** out )
    {
        if ( desc->BindFlags == D3D11_BIND_DEPTH_STENCIL )
        {
            fake_plain_texture * d = new fake_plain_texture; d->desc = *desc;
            live_objects++; depth_textures.push_back( d ); *out = d;
            return 0;
        }
        if ( desc->Format != DXGI_FORMAT_B8G8R8A8_UNORM || desc->Usage != D3D11_USAGE_DYNAMIC ) return -1;
        fake_texture * t = new fake_texture;
        t->width = desc->Width; t->height = desc->Height; t->pitch = desc->Width + 7;
        t->pixels.assign( (size_t)t->pitch * t->height, 0xDEADBEEF );
        live_objects++; textures.push_back( t ); *out = t;
        return 0;
    }
    HRESULT CreateShaderResourceView( ID3D11Resource *, const D3D11_SHADER_RESOURCE_VIEW_DESC *, ID3D11ShaderResourceView ** out )
    {
        live_objects++; *out = new counted<ID3D11ShaderResourceView>;
        return 0;
    }
    HRESULT CreateBuffer( const D3D11_BUFFER_DESC * desc, const D3D11_SUBRESOURCE_DATA *, ID3D11Buffer ** out )
    {
        fake_buffer * b = new fake_buffer; b->desc = *desc; b->bytes.assign( desc->ByteWidth, 0xEE );
        live_objects++; buffers.push_back( b ); *out = b;
        return 0;
    }
    HRESULT CreateBlendState( const D3D11_BLEND_DESC * desc, ID3D11BlendState ** out )
    {
        fake_blend * b = new fake_blend; b->desc = *desc;
        live_objects++; blends.push_back( b ); *out = b;
        return 0;
    }
    HRESULT CreateSamplerState( const D3D11_SAMPLER_DESC * desc, ID3D11SamplerState ** out )
    {
        fake_sampler * s = new fake_sampler; s->desc = *desc;
        live_objects++; samplers.push_back( s ); *out = s;
        return 0;
    }
    HRESULT CreateVertexShader( const void * data, size_t size, ID3D11ClassLinkage *, ID3D11VertexShader ** out )
    {
        if ( data == NULL || size == 0 ) return -1;
        fake_shader_v * s = new fake_shader_v; s->size = size;
        live_objects++; vertex_shader_sizes.push_back( size ); *out = s;
        return 0;
    }
    HRESULT CreatePixelShader( const void * data, size_t size, ID3D11ClassLinkage *, ID3D11PixelShader ** out )
    {
        if ( data == NULL || size == 0 ) return -1;
        fake_shader_p * s = new fake_shader_p; s->size = size;
        live_objects++; pixel_shader_sizes.push_back( size ); *out = s;
        return 0;
    }
    HRESULT CreateInputLayout_impl( const D3D11_INPUT_ELEMENT_DESC * desc, UINT count, const void * data, size_t size, ID3D11InputLayout ** out )
    {
        if ( data == NULL || size == 0 || count == 0 ) return -1;
        fake_layout * l = new fake_layout; l->num_elements = count; l->last_offset = desc[ count - 1 ].AlignedByteOffset;
        live_objects++; layouts.push_back( l ); *out = l;
        return 0;
    }
    HRESULT CreateInputLayout( const D3D11_INPUT_ELEMENT_DESC * desc, UINT count, const void * data, size_t size, ID3D11InputLayout ** out ) { return CreateInputLayout_impl( desc, count, data, size, out ); }
    HRESULT CreateRenderTargetView( ID3D11Resource *, const D3D11_RENDER_TARGET_VIEW_DESC *, ID3D11RenderTargetView ** out ) { live_objects++; target_views++; *out = new counted<ID3D11RenderTargetView>; return 0; }
    HRESULT CreateDepthStencilView( ID3D11Resource *, const D3D11_DEPTH_STENCIL_VIEW_DESC *, ID3D11DepthStencilView ** out ) { live_objects++; depth_views++; *out = new counted<ID3D11DepthStencilView>; return 0; }
    HRESULT CreateDepthStencilState( const D3D11_DEPTH_STENCIL_DESC * desc, ID3D11DepthStencilState ** out ) { fake_depth_state * d = new fake_depth_state; d->desc = *desc; live_objects++; depth_states.push_back( d ); *out = d; return 0; }
    HRESULT CreateRasterizerState( const D3D11_RASTERIZER_DESC * desc, ID3D11RasterizerState ** out ) { fake_raster * r = new fake_raster; r->desc = *desc; live_objects++; rasters.push_back( r ); *out = r; return 0; }
};
struct fake_draw { ID3D11Buffer * vertexbuffer; UINT stride; ID3D11Buffer * indexbuffer; UINT count; bool indexed; };
struct fake_context : ID3D11DeviceContext
{
    ID3D11Buffer * bound_vertexbuffer = NULL, * bound_indexbuffer = NULL;
    UINT bound_stride = 0;
    std::vector<fake_draw> draws;

    HRESULT QueryInterface( void *, void ** ) { return 0; }
    unsigned long AddRef() { return 1; }
    unsigned long Release() { return 0; }
    HRESULT Map( ID3D11Resource * r, UINT, D3D11_MAP type, UINT, D3D11_MAPPED_SUBRESOURCE * mapped )
    {
        if ( fake_texture * t = dynamic_cast<fake_texture *>( r ) )
        {
            mapped->pData = t->pixels.data();
            mapped->RowPitch = t->pitch * 4;
        }
        else if ( fake_buffer * b = dynamic_cast<fake_buffer *>( r ) )
        {
            if ( type == D3D11_MAP_WRITE_DISCARD ) { b->discards++; b->bytes.assign( b->bytes.size(), 0xEE ); }
            mapped->pData = b->bytes.data();
            mapped->RowPitch = (UINT)b->bytes.size();
        }
        return 0;
    }
    void Unmap( ID3D11Resource *, UINT ) {}
    void IASetVertexBuffers( UINT, UINT, ID3D11Buffer * const * buffers, const UINT * strides, const UINT * ) { bound_vertexbuffer = buffers[ 0 ]; bound_stride = strides[ 0 ]; }
    void IASetIndexBuffer( ID3D11Buffer * buffer, DXGI_FORMAT, UINT ) { bound_indexbuffer = buffer; }
    void Draw( UINT count, UINT ) { draws.push_back( { bound_vertexbuffer, bound_stride, NULL, count, false } ); }
    void DrawIndexed( UINT count, UINT, int ) { draws.push_back( { bound_vertexbuffer, bound_stride, bound_indexbuffer, count, true } ); }

    ID3D11InputLayout * layout = NULL;
    ID3D11VertexShader * vertex_shader = NULL;
    ID3D11PixelShader * pixel_shader = NULL;
    ID3D11Buffer * vs_constants[ 2 ] = { NULL, NULL }, * ps_constants[ 2 ] = { NULL, NULL };
    ID3D11ShaderResourceView * resource_view = NULL;
    ID3D11SamplerState * sampler = NULL;
    ID3D11BlendState * blend = NULL;
    UINT blend_mask = 0;
    void IASetInputLayout( ID3D11InputLayout * l ) { layout = l; }
    void VSSetShader( ID3D11VertexShader * s, ID3D11ClassInstance * const *, UINT ) { vertex_shader = s; }
    void PSSetShader( ID3D11PixelShader * s, ID3D11ClassInstance * const *, UINT ) { pixel_shader = s; }
    void VSSetConstantBuffers( UINT slot, UINT, ID3D11Buffer * const * b ) { vs_constants[ slot ] = b[ 0 ]; }
    void PSSetConstantBuffers( UINT slot, UINT, ID3D11Buffer * const * b ) { ps_constants[ slot ] = b[ 0 ]; }
    void PSSetShaderResources( UINT, UINT, ID3D11ShaderResourceView * const * v ) { resource_view = v[ 0 ]; }
    void PSSetSamplers( UINT, UINT, ID3D11SamplerState * const * v ) { sampler = v[ 0 ]; }
    void OMSetBlendState( ID3D11BlendState * b, const float *, UINT mask ) { blend = b; blend_mask = mask; }
    ID3D11RenderTargetView * target = NULL;
    ID3D11DepthStencilView * depth = NULL;
    ID3D11DepthStencilState * depth_state = NULL;
    ID3D11RasterizerState * raster = NULL;
    D3D11_PRIMITIVE_TOPOLOGY topology = (D3D11_PRIMITIVE_TOPOLOGY)0;
    D3D11_VIEWPORT viewport = {};
    float clear_color[ 4 ] = { -1, -1, -1, -1 };
    int depth_clears = 0;
    void OMSetRenderTargets( UINT, ID3D11RenderTargetView * const * views_, ID3D11DepthStencilView * d ) { target = views_[ 0 ]; depth = d; }
    void OMSetDepthStencilState( ID3D11DepthStencilState * s, UINT ) { depth_state = s; }
    void IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY t ) { topology = t; }
    void ClearRenderTargetView( ID3D11RenderTargetView *, const float * c ) { memcpy( clear_color, c, sizeof( clear_color ) ); }
    void ClearDepthStencilView( ID3D11DepthStencilView *, UINT, float, unsigned char ) { depth_clears++; }
    void RSSetState( ID3D11RasterizerState * r ) { raster = r; }
    void RSSetViewports( UINT, const D3D11_VIEWPORT * v ) { viewport = v[ 0 ]; }
    void UpdateSubresource( ID3D11Resource * r, UINT, const D3D11_BOX *, const void * data, UINT, UINT )
    {
        fake_buffer * b = dynamic_cast<fake_buffer *>( r );
        if ( b ) memcpy( b->bytes.data(), data, b->bytes.size() );
    }
};
static fake_device the_device;
static fake_context the_context;

// The swap chain and device creation. The device and context are the two static objects above;
// they count as live objects while "created" so a test can check gge_shutdown releases them.
struct fake_swap_chain : counted<IDXGISwapChain>
{
    DXGI_SWAP_CHAIN_DESC desc;
    std::vector<UINT> presents;
    HRESULT GetBuffer( UINT, const GUID &, void ** out ) { live_objects++; *out = new counted<ID3D11Texture2D>; return 0; }
    HRESULT Present( UINT sync, UINT ) { presents.push_back( sync ); return 0; }
};
static fake_swap_chain * the_swap_chain = NULL;
const GUID IID_ID3D11Texture2D = {};
HWND GetActiveWindow() { return (HWND)0x1234; }

#ifdef FAKE_D3D_REAL_DEVICE_GLOBALS
// the test links src/gge_main.cpp, which owns d3d_device and creates it through this
HRESULT D3D11CreateDeviceAndSwapChain( IDXGIAdapter *, D3D_DRIVER_TYPE type, void *, UINT, const D3D_FEATURE_LEVEL *, UINT, UINT sdk,
    const DXGI_SWAP_CHAIN_DESC * desc, IDXGISwapChain ** swap, ID3D11Device ** device, D3D_FEATURE_LEVEL *, ID3D11DeviceContext ** context )
{
    if ( type != D3D_DRIVER_TYPE_HARDWARE || sdk != D3D11_SDK_VERSION ) return -1;
    the_swap_chain = new fake_swap_chain; the_swap_chain->desc = *desc;
    live_objects++;
    *swap = the_swap_chain; *device = &the_device; *context = &the_context;
    return 0;
}
#else
ID3D11Device * d3d_device = &the_device;
ID3D11DeviceContext * d3d_device_context = &the_context;
#endif
bool d3d_error( HRESULT result, const char *, const char *, int ) { if ( result != 0 ) d3d_errors++; return false; }

// view state the material code reads
view_data views[ max_clients ];
matrix screen_matrix;
