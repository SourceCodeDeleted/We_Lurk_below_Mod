//-----------------------------------------------------------------------------
// gge_main.cpp  (device half)
// Reconstructed from ILB.exe: create_device_resources 0x40C260 (create_device_and_swapchain inlined),
// gge_shutdown 0x40C710, set_render_target_and_clear 0x40C810, gge_menu_render 0x40C930.
//
// Original names: create_device_and_swapchain, create_device_resources, gge_menu_render,
// gge_get_device, gge_get_device_context, d3d_device, d3d_device_context. Chosen here:
// gge_shutdown, set_render_target_and_clear, gge_module_tbl and the names of the other Direct3D
// objects.
//
// Not reconstructed yet (same file): create_render_to_texture_resources 0x40BBE0 and its free
// 0x40C5D0, the in-game frame (0x40CA60, gge_game_render_client_to_texture 0x40CD40,
// gge_game_render_present 0x40D670), display mode enumeration 0x40D960 and the video memory text
// 0x40DBF0.

#include "main.h"
#include "ec_config.h"
#include "gge_main.h"
#include "gge_gs3.h"
#include "gge_textures.h"
#include "gge_materials.h"
#include "gge_models.h"
#include "gge_view.h"
#include "pending.h"
#include <SDL.h>

ID3D11Device * d3d_device = NULL;                               // 0x383BE98
ID3D11DeviceContext * d3d_device_context = NULL;                // 0x383BE9C

static ID3D11RenderTargetView * back_buffer_view = NULL;        // 0x3735D74
static ID3D11Texture2D * depth_stencil_buffer = NULL;           // 0x383BE8C
static ID3D11DepthStencilState * depth_stencil_state = NULL;    // 0x383BE90
static ID3D11RasterizerState * rasterizer_state = NULL;         // 0x383BE94
static IDXGISwapChain * swap_chain = NULL;
static ID3D11DepthStencilView * depth_stencil_view = NULL;      // 0x383BEA8

//-----------------------------------------------------------------------------
// The graphics modules, started in this order by the client start-up and freed in reverse by
// gge_shutdown (table at 0x5D42A0).

gge_module gge_module_tbl[ num_gge_modules ] =
{
    { gge_gs3_init,         gge_gs3_free },
    { gge_textures_init,    gge_textures_free_all },
    { gge_materials_init,   gge_materials_free },
    { gge_models_init,      gge_models_free },
    { gge_view_set_ports,   NULL },
};

//-----------------------------------------------------------------------------

static bool create_device_and_swapchain()
{
    DXGI_MODE_DESC mode;
    SDL_memset( &mode, 0, sizeof( mode ) );
    mode.Width = window_width;
    mode.Height = window_height;
    mode.RefreshRate.Numerator = 60;
    mode.RefreshRate.Denominator = 1;
    mode.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    mode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    mode.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    DXGI_SWAP_CHAIN_DESC desc;
    SDL_memset( &desc, 0, sizeof( desc ) );
    desc.BufferDesc = mode;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 1;
    desc.OutputWindow = GetActiveWindow();
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    HRESULT hr = D3D11CreateDeviceAndSwapChain( NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
        &desc, &swap_chain, &d3d_device, NULL, &d3d_device_context );
    assert_return_false( hr == S_OK );

    return true;
}

//-----------------------------------------------------------------------------
// device, back buffer view, a 16-bit depth buffer the size of one view, and the fixed depth and
// rasterizer states

