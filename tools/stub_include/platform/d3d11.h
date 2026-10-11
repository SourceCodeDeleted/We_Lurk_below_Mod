#pragma once
// minimal Direct3D 11 stand-in (see README.txt): only what src/ uses
#include <windows.h>
typedef long HRESULT;
typedef unsigned int UINT;
enum DXGI_FORMAT { DXGI_FORMAT_UNKNOWN = 0, DXGI_FORMAT_D16_UNORM = 55, DXGI_FORMAT_R32G32B32A32_FLOAT = 2, DXGI_FORMAT_R32G32B32_FLOAT = 6, DXGI_FORMAT_R32G32_FLOAT = 16, DXGI_FORMAT_R8G8B8A8_UNORM = 28,
    DXGI_FORMAT_R32_FLOAT = 41, DXGI_FORMAT_R16_UINT = 57, DXGI_FORMAT_B8G8R8A8_UNORM = 87 };
enum D3D11_INPUT_CLASSIFICATION { D3D11_INPUT_PER_VERTEX_DATA = 0 };
struct D3D11_INPUT_ELEMENT_DESC { const char * SemanticName; UINT SemanticIndex; DXGI_FORMAT Format; UINT InputSlot, AlignedByteOffset; D3D11_INPUT_CLASSIFICATION InputSlotClass; UINT InstanceDataStepRate; };
struct D3D11_BOX;
struct ID3D11ClassInstance;
enum D3D11_USAGE { D3D11_USAGE_DEFAULT = 0, D3D11_USAGE_DYNAMIC = 2 };
enum D3D11_MAP { D3D11_MAP_WRITE_DISCARD = 4, D3D11_MAP_WRITE_NO_OVERWRITE = 5 };
#define D3D11_BIND_VERTEX_BUFFER    0x1
#define D3D11_BIND_INDEX_BUFFER     0x2
#define D3D11_BIND_SHADER_RESOURCE  0x8
#define D3D11_CPU_ACCESS_WRITE      0x10000
struct DXGI_SAMPLE_DESC { UINT Count; UINT Quality; };
struct D3D11_TEXTURE2D_DESC
{
    UINT Width, Height, MipLevels, ArraySize;
    DXGI_FORMAT Format;
    DXGI_SAMPLE_DESC SampleDesc;
    D3D11_USAGE Usage;
    UINT BindFlags, CPUAccessFlags, MiscFlags;
};
struct D3D11_MAPPED_SUBRESOURCE { void * pData; UINT RowPitch; UINT DepthPitch; };
struct D3D11_SUBRESOURCE_DATA;
struct D3D11_SHADER_RESOURCE_VIEW_DESC;
#define TRUE 1
#define FALSE 0
#define S_OK 0
#define D3D11_SDK_VERSION 7
#define D3D11_BIND_DEPTH_STENCIL        0x40
#define DXGI_USAGE_RENDER_TARGET_OUTPUT 0x20
#define D3D11_CLEAR_DEPTH               0x1
#define D3D11_CLEAR_STENCIL             0x2
struct GUID { unsigned long a; unsigned short b, c; unsigned char d[ 8 ]; };
extern const GUID IID_ID3D11Texture2D;
HWND __stdcall GetActiveWindow();
enum D3D_DRIVER_TYPE { D3D_DRIVER_TYPE_HARDWARE = 1 };
enum D3D_FEATURE_LEVEL { D3D_FEATURE_LEVEL_11_0 = 0xb000 };
enum D3D11_PRIMITIVE_TOPOLOGY { D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST = 4 };
enum DXGI_MODE_SCANLINE_ORDER { DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED = 0 };
enum DXGI_MODE_SCALING { DXGI_MODE_SCALING_UNSPECIFIED = 0 };
enum DXGI_SWAP_EFFECT { DXGI_SWAP_EFFECT_DISCARD = 0 };
enum D3D11_DEPTH_WRITE_MASK { D3D11_DEPTH_WRITE_MASK_ZERO = 0, D3D11_DEPTH_WRITE_MASK_ALL = 1 };
enum D3D11_FILL_MODE { D3D11_FILL_SOLID = 3 };
enum D3D11_CULL_MODE { D3D11_CULL_NONE = 1 };
struct DXGI_RATIONAL { UINT Numerator, Denominator; };
struct DXGI_MODE_DESC { UINT Width, Height; DXGI_RATIONAL RefreshRate; DXGI_FORMAT Format; DXGI_MODE_SCANLINE_ORDER ScanlineOrdering; DXGI_MODE_SCALING Scaling; };
struct DXGI_SWAP_CHAIN_DESC { DXGI_MODE_DESC BufferDesc; DXGI_SAMPLE_DESC SampleDesc; UINT BufferUsage, BufferCount; HWND OutputWindow; BOOL Windowed; DXGI_SWAP_EFFECT SwapEffect; UINT Flags; };
struct D3D11_VIEWPORT { float TopLeftX, TopLeftY, Width, Height, MinDepth, MaxDepth; };
struct D3D11_RENDER_TARGET_VIEW_DESC;
struct D3D11_DEPTH_STENCIL_VIEW_DESC;
struct IDXGIAdapter;
#define D3D11_BIND_CONSTANT_BUFFER      0x4
#define D3D11_COLOR_WRITE_ENABLE_ALL    15
#define D3D11_FLOAT32_MAX               3.402823466e+38f
enum D3D11_BLEND { D3D11_BLEND_ZERO = 1, D3D11_BLEND_ONE = 2, D3D11_BLEND_SRC_ALPHA = 5, D3D11_BLEND_INV_SRC_ALPHA = 6 };
enum D3D11_BLEND_OP { D3D11_BLEND_OP_ADD = 1 };
enum D3D11_FILTER { D3D11_FILTER_MIN_MAG_MIP_POINT = 0, D3D11_FILTER_MIN_MAG_MIP_LINEAR = 0x15 };
enum D3D11_TEXTURE_ADDRESS_MODE { D3D11_TEXTURE_ADDRESS_WRAP = 1, D3D11_TEXTURE_ADDRESS_CLAMP = 3 };
enum D3D11_COMPARISON_FUNC { D3D11_COMPARISON_NEVER = 1, D3D11_COMPARISON_LESS_EQUAL = 4 };
struct D3D11_DEPTH_STENCILOP_DESC { int StencilFailOp, StencilDepthFailOp, StencilPassOp, StencilFunc; };
struct D3D11_DEPTH_STENCIL_DESC { BOOL DepthEnable; D3D11_DEPTH_WRITE_MASK DepthWriteMask; D3D11_COMPARISON_FUNC DepthFunc; BOOL StencilEnable; unsigned char StencilReadMask, StencilWriteMask; D3D11_DEPTH_STENCILOP_DESC FrontFace, BackFace; };
struct D3D11_RASTERIZER_DESC { D3D11_FILL_MODE FillMode; D3D11_CULL_MODE CullMode; BOOL FrontCounterClockwise; int DepthBias; float DepthBiasClamp, SlopeScaledDepthBias; BOOL DepthClipEnable, ScissorEnable, MultisampleEnable, AntialiasedLineEnable; };
struct D3D11_BUFFER_DESC { UINT ByteWidth; D3D11_USAGE Usage; UINT BindFlags, CPUAccessFlags, MiscFlags, StructureByteStride; };
struct D3D11_RENDER_TARGET_BLEND_DESC
{
    BOOL BlendEnable;
    D3D11_BLEND SrcBlend, DestBlend; D3D11_BLEND_OP BlendOp;
    D3D11_BLEND SrcBlendAlpha, DestBlendAlpha; D3D11_BLEND_OP BlendOpAlpha;
    unsigned char RenderTargetWriteMask;
};
struct D3D11_BLEND_DESC { BOOL AlphaToCoverageEnable, IndependentBlendEnable; D3D11_RENDER_TARGET_BLEND_DESC RenderTarget[ 8 ]; };
struct D3D11_SAMPLER_DESC
{
    D3D11_FILTER Filter;
    D3D11_TEXTURE_ADDRESS_MODE AddressU, AddressV, AddressW;
    float MipLODBias; UINT MaxAnisotropy; D3D11_COMPARISON_FUNC ComparisonFunc;
    float BorderColor[ 4 ]; float MinLOD, MaxLOD;
};
struct ID3D11ClassLinkage;
struct IUnknown { virtual HRESULT QueryInterface( void *, void ** ) = 0; virtual unsigned long AddRef() = 0; virtual unsigned long Release() = 0; };
struct ID3D11Resource : IUnknown {};
struct ID3D11Texture2D : ID3D11Resource {};
struct ID3D11ShaderResourceView : IUnknown {};
struct ID3D11Buffer : ID3D11Resource {};
struct ID3D11BlendState : IUnknown {};
struct ID3D11SamplerState : IUnknown {};
struct ID3D11VertexShader : IUnknown {};
struct ID3D11PixelShader : IUnknown {};
struct ID3D11InputLayout : IUnknown {};
struct ID3D11RenderTargetView : IUnknown {};
struct ID3D11DepthStencilView : IUnknown {};
struct ID3D11DepthStencilState : IUnknown {};
struct ID3D11RasterizerState : IUnknown {};
struct IDXGISwapChain : IUnknown
{
    virtual HRESULT GetBuffer( UINT, const GUID &, void ** ) = 0;
    virtual HRESULT Present( UINT, UINT ) = 0;
};
struct ID3D11Device : IUnknown
{
    virtual HRESULT CreateTexture2D( const D3D11_TEXTURE2D_DESC *, const D3D11_SUBRESOURCE_DATA *, ID3D11Texture2D ** ) = 0;
    virtual HRESULT CreateShaderResourceView( ID3D11Resource *, const D3D11_SHADER_RESOURCE_VIEW_DESC *, ID3D11ShaderResourceView ** ) = 0;
    virtual HRESULT CreateBuffer( const D3D11_BUFFER_DESC *, const D3D11_SUBRESOURCE_DATA *, ID3D11Buffer ** ) = 0;
    virtual HRESULT CreateBlendState( const D3D11_BLEND_DESC *, ID3D11BlendState ** ) = 0;
    virtual HRESULT CreateSamplerState( const D3D11_SAMPLER_DESC *, ID3D11SamplerState ** ) = 0;
    virtual HRESULT CreateVertexShader( const void *, size_t, ID3D11ClassLinkage *, ID3D11VertexShader ** ) = 0;
    virtual HRESULT CreatePixelShader( const void *, size_t, ID3D11ClassLinkage *, ID3D11PixelShader ** ) = 0;
    virtual HRESULT CreateInputLayout( const D3D11_INPUT_ELEMENT_DESC *, UINT, const void *, size_t, ID3D11InputLayout ** ) = 0;
    virtual HRESULT CreateRenderTargetView( ID3D11Resource *, const D3D11_RENDER_TARGET_VIEW_DESC *, ID3D11RenderTargetView ** ) = 0;
    virtual HRESULT CreateDepthStencilView( ID3D11Resource *, const D3D11_DEPTH_STENCIL_VIEW_DESC *, ID3D11DepthStencilView ** ) = 0;
    virtual HRESULT CreateDepthStencilState( const D3D11_DEPTH_STENCIL_DESC *, ID3D11DepthStencilState ** ) = 0;
    virtual HRESULT CreateRasterizerState( const D3D11_RASTERIZER_DESC *, ID3D11RasterizerState ** ) = 0;
};
struct ID3D11DeviceContext : IUnknown
{
    virtual HRESULT Map( ID3D11Resource *, UINT, D3D11_MAP, UINT, D3D11_MAPPED_SUBRESOURCE * ) = 0;
    virtual void Unmap( ID3D11Resource *, UINT ) = 0;
    virtual void IASetVertexBuffers( UINT, UINT, ID3D11Buffer * const *, const UINT *, const UINT * ) = 0;
    virtual void IASetIndexBuffer( ID3D11Buffer *, DXGI_FORMAT, UINT ) = 0;
    virtual void Draw( UINT, UINT ) = 0;
    virtual void DrawIndexed( UINT, UINT, int ) = 0;
    virtual void IASetInputLayout( ID3D11InputLayout * ) = 0;
    virtual void VSSetShader( ID3D11VertexShader *, ID3D11ClassInstance * const *, UINT ) = 0;
    virtual void PSSetShader( ID3D11PixelShader *, ID3D11ClassInstance * const *, UINT ) = 0;
    virtual void VSSetConstantBuffers( UINT, UINT, ID3D11Buffer * const * ) = 0;
    virtual void PSSetConstantBuffers( UINT, UINT, ID3D11Buffer * const * ) = 0;
    virtual void PSSetShaderResources( UINT, UINT, ID3D11ShaderResourceView * const * ) = 0;
    virtual void PSSetSamplers( UINT, UINT, ID3D11SamplerState * const * ) = 0;
    virtual void OMSetBlendState( ID3D11BlendState *, const float *, UINT ) = 0;
    virtual void UpdateSubresource( ID3D11Resource *, UINT, const D3D11_BOX *, const void *, UINT, UINT ) = 0;
    virtual void OMSetRenderTargets( UINT, ID3D11RenderTargetView * const *, ID3D11DepthStencilView * ) = 0;
    virtual void OMSetDepthStencilState( ID3D11DepthStencilState *, UINT ) = 0;
    virtual void IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY ) = 0;
    virtual void ClearRenderTargetView( ID3D11RenderTargetView *, const float * ) = 0;
    virtual void ClearDepthStencilView( ID3D11DepthStencilView *, UINT, float, unsigned char ) = 0;
    virtual void RSSetState( ID3D11RasterizerState * ) = 0;
    virtual void RSSetViewports( UINT, const D3D11_VIEWPORT * ) = 0;
};
HRESULT __stdcall D3D11CreateDeviceAndSwapChain( IDXGIAdapter *, D3D_DRIVER_TYPE, void *, UINT, const D3D_FEATURE_LEVEL *, UINT, UINT,
    const DXGI_SWAP_CHAIN_DESC *, IDXGISwapChain **, ID3D11Device **, D3D_FEATURE_LEVEL *, ID3D11DeviceContext ** );