bool create_device_resources()
{
    int view_width = ( game_globals.unknown_2044 == 2 ) ? window_width / 2 : window_width;

    bool device_ok = create_device_and_swapchain();
    assert_return_false( device_ok );

    ID3D11Texture2D * back_buffer = NULL;
    HRESULT result = swap_chain->GetBuffer( 0, IID_ID3D11Texture2D, (void **)&back_buffer );
    if ( result != S_OK ) { d3d_check( result ); return false; }

    result = d3d_device->CreateRenderTargetView( back_buffer, NULL, &back_buffer_view );
    if ( result != S_OK ) { d3d_check( result ); return false; }
    if ( back_buffer ) back_buffer->Release();

    D3D11_TEXTURE2D_DESC depth_desc;
    depth_desc.Width = view_width;
    depth_desc.Height = window_height;
    depth_desc.MipLevels = 1;
    depth_desc.ArraySize = 1;
    depth_desc.Format = DXGI_FORMAT_D16_UNORM;
    depth_desc.SampleDesc.Count = 1;
    depth_desc.SampleDesc.Quality = 0;
    depth_desc.Usage = D3D11_USAGE_DEFAULT;
    depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depth_desc.CPUAccessFlags = 0;
    depth_desc.MiscFlags = 0;
    result = d3d_device->CreateTexture2D( &depth_desc, NULL, &depth_stencil_buffer );
    if ( result != S_OK ) { d3d_check( result ); return false; }

    result = d3d_device->CreateDepthStencilView( depth_stencil_buffer, NULL, &depth_stencil_view );
    if ( result != S_OK ) { d3d_check( result ); return false; }

    D3D11_DEPTH_STENCIL_DESC state_desc;
    memset( &state_desc, 0, sizeof( state_desc ) );
    state_desc.DepthEnable = TRUE;
    state_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    state_desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    state_desc.StencilEnable = FALSE;
    result = d3d_device->CreateDepthStencilState( &state_desc, &depth_stencil_state );
    if ( result != S_OK ) { d3d_check( result ); return false; }

    D3D11_RASTERIZER_DESC raster_desc;
    memset( &raster_desc, 0, sizeof( raster_desc ) );
    raster_desc.FillMode = D3D11_FILL_SOLID;
    raster_desc.CullMode = D3D11_CULL_NONE;
    raster_desc.DepthClipEnable = TRUE;
    result = d3d_device->CreateRasterizerState( &raster_desc, &rasterizer_state );
    if ( result != S_OK ) { d3d_check( result ); return false; }

    return true;
}

//-----------------------------------------------------------------------------

void gge_shutdown()
{
    sub_40C5D0();       // frees the render-to-texture resources

    for ( int i = num_gge_modules - 1; i >= 0; i-- )
    {
        if ( gge_module_tbl[ i ].free ) gge_module_tbl[ i ].free();
    }

    if ( depth_stencil_view ) { depth_stencil_view->Release(); depth_stencil_view = NULL; }
    if ( back_buffer_view ) { back_buffer_view->Release(); back_buffer_view = NULL; }
    if ( depth_stencil_buffer ) { depth_stencil_buffer->Release(); depth_stencil_buffer = NULL; }
    if ( depth_stencil_state ) { depth_stencil_state->Release(); depth_stencil_state = NULL; }
    if ( rasterizer_state ) { rasterizer_state->Release(); rasterizer_state = NULL; }
    if ( swap_chain ) { swap_chain->Release(); swap_chain = NULL; }
    if ( d3d_device_context ) { d3d_device_context->Release(); d3d_device_context = NULL; }
    if ( d3d_device ) { d3d_device->Release(); d3d_device = NULL; }
}

//-----------------------------------------------------------------------------
// targets the back buffer and clears it; color is 0x00RRGGBB

static void set_render_target_and_clear( dword color )
{
    d3d_device_context->OMSetRenderTargets( 1, &back_buffer_view, depth_stencil_view );
    d3d_device_context->OMSetDepthStencilState( depth_stencil_state, 1 );
    d3d_device_context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    float clear_color[ 4 ];
    clear_color[ 0 ] = (float)( ( color >> 16 ) & 0xff ) / 255.0f;
    clear_color[ 1 ] = (float)( ( color >> 8 ) & 0xff ) / 255.0f;
    clear_color[ 2 ] = (float)( color & 0xff ) / 255.0f;
    clear_color[ 3 ] = 1.0f;
    d3d_device_context->ClearRenderTargetView( back_buffer_view, clear_color );
    d3d_device_context->ClearDepthStencilView( depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );
}

//-----------------------------------------------------------------------------
// one frame outside the game: clear, draw every layer in screen space, present

void gge_menu_render( dword clear_color )
{
    assert_return( d3d_device );
    assert_return( d3d_device_context );

    set_render_target_and_clear( clear_color );
    d3d_device_context->RSSetState( rasterizer_state );

    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = (float)window_width;
    viewport.Height = (float)window_height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    d3d_device_context->RSSetViewports( 1, &viewport );

    gge_render_layers( 0, 0, 0 );

    swap_chain->Present( config.vsync ? 1 : 0, 0 );
}
