#define __int8 char
#define __int16 short
#define __int32 int
#define __int64 long long

struct _GUID;
struct DXGI_SWAP_CHAIN_DESC;
struct DXGI_MODE_DESC;
struct HWND__;
struct tagRECT;
union _LARGE_INTEGER;
struct ID3D11BlendStateVtbl;
struct ID3D11Device;
struct ID3D11DeviceVtbl;
struct D3D11_SUBRESOURCE_DATA;
struct ID3D11Buffer;
struct ID3D11BufferVtbl;
struct IUnknown;
struct IUnknownVtbl;
struct D3D11_TEXTURE1D_DESC;
struct ID3D11Texture1D;
struct ID3D11Texture1DVtbl;
struct ID3D11Texture2D;
struct ID3D11Texture2DVtbl;
struct D3D11_TEXTURE3D_DESC;
struct ID3D11Texture3D;
struct ID3D11Texture3DVtbl;
struct ID3D11Resource;
struct ID3D11ResourceVtbl;
struct ID3D11ShaderResourceView;
struct ID3D11ShaderResourceViewVtbl;
struct D3D11_UNORDERED_ACCESS_VIEW_DESC;
struct ID3D11UnorderedAccessView;
struct ID3D11UnorderedAccessViewVtbl;
struct ID3D11RenderTargetView;
struct ID3D11RenderTargetViewVtbl;
struct D3D11_DEPTH_STENCIL_VIEW_DESC;
struct ID3D11DepthStencilView;
struct ID3D11DepthStencilViewVtbl;
struct ID3D11InputLayout;
struct ID3D11InputLayoutVtbl;
struct ID3D11ClassLinkage;
struct ID3D11ClassLinkageVtbl;
struct ID3D11ClassInstance;
struct ID3D11ClassInstanceVtbl;
struct D3D11_CLASS_INSTANCE_DESC;
struct ID3D11VertexShader;
struct ID3D11VertexShaderVtbl;
struct ID3D11GeometryShader;
struct ID3D11GeometryShaderVtbl;
struct D3D11_SO_DECLARATION_ENTRY;
struct ID3D11PixelShader;
struct ID3D11PixelShaderVtbl;
struct ID3D11HullShader;
struct ID3D11HullShaderVtbl;
struct ID3D11DomainShader;
struct ID3D11DomainShaderVtbl;
struct ID3D11ComputeShader;
struct ID3D11ComputeShaderVtbl;
struct ID3D11DepthStencilState;
struct ID3D11DepthStencilStateVtbl;
struct ID3D11RasterizerState;
struct ID3D11RasterizerStateVtbl;
struct ID3D11SamplerState;
struct ID3D11SamplerStateVtbl;
struct D3D11_QUERY_DESC;
struct ID3D11Query;
struct ID3D11QueryVtbl;
struct ID3D11Predicate;
struct ID3D11PredicateVtbl;
struct D3D11_COUNTER_DESC;
struct ID3D11Counter;
struct ID3D11CounterVtbl;
struct ID3D11DeviceContext;
struct ID3D11DeviceContextVtbl;
struct ID3D11Asynchronous;
struct ID3D11AsynchronousVtbl;
struct D3D11_BOX;
struct ID3D11CommandList;
struct ID3D11CommandListVtbl;
struct D3D11_COUNTER_INFO;
struct IDXGIAdapterVtbl;
struct IDXGIOutput;
struct IDXGIOutputVtbl;
struct DXGI_OUTPUT_DESC;
struct HMONITOR__;
struct DXGI_GAMMA_CONTROL_CAPABILITIES;
struct DXGI_GAMMA_CONTROL;
struct IDXGISurface;
struct IDXGISurfaceVtbl;
struct DXGI_SURFACE_DESC;
struct DXGI_MAPPED_RECT;
struct DXGI_FRAME_STATISTICS;
struct DXGI_ADAPTER_DESC;
struct IDXGISwapChainVtbl;

/* 30 */
typedef int BOOL;

/* 196 */
enum D3D11_BLEND
{
  D3D11_BLEND_ZERO = 0x1,
  D3D11_BLEND_ONE = 0x2,
  D3D11_BLEND_SRC_COLOR = 0x3,
  D3D11_BLEND_INV_SRC_COLOR = 0x4,
  D3D11_BLEND_SRC_ALPHA = 0x5,
  D3D11_BLEND_INV_SRC_ALPHA = 0x6,
  D3D11_BLEND_DEST_ALPHA = 0x7,
  D3D11_BLEND_INV_DEST_ALPHA = 0x8,
  D3D11_BLEND_DEST_COLOR = 0x9,
  D3D11_BLEND_INV_DEST_COLOR = 0xA,
  D3D11_BLEND_SRC_ALPHA_SAT = 0xB,
  D3D11_BLEND_BLEND_FACTOR = 0xE,
  D3D11_BLEND_INV_BLEND_FACTOR = 0xF,
  D3D11_BLEND_SRC1_COLOR = 0x10,
  D3D11_BLEND_INV_SRC1_COLOR = 0x11,
  D3D11_BLEND_SRC1_ALPHA = 0x12,
  D3D11_BLEND_INV_SRC1_ALPHA = 0x13,
};

/* 197 */
enum D3D11_BLEND_OP
{
  D3D11_BLEND_OP_ADD = 0x1,
  D3D11_BLEND_OP_SUBTRACT = 0x2,
  D3D11_BLEND_OP_REV_SUBTRACT = 0x3,
  D3D11_BLEND_OP_MIN = 0x4,
  D3D11_BLEND_OP_MAX = 0x5,
};

/* 198 */
typedef unsigned __int8 UINT8;

/* 195 */
struct D3D11_RENDER_TARGET_BLEND_DESC
{
  BOOL BlendEnable;
  D3D11_BLEND SrcBlend;
  D3D11_BLEND DestBlend;
  D3D11_BLEND_OP BlendOp;
  D3D11_BLEND SrcBlendAlpha;
  D3D11_BLEND DestBlendAlpha;
  D3D11_BLEND_OP BlendOpAlpha;
  UINT8 RenderTargetWriteMask;
};

/* 194 */
struct D3D11_BLEND_DESC
{
  BOOL AlphaToCoverageEnable;
  BOOL IndependentBlendEnable;
  D3D11_RENDER_TARGET_BLEND_DESC RenderTarget[8];
};

/* 21 */
typedef unsigned int UINT;

/* 200 */
enum D3D11_USAGE
{
  D3D11_USAGE_DEFAULT = 0x0,
  D3D11_USAGE_IMMUTABLE = 0x1,
  D3D11_USAGE_DYNAMIC = 0x2,
  D3D11_USAGE_STAGING = 0x3,
};

/* 199 */
struct D3D11_BUFFER_DESC
{
  UINT ByteWidth;
  D3D11_USAGE Usage;
  UINT BindFlags;
  UINT CPUAccessFlags;
  UINT MiscFlags;
  UINT StructureByteStride;
};

/* 202 */
enum D3D11_DEPTH_WRITE_MASK
{
  D3D11_DEPTH_WRITE_MASK_ZERO = 0x0,
  D3D11_DEPTH_WRITE_MASK_ALL = 0x1,
};

/* 203 */
enum D3D11_COMPARISON_FUNC
{
  D3D11_COMPARISON_NEVER = 0x1,
  D3D11_COMPARISON_LESS = 0x2,
  D3D11_COMPARISON_EQUAL = 0x3,
  D3D11_COMPARISON_LESS_EQUAL = 0x4,
  D3D11_COMPARISON_GREATER = 0x5,
  D3D11_COMPARISON_NOT_EQUAL = 0x6,
  D3D11_COMPARISON_GREATER_EQUAL = 0x7,
  D3D11_COMPARISON_ALWAYS = 0x8,
};

/* 205 */
enum D3D11_STENCIL_OP
{
  D3D11_STENCIL_OP_KEEP = 0x1,
  D3D11_STENCIL_OP_ZERO = 0x2,
  D3D11_STENCIL_OP_REPLACE = 0x3,
  D3D11_STENCIL_OP_INCR_SAT = 0x4,
  D3D11_STENCIL_OP_DECR_SAT = 0x5,
  D3D11_STENCIL_OP_INVERT = 0x6,
  D3D11_STENCIL_OP_INCR = 0x7,
  D3D11_STENCIL_OP_DECR = 0x8,
};

/* 204 */
struct D3D11_DEPTH_STENCILOP_DESC
{
  D3D11_STENCIL_OP StencilFailOp;
  D3D11_STENCIL_OP StencilDepthFailOp;
  D3D11_STENCIL_OP StencilPassOp;
  D3D11_COMPARISON_FUNC StencilFunc;
};

/* 201 */
struct D3D11_DEPTH_STENCIL_DESC
{
  BOOL DepthEnable;
  D3D11_DEPTH_WRITE_MASK DepthWriteMask;
  D3D11_COMPARISON_FUNC DepthFunc;
  BOOL StencilEnable;
  UINT8 StencilReadMask;
  UINT8 StencilWriteMask;
  D3D11_DEPTH_STENCILOP_DESC FrontFace;
  D3D11_DEPTH_STENCILOP_DESC BackFace;
};

/* 83 */
typedef char CHAR;

/* 207 */
typedef const CHAR *LPCSTR;

/* 23 */
enum DXGI_FORMAT
{
  DXGI_FORMAT_UNKNOWN = 0x0,
  DXGI_FORMAT_R32G32B32A32_TYPELESS = 0x1,
  DXGI_FORMAT_R32G32B32A32_FLOAT = 0x2,
  DXGI_FORMAT_R32G32B32A32_UINT = 0x3,
  DXGI_FORMAT_R32G32B32A32_SINT = 0x4,
  DXGI_FORMAT_R32G32B32_TYPELESS = 0x5,
  DXGI_FORMAT_R32G32B32_FLOAT = 0x6,
  DXGI_FORMAT_R32G32B32_UINT = 0x7,
  DXGI_FORMAT_R32G32B32_SINT = 0x8,
  DXGI_FORMAT_R16G16B16A16_TYPELESS = 0x9,
  DXGI_FORMAT_R16G16B16A16_FLOAT = 0xA,
  DXGI_FORMAT_R16G16B16A16_UNORM = 0xB,
  DXGI_FORMAT_R16G16B16A16_UINT = 0xC,
  DXGI_FORMAT_R16G16B16A16_SNORM = 0xD,
  DXGI_FORMAT_R16G16B16A16_SINT = 0xE,
  DXGI_FORMAT_R32G32_TYPELESS = 0xF,
  DXGI_FORMAT_R32G32_FLOAT = 0x10,
  DXGI_FORMAT_R32G32_UINT = 0x11,
  DXGI_FORMAT_R32G32_SINT = 0x12,
  DXGI_FORMAT_R32G8X24_TYPELESS = 0x13,
  DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 0x14,
  DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 0x15,
  DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 0x16,
  DXGI_FORMAT_R10G10B10A2_TYPELESS = 0x17,
  DXGI_FORMAT_R10G10B10A2_UNORM = 0x18,
  DXGI_FORMAT_R10G10B10A2_UINT = 0x19,
  DXGI_FORMAT_R11G11B10_FLOAT = 0x1A,
  DXGI_FORMAT_R8G8B8A8_TYPELESS = 0x1B,
  DXGI_FORMAT_R8G8B8A8_UNORM = 0x1C,
  DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 0x1D,
  DXGI_FORMAT_R8G8B8A8_UINT = 0x1E,
  DXGI_FORMAT_R8G8B8A8_SNORM = 0x1F,
  DXGI_FORMAT_R8G8B8A8_SINT = 0x20,
  DXGI_FORMAT_R16G16_TYPELESS = 0x21,
  DXGI_FORMAT_R16G16_FLOAT = 0x22,
  DXGI_FORMAT_R16G16_UNORM = 0x23,
  DXGI_FORMAT_R16G16_UINT = 0x24,
  DXGI_FORMAT_R16G16_SNORM = 0x25,
  DXGI_FORMAT_R16G16_SINT = 0x26,
  DXGI_FORMAT_R32_TYPELESS = 0x27,
  DXGI_FORMAT_D32_FLOAT = 0x28,
  DXGI_FORMAT_R32_FLOAT = 0x29,
  DXGI_FORMAT_R32_UINT = 0x2A,
  DXGI_FORMAT_R32_SINT = 0x2B,
  DXGI_FORMAT_R24G8_TYPELESS = 0x2C,
  DXGI_FORMAT_D24_UNORM_S8_UINT = 0x2D,
  DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 0x2E,
  DXGI_FORMAT_X24_TYPELESS_G8_UINT = 0x2F,
  DXGI_FORMAT_R8G8_TYPELESS = 0x30,
  DXGI_FORMAT_R8G8_UNORM = 0x31,
  DXGI_FORMAT_R8G8_UINT = 0x32,
  DXGI_FORMAT_R8G8_SNORM = 0x33,
  DXGI_FORMAT_R8G8_SINT = 0x34,
  DXGI_FORMAT_R16_TYPELESS = 0x35,
  DXGI_FORMAT_R16_FLOAT = 0x36,
  DXGI_FORMAT_D16_UNORM = 0x37,
  DXGI_FORMAT_R16_UNORM = 0x38,
  DXGI_FORMAT_R16_UINT = 0x39,
  DXGI_FORMAT_R16_SNORM = 0x3A,
  DXGI_FORMAT_R16_SINT = 0x3B,
  DXGI_FORMAT_R8_TYPELESS = 0x3C,
  DXGI_FORMAT_R8_UNORM = 0x3D,
  DXGI_FORMAT_R8_UINT = 0x3E,
  DXGI_FORMAT_R8_SNORM = 0x3F,
  DXGI_FORMAT_R8_SINT = 0x40,
  DXGI_FORMAT_A8_UNORM = 0x41,
  DXGI_FORMAT_R1_UNORM = 0x42,
  DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 0x43,
  DXGI_FORMAT_R8G8_B8G8_UNORM = 0x44,
  DXGI_FORMAT_G8R8_G8B8_UNORM = 0x45,
  DXGI_FORMAT_BC1_TYPELESS = 0x46,
  DXGI_FORMAT_BC1_UNORM = 0x47,
  DXGI_FORMAT_BC1_UNORM_SRGB = 0x48,
  DXGI_FORMAT_BC2_TYPELESS = 0x49,
  DXGI_FORMAT_BC2_UNORM = 0x4A,
  DXGI_FORMAT_BC2_UNORM_SRGB = 0x4B,
  DXGI_FORMAT_BC3_TYPELESS = 0x4C,
  DXGI_FORMAT_BC3_UNORM = 0x4D,
  DXGI_FORMAT_BC3_UNORM_SRGB = 0x4E,
  DXGI_FORMAT_BC4_TYPELESS = 0x4F,
  DXGI_FORMAT_BC4_UNORM = 0x50,
  DXGI_FORMAT_BC4_SNORM = 0x51,
  DXGI_FORMAT_BC5_TYPELESS = 0x52,
  DXGI_FORMAT_BC5_UNORM = 0x53,
  DXGI_FORMAT_BC5_SNORM = 0x54,
  DXGI_FORMAT_B5G6R5_UNORM = 0x55,
  DXGI_FORMAT_B5G5R5A1_UNORM = 0x56,
  DXGI_FORMAT_B8G8R8A8_UNORM = 0x57,
  DXGI_FORMAT_B8G8R8X8_UNORM = 0x58,
  DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 0x59,
  DXGI_FORMAT_B8G8R8A8_TYPELESS = 0x5A,
  DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 0x5B,
  DXGI_FORMAT_B8G8R8X8_TYPELESS = 0x5C,
  DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 0x5D,
  DXGI_FORMAT_BC6H_TYPELESS = 0x5E,
  DXGI_FORMAT_BC6H_UF16 = 0x5F,
  DXGI_FORMAT_BC6H_SF16 = 0x60,
  DXGI_FORMAT_BC7_TYPELESS = 0x61,
  DXGI_FORMAT_BC7_UNORM = 0x62,
  DXGI_FORMAT_BC7_UNORM_SRGB = 0x63,
  DXGI_FORMAT_AYUV = 0x64,
  DXGI_FORMAT_Y410 = 0x65,
  DXGI_FORMAT_Y416 = 0x66,
  DXGI_FORMAT_NV12 = 0x67,
  DXGI_FORMAT_P010 = 0x68,
  DXGI_FORMAT_P016 = 0x69,
  DXGI_FORMAT_420_OPAQUE = 0x6A,
  DXGI_FORMAT_YUY2 = 0x6B,
  DXGI_FORMAT_Y210 = 0x6C,
  DXGI_FORMAT_Y216 = 0x6D,
  DXGI_FORMAT_NV11 = 0x6E,
  DXGI_FORMAT_AI44 = 0x6F,
  DXGI_FORMAT_IA44 = 0x70,
  DXGI_FORMAT_P8 = 0x71,
  DXGI_FORMAT_A8P8 = 0x72,
  DXGI_FORMAT_B4G4R4A4_UNORM = 0x73,
  DXGI_FORMAT_P208 = 0x82,
  DXGI_FORMAT_V208 = 0x83,
  DXGI_FORMAT_V408 = 0x84,
  DXGI_FORMAT_FORCE_UINT = 0xFFFFFFFF,
};

/* 208 */
enum D3D11_INPUT_CLASSIFICATION
{
  D3D11_INPUT_PER_VERTEX_DATA = 0x0,
  D3D11_INPUT_PER_INSTANCE_DATA = 0x1,
};

/* 206 */
struct D3D11_INPUT_ELEMENT_DESC
{
  LPCSTR SemanticName;
  UINT SemanticIndex;
  DXGI_FORMAT Format;
  UINT InputSlot;
  UINT AlignedByteOffset;
  D3D11_INPUT_CLASSIFICATION InputSlotClass;
  UINT InstanceDataStepRate;
};

/* 209 */
struct D3D11_MAPPED_SUBRESOURCE
{
  void *pData;
  UINT RowPitch;
  UINT DepthPitch;
};

/* 211 */
enum D3D11_FILL_MODE
{
  D3D11_FILL_WIREFRAME = 0x2,
  D3D11_FILL_SOLID = 0x3,
};

/* 212 */
enum D3D11_CULL_MODE
{
  D3D11_CULL_NONE = 0x1,
  D3D11_CULL_FRONT = 0x2,
  D3D11_CULL_BACK = 0x3,
};

/* 213 */
typedef int INT;

/* 214 */
typedef float FLOAT;

/* 210 */
struct D3D11_RASTERIZER_DESC
{
  D3D11_FILL_MODE FillMode;
  D3D11_CULL_MODE CullMode;
  BOOL FrontCounterClockwise;
  INT DepthBias;
  FLOAT DepthBiasClamp;
  FLOAT SlopeScaledDepthBias;
  BOOL DepthClipEnable;
  BOOL ScissorEnable;
  BOOL MultisampleEnable;
  BOOL AntialiasedLineEnable;
};

/* 216 */
enum D3D11_RTV_DIMENSION
{
  D3D11_RTV_DIMENSION_UNKNOWN = 0x0,
  D3D11_RTV_DIMENSION_BUFFER = 0x1,
  D3D11_RTV_DIMENSION_TEXTURE1D = 0x2,
  D3D11_RTV_DIMENSION_TEXTURE1DARRAY = 0x3,
  D3D11_RTV_DIMENSION_TEXTURE2D = 0x4,
  D3D11_RTV_DIMENSION_TEXTURE2DARRAY = 0x5,
  D3D11_RTV_DIMENSION_TEXTURE2DMS = 0x6,
  D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY = 0x7,
  D3D11_RTV_DIMENSION_TEXTURE3D = 0x8,
};

/* 219 */
union D3D11_BUFFER_RTV::$636FCB046847EEDF15856CE7C35FCB7F
{
  UINT FirstElement;
  UINT ElementOffset;
};

/* 220 */
union D3D11_BUFFER_RTV::$5EA6C023B6E595B5E45F8B856510107D
{
  UINT NumElements;
  UINT ElementWidth;
};

/* 218 */
struct D3D11_BUFFER_RTV
{
  union
  {
    UINT FirstElement;
    UINT ElementOffset;
  };
  union
  {
    UINT NumElements;
    UINT ElementWidth;
  };
};

/* 221 */
struct D3D11_TEX1D_RTV
{
  UINT MipSlice;
};

/* 222 */
struct D3D11_TEX1D_ARRAY_RTV
{
  UINT MipSlice;
  UINT FirstArraySlice;
  UINT ArraySize;
};

/* 223 */
struct D3D11_TEX2D_RTV
{
  UINT MipSlice;
};

/* 224 */
struct D3D11_TEX2D_ARRAY_RTV
{
  UINT MipSlice;
  UINT FirstArraySlice;
  UINT ArraySize;
};

/* 225 */
struct D3D11_TEX2DMS_RTV
{
  UINT UnusedField_NothingToDefine;
};

/* 226 */
struct D3D11_TEX2DMS_ARRAY_RTV
{
  UINT FirstArraySlice;
  UINT ArraySize;
};

/* 227 */
struct D3D11_TEX3D_RTV
{
  UINT MipSlice;
  UINT FirstWSlice;
  UINT WSize;
};

/* 217 */
union D3D11_RENDER_TARGET_VIEW_DESC::$C25145134444453513A60B25C9F48069
{
  D3D11_BUFFER_RTV Buffer;
  D3D11_TEX1D_RTV Texture1D;
  D3D11_TEX1D_ARRAY_RTV Texture1DArray;
  D3D11_TEX2D_RTV Texture2D;
  D3D11_TEX2D_ARRAY_RTV Texture2DArray;
  D3D11_TEX2DMS_RTV Texture2DMS;
  D3D11_TEX2DMS_ARRAY_RTV Texture2DMSArray;
  D3D11_TEX3D_RTV Texture3D;
};

/* 215 */
struct D3D11_RENDER_TARGET_VIEW_DESC
{
  DXGI_FORMAT Format;
  D3D11_RTV_DIMENSION ViewDimension;
  union
  {
    D3D11_BUFFER_RTV Buffer;
    D3D11_TEX1D_RTV Texture1D;
    D3D11_TEX1D_ARRAY_RTV Texture1DArray;
    D3D11_TEX2D_RTV Texture2D;
    D3D11_TEX2D_ARRAY_RTV Texture2DArray;
    D3D11_TEX2DMS_RTV Texture2DMS;
    D3D11_TEX2DMS_ARRAY_RTV Texture2DMSArray;
    D3D11_TEX3D_RTV Texture3D;
  };
};

/* 229 */
enum D3D11_FILTER
{
  D3D11_FILTER_MIN_MAG_MIP_POINT = 0x0,
  D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR = 0x1,
  D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x4,
  D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR = 0x5,
  D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT = 0x10,
  D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x11,
  D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT = 0x14,
  D3D11_FILTER_MIN_MAG_MIP_LINEAR = 0x15,
  D3D11_FILTER_ANISOTROPIC = 0x55,
  D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT = 0x80,
  D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR = 0x81,
  D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x84,
  D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR = 0x85,
  D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT = 0x90,
  D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x91,
  D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT = 0x94,
  D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR = 0x95,
  D3D11_FILTER_COMPARISON_ANISOTROPIC = 0xD5,
  D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT = 0x100,
  D3D11_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x101,
  D3D11_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x104,
  D3D11_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x105,
  D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x110,
  D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x111,
  D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x114,
  D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR = 0x115,
  D3D11_FILTER_MINIMUM_ANISOTROPIC = 0x155,
  D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT = 0x180,
  D3D11_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x181,
  D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x184,
  D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x185,
  D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x190,
  D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x191,
  D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x194,
  D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR = 0x195,
  D3D11_FILTER_MAXIMUM_ANISOTROPIC = 0x1D5,
};

/* 230 */
enum D3D11_TEXTURE_ADDRESS_MODE
{
  D3D11_TEXTURE_ADDRESS_WRAP = 0x1,
  D3D11_TEXTURE_ADDRESS_MIRROR = 0x2,
  D3D11_TEXTURE_ADDRESS_CLAMP = 0x3,
  D3D11_TEXTURE_ADDRESS_BORDER = 0x4,
  D3D11_TEXTURE_ADDRESS_MIRROR_ONCE = 0x5,
};

/* 228 */
struct D3D11_SAMPLER_DESC
{
  D3D11_FILTER Filter;
  D3D11_TEXTURE_ADDRESS_MODE AddressU;
  D3D11_TEXTURE_ADDRESS_MODE AddressV;
  D3D11_TEXTURE_ADDRESS_MODE AddressW;
  FLOAT MipLODBias;
  UINT MaxAnisotropy;
  D3D11_COMPARISON_FUNC ComparisonFunc;
  FLOAT BorderColor[4];
  FLOAT MinLOD;
  FLOAT MaxLOD;
};

/* 233 */
enum D3D_SRV_DIMENSION
{
  D3D_SRV_DIMENSION_UNKNOWN = 0x0,
  D3D_SRV_DIMENSION_BUFFER = 0x1,
  D3D_SRV_DIMENSION_TEXTURE1D = 0x2,
  D3D_SRV_DIMENSION_TEXTURE1DARRAY = 0x3,
  D3D_SRV_DIMENSION_TEXTURE2D = 0x4,
  D3D_SRV_DIMENSION_TEXTURE2DARRAY = 0x5,
  D3D_SRV_DIMENSION_TEXTURE2DMS = 0x6,
  D3D_SRV_DIMENSION_TEXTURE2DMSARRAY = 0x7,
  D3D_SRV_DIMENSION_TEXTURE3D = 0x8,
  D3D_SRV_DIMENSION_TEXTURECUBE = 0x9,
  D3D_SRV_DIMENSION_TEXTURECUBEARRAY = 0xA,
  D3D_SRV_DIMENSION_BUFFEREX = 0xB,
  D3D10_SRV_DIMENSION_UNKNOWN = 0x0,
  D3D10_SRV_DIMENSION_BUFFER = 0x1,
  D3D10_SRV_DIMENSION_TEXTURE1D = 0x2,
  D3D10_SRV_DIMENSION_TEXTURE1DARRAY = 0x3,
  D3D10_SRV_DIMENSION_TEXTURE2D = 0x4,
  D3D10_SRV_DIMENSION_TEXTURE2DARRAY = 0x5,
  D3D10_SRV_DIMENSION_TEXTURE2DMS = 0x6,
  D3D10_SRV_DIMENSION_TEXTURE2DMSARRAY = 0x7,
  D3D10_SRV_DIMENSION_TEXTURE3D = 0x8,
  D3D10_SRV_DIMENSION_TEXTURECUBE = 0x9,
  D3D10_1_SRV_DIMENSION_UNKNOWN = 0x0,
  D3D10_1_SRV_DIMENSION_BUFFER = 0x1,
  D3D10_1_SRV_DIMENSION_TEXTURE1D = 0x2,
  D3D10_1_SRV_DIMENSION_TEXTURE1DARRAY = 0x3,
  D3D10_1_SRV_DIMENSION_TEXTURE2D = 0x4,
  D3D10_1_SRV_DIMENSION_TEXTURE2DARRAY = 0x5,
  D3D10_1_SRV_DIMENSION_TEXTURE2DMS = 0x6,
  D3D10_1_SRV_DIMENSION_TEXTURE2DMSARRAY = 0x7,
  D3D10_1_SRV_DIMENSION_TEXTURE3D = 0x8,
  D3D10_1_SRV_DIMENSION_TEXTURECUBE = 0x9,
  D3D10_1_SRV_DIMENSION_TEXTURECUBEARRAY = 0xA,
  D3D11_SRV_DIMENSION_UNKNOWN = 0x0,
  D3D11_SRV_DIMENSION_BUFFER = 0x1,
  D3D11_SRV_DIMENSION_TEXTURE1D = 0x2,
  D3D11_SRV_DIMENSION_TEXTURE1DARRAY = 0x3,
  D3D11_SRV_DIMENSION_TEXTURE2D = 0x4,
  D3D11_SRV_DIMENSION_TEXTURE2DARRAY = 0x5,
  D3D11_SRV_DIMENSION_TEXTURE2DMS = 0x6,
  D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY = 0x7,
  D3D11_SRV_DIMENSION_TEXTURE3D = 0x8,
  D3D11_SRV_DIMENSION_TEXTURECUBE = 0x9,
  D3D11_SRV_DIMENSION_TEXTURECUBEARRAY = 0xA,
  D3D11_SRV_DIMENSION_BUFFEREX = 0xB,
};

/* 232 */
typedef D3D_SRV_DIMENSION D3D11_SRV_DIMENSION;

/* 236 */
union D3D11_BUFFER_SRV::$636FCB046847EEDF15856CE7C35FCB7F
{
  UINT FirstElement;
  UINT ElementOffset;
};

/* 237 */
union D3D11_BUFFER_SRV::$5EA6C023B6E595B5E45F8B856510107D
{
  UINT NumElements;
  UINT ElementWidth;
};

/* 235 */
struct D3D11_BUFFER_SRV
{
  union
  {
    UINT FirstElement;
    UINT ElementOffset;
  };
  union
  {
    UINT NumElements;
    UINT ElementWidth;
  };
};

/* 238 */
struct D3D11_TEX1D_SRV
{
  UINT MostDetailedMip;
  UINT MipLevels;
};

/* 239 */
struct D3D11_TEX1D_ARRAY_SRV
{
  UINT MostDetailedMip;
  UINT MipLevels;
  UINT FirstArraySlice;
  UINT ArraySize;
};

/* 240 */
struct D3D11_TEX2D_SRV
{
  UINT MostDetailedMip;
  UINT MipLevels;
};

/* 241 */
struct D3D11_TEX2D_ARRAY_SRV
{
  UINT MostDetailedMip;
  UINT MipLevels;
  UINT FirstArraySlice;
  UINT ArraySize;
};

/* 242 */
struct D3D11_TEX2DMS_SRV
{
  UINT UnusedField_NothingToDefine;
};

/* 243 */
struct D3D11_TEX2DMS_ARRAY_SRV
{
  UINT FirstArraySlice;
  UINT ArraySize;
};

/* 244 */
struct D3D11_TEX3D_SRV
{
  UINT MostDetailedMip;
  UINT MipLevels;
};

/* 245 */
struct D3D11_TEXCUBE_SRV
{
  UINT MostDetailedMip;
  UINT MipLevels;
};

/* 246 */
struct D3D11_TEXCUBE_ARRAY_SRV
{
  UINT MostDetailedMip;
  UINT MipLevels;
  UINT First2DArrayFace;
  UINT NumCubes;
};

/* 247 */
struct D3D11_BUFFEREX_SRV
{
  UINT FirstElement;
  UINT NumElements;
  UINT Flags;
};

/* 234 */
union D3D11_SHADER_RESOURCE_VIEW_DESC::$9432D0D16C541F7FDBD711DB3059C804
{
  D3D11_BUFFER_SRV Buffer;
  D3D11_TEX1D_SRV Texture1D;
  D3D11_TEX1D_ARRAY_SRV Texture1DArray;
  D3D11_TEX2D_SRV Texture2D;
  D3D11_TEX2D_ARRAY_SRV Texture2DArray;
  D3D11_TEX2DMS_SRV Texture2DMS;
  D3D11_TEX2DMS_ARRAY_SRV Texture2DMSArray;
  D3D11_TEX3D_SRV Texture3D;
  D3D11_TEXCUBE_SRV TextureCube;
  D3D11_TEXCUBE_ARRAY_SRV TextureCubeArray;
  D3D11_BUFFEREX_SRV BufferEx;
};

/* 231 */
struct D3D11_SHADER_RESOURCE_VIEW_DESC
{
  DXGI_FORMAT Format;
  D3D11_SRV_DIMENSION ViewDimension;
  union
  {
    D3D11_BUFFER_SRV Buffer;
    D3D11_TEX1D_SRV Texture1D;
    D3D11_TEX1D_ARRAY_SRV Texture1DArray;
    D3D11_TEX2D_SRV Texture2D;
    D3D11_TEX2D_ARRAY_SRV Texture2DArray;
    D3D11_TEX2DMS_SRV Texture2DMS;
    D3D11_TEX2DMS_ARRAY_SRV Texture2DMSArray;
    D3D11_TEX3D_SRV Texture3D;
    D3D11_TEXCUBE_SRV TextureCube;
    D3D11_TEXCUBE_ARRAY_SRV TextureCubeArray;
    D3D11_BUFFEREX_SRV BufferEx;
  };
};

/* 26 */
struct DXGI_SAMPLE_DESC
{
  UINT Count;
  UINT Quality;
};

/* 248 */
struct D3D11_TEXTURE2D_DESC
{
  UINT Width;
  UINT Height;
  UINT MipLevels;
  UINT ArraySize;
  DXGI_FORMAT Format;
  DXGI_SAMPLE_DESC SampleDesc;
  D3D11_USAGE Usage;
  UINT BindFlags;
  UINT CPUAccessFlags;
  UINT MiscFlags;
};

/* 249 */
struct D3D11_VIEWPORT
{
  FLOAT TopLeftX;
  FLOAT TopLeftY;
  FLOAT Width;
  FLOAT Height;
  FLOAT MinDepth;
  FLOAT MaxDepth;
};

/* 250 */
enum D3D_DRIVER_TYPE
{
  D3D_DRIVER_TYPE_UNKNOWN = 0x0,
  D3D_DRIVER_TYPE_HARDWARE = 0x1,
  D3D_DRIVER_TYPE_REFERENCE = 0x2,
  D3D_DRIVER_TYPE_NULL = 0x3,
  D3D_DRIVER_TYPE_SOFTWARE = 0x4,
  D3D_DRIVER_TYPE_WARP = 0x5,
};

/* 251 */
enum D3D_FEATURE_LEVEL
{
  D3D_FEATURE_LEVEL_9_1 = 0x9100,
  D3D_FEATURE_LEVEL_9_2 = 0x9200,
  D3D_FEATURE_LEVEL_9_3 = 0x9300,
  D3D_FEATURE_LEVEL_10_0 = 0xA000,
  D3D_FEATURE_LEVEL_10_1 = 0xA100,
  D3D_FEATURE_LEVEL_11_0 = 0xB000,
  D3D_FEATURE_LEVEL_11_1 = 0xB100,
  D3D_FEATURE_LEVEL_12_0 = 0xC000,
  D3D_FEATURE_LEVEL_12_1 = 0xC100,
};

/* 252 */
struct ID3D11BlendState
{
  struct ID3D11BlendStateVtbl *lpVtbl;
};

/* 254 */
typedef int HRESULT;

/* 2 */
typedef struct _GUID GUID;

/* 32 */
typedef GUID IID;

/* 255 */
typedef unsigned int ULONG;

/* 253 */
struct ID3D11BlendStateVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11BlendState *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11BlendState *This);
  ULONG (__stdcall *Release)(ID3D11BlendState *This);
  void (__stdcall *GetDevice)(ID3D11BlendState *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11BlendState *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11BlendState *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11BlendState *This, const GUID *const guid, const IUnknown *pData);
  void (__stdcall *GetDesc)(ID3D11BlendState *This, D3D11_BLEND_DESC *pDesc);
};

/* 1 */
struct _GUID
{
  unsigned int Data1;
  unsigned __int16 Data2;
  unsigned __int16 Data3;
  unsigned __int8 Data4[8];
};

/* 256 */
struct ID3D11Device
{
  struct ID3D11DeviceVtbl *lpVtbl;
};

/* 261 */
#pragma pack(push, 8)
struct IUnknown
{
  struct IUnknownVtbl *lpVtbl;
};
#pragma pack(pop)

/* 60 */
typedef unsigned int ULONG_PTR;

/* 84 */
typedef ULONG_PTR SIZE_T;

/* 14 */
typedef void *HANDLE;

/* 349 */
enum D3D11_COUNTER_TYPE
{
  D3D11_COUNTER_TYPE_FLOAT32 = 0x0,
  D3D11_COUNTER_TYPE_UINT16 = 0x1,
  D3D11_COUNTER_TYPE_UINT32 = 0x2,
  D3D11_COUNTER_TYPE_UINT64 = 0x3,
};

/* 82 */
typedef CHAR *LPSTR;

/* 350 */
enum D3D11_FEATURE
{
  D3D11_FEATURE_THREADING = 0x0,
  D3D11_FEATURE_DOUBLES = 0x1,
  D3D11_FEATURE_FORMAT_SUPPORT = 0x2,
  D3D11_FEATURE_FORMAT_SUPPORT2 = 0x3,
  D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS = 0x4,
  D3D11_FEATURE_D3D11_OPTIONS = 0x5,
  D3D11_FEATURE_ARCHITECTURE_INFO = 0x6,
  D3D11_FEATURE_D3D9_OPTIONS = 0x7,
  D3D11_FEATURE_SHADER_MIN_PRECISION_SUPPORT = 0x8,
  D3D11_FEATURE_D3D9_SHADOW_SUPPORT = 0x9,
  D3D11_FEATURE_D3D11_OPTIONS1 = 0xA,
  D3D11_FEATURE_D3D9_SIMPLE_INSTANCING_SUPPORT = 0xB,
  D3D11_FEATURE_MARKER_SUPPORT = 0xC,
  D3D11_FEATURE_D3D9_OPTIONS1 = 0xD,
  D3D11_FEATURE_D3D11_OPTIONS2 = 0xE,
  D3D11_FEATURE_D3D11_OPTIONS3 = 0xF,
  D3D11_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT = 0x10,
  D3D11_FEATURE_D3D11_OPTIONS4 = 0x11,
  D3D11_FEATURE_SHADER_CACHE = 0x12,
  D3D11_FEATURE_D3D11_OPTIONS5 = 0x13,
};

/* 257 */
struct ID3D11DeviceVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11Device *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11Device *This);
  ULONG (__stdcall *Release)(ID3D11Device *This);
  HRESULT (__stdcall *CreateBuffer)(ID3D11Device *This, const D3D11_BUFFER_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Buffer **ppBuffer);
  HRESULT (__stdcall *CreateTexture1D)(ID3D11Device *This, const D3D11_TEXTURE1D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture1D **ppTexture1D);
  HRESULT (__stdcall *CreateTexture2D)(ID3D11Device *This, const D3D11_TEXTURE2D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture2D **ppTexture2D);
  HRESULT (__stdcall *CreateTexture3D)(ID3D11Device *This, const D3D11_TEXTURE3D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture3D **ppTexture3D);
  HRESULT (__stdcall *CreateShaderResourceView)(ID3D11Device *This, ID3D11Resource *pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc, ID3D11ShaderResourceView **ppSRView);
  HRESULT (__stdcall *CreateUnorderedAccessView)(ID3D11Device *This, ID3D11Resource *pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc, ID3D11UnorderedAccessView **ppUAView);
  HRESULT (__stdcall *CreateRenderTargetView)(ID3D11Device *This, ID3D11Resource *pResource, const D3D11_RENDER_TARGET_VIEW_DESC *pDesc, ID3D11RenderTargetView **ppRTView);
  HRESULT (__stdcall *CreateDepthStencilView)(ID3D11Device *This, ID3D11Resource *pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc, ID3D11DepthStencilView **ppDepthStencilView);
  HRESULT (__stdcall *CreateInputLayout)(ID3D11Device *This, const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs, UINT NumElements, const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, ID3D11InputLayout **ppInputLayout);
  HRESULT (__stdcall *CreateVertexShader)(ID3D11Device *This, const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11VertexShader **ppVertexShader);
  HRESULT (__stdcall *CreateGeometryShader)(ID3D11Device *This, const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11GeometryShader **ppGeometryShader);
  HRESULT (__stdcall *CreateGeometryShaderWithStreamOutput)(ID3D11Device *This, const void *pShaderBytecode, SIZE_T BytecodeLength, const D3D11_SO_DECLARATION_ENTRY *pSODeclaration, UINT NumEntries, const UINT *pBufferStrides, UINT NumStrides, UINT RasterizedStream, ID3D11ClassLinkage *pClassLinkage, ID3D11GeometryShader **ppGeometryShader);
  HRESULT (__stdcall *CreatePixelShader)(ID3D11Device *This, const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11PixelShader **ppPixelShader);
  HRESULT (__stdcall *CreateHullShader)(ID3D11Device *This, const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11HullShader **ppHullShader);
  HRESULT (__stdcall *CreateDomainShader)(ID3D11Device *This, const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11DomainShader **ppDomainShader);
  HRESULT (__stdcall *CreateComputeShader)(ID3D11Device *This, const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11ComputeShader **ppComputeShader);
  HRESULT (__stdcall *CreateClassLinkage)(ID3D11Device *This, ID3D11ClassLinkage **ppLinkage);
  HRESULT (__stdcall *CreateBlendState)(ID3D11Device *This, const D3D11_BLEND_DESC *pBlendStateDesc, ID3D11BlendState **ppBlendState);
  HRESULT (__stdcall *CreateDepthStencilState)(ID3D11Device *This, const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc, ID3D11DepthStencilState **ppDepthStencilState);
  HRESULT (__stdcall *CreateRasterizerState)(ID3D11Device *This, const D3D11_RASTERIZER_DESC *pRasterizerDesc, ID3D11RasterizerState **ppRasterizerState);
  HRESULT (__stdcall *CreateSamplerState)(ID3D11Device *This, const D3D11_SAMPLER_DESC *pSamplerDesc, ID3D11SamplerState **ppSamplerState);
  HRESULT (__stdcall *CreateQuery)(ID3D11Device *This, const D3D11_QUERY_DESC *pQueryDesc, ID3D11Query **ppQuery);
  HRESULT (__stdcall *CreatePredicate)(ID3D11Device *This, const D3D11_QUERY_DESC *pPredicateDesc, ID3D11Predicate **ppPredicate);
  HRESULT (__stdcall *CreateCounter)(ID3D11Device *This, const D3D11_COUNTER_DESC *pCounterDesc, ID3D11Counter **ppCounter);
  HRESULT (__stdcall *CreateDeferredContext)(ID3D11Device *This, UINT ContextFlags, ID3D11DeviceContext **ppDeferredContext);
  HRESULT (__stdcall *OpenSharedResource)(ID3D11Device *This, HANDLE hResource, const IID *const ReturnedInterface, void **ppResource);
  HRESULT (__stdcall *CheckFormatSupport)(ID3D11Device *This, DXGI_FORMAT Format, UINT *pFormatSupport);
  HRESULT (__stdcall *CheckMultisampleQualityLevels)(ID3D11Device *This, DXGI_FORMAT Format, UINT SampleCount, UINT *pNumQualityLevels);
  void (__stdcall *CheckCounterInfo)(ID3D11Device *This, D3D11_COUNTER_INFO *pCounterInfo);
  HRESULT (__stdcall *CheckCounter)(ID3D11Device *This, const D3D11_COUNTER_DESC *pDesc, D3D11_COUNTER_TYPE *pType, UINT *pActiveCounters, LPSTR szName, UINT *pNameLength, LPSTR szUnits, UINT *pUnitsLength, LPSTR szDescription, UINT *pDescriptionLength);
  HRESULT (__stdcall *CheckFeatureSupport)(ID3D11Device *This, D3D11_FEATURE Feature, void *pFeatureSupportData, UINT FeatureSupportDataSize);
  HRESULT (__stdcall *GetPrivateData)(ID3D11Device *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11Device *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11Device *This, const GUID *const guid, const IUnknown *pData);
  D3D_FEATURE_LEVEL (__stdcall *GetFeatureLevel)(ID3D11Device *This);
  UINT (__stdcall *GetCreationFlags)(ID3D11Device *This);
  HRESULT (__stdcall *GetDeviceRemovedReason)(ID3D11Device *This);
  void (__stdcall *GetImmediateContext)(ID3D11Device *This, ID3D11DeviceContext **ppImmediateContext);
  HRESULT (__stdcall *SetExceptionMode)(ID3D11Device *This, UINT RaiseFlags);
  UINT (__stdcall *GetExceptionMode)(ID3D11Device *This);
};

/* 262 */
#pragma pack(push, 8)
struct IUnknownVtbl
{
  HRESULT (__stdcall *QueryInterface)(IUnknown *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(IUnknown *This);
  ULONG (__stdcall *Release)(IUnknown *This);
};
#pragma pack(pop)

/* 258 */
struct D3D11_SUBRESOURCE_DATA
{
  const void *pSysMem;
  UINT SysMemPitch;
  UINT SysMemSlicePitch;
};

/* 259 */
struct ID3D11Buffer
{
  struct ID3D11BufferVtbl *lpVtbl;
};

/* 264 */
struct D3D11_TEXTURE1D_DESC
{
  UINT Width;
  UINT MipLevels;
  UINT ArraySize;
  DXGI_FORMAT Format;
  D3D11_USAGE Usage;
  UINT BindFlags;
  UINT CPUAccessFlags;
  UINT MiscFlags;
};

/* 265 */
struct ID3D11Texture1D
{
  struct ID3D11Texture1DVtbl *lpVtbl;
};

/* 267 */
struct ID3D11Texture2D
{
  struct ID3D11Texture2DVtbl *lpVtbl;
};

/* 269 */
struct D3D11_TEXTURE3D_DESC
{
  UINT Width;
  UINT Height;
  UINT Depth;
  UINT MipLevels;
  DXGI_FORMAT Format;
  D3D11_USAGE Usage;
  UINT BindFlags;
  UINT CPUAccessFlags;
  UINT MiscFlags;
};

/* 270 */
struct ID3D11Texture3D
{
  struct ID3D11Texture3DVtbl *lpVtbl;
};

/* 272 */
struct ID3D11Resource
{
  struct ID3D11ResourceVtbl *lpVtbl;
};

/* 274 */
struct ID3D11ShaderResourceView
{
  struct ID3D11ShaderResourceViewVtbl *lpVtbl;
};

/* 277 */
enum D3D11_UAV_DIMENSION
{
  D3D11_UAV_DIMENSION_UNKNOWN = 0x0,
  D3D11_UAV_DIMENSION_BUFFER = 0x1,
  D3D11_UAV_DIMENSION_TEXTURE1D = 0x2,
  D3D11_UAV_DIMENSION_TEXTURE1DARRAY = 0x3,
  D3D11_UAV_DIMENSION_TEXTURE2D = 0x4,
  D3D11_UAV_DIMENSION_TEXTURE2DARRAY = 0x5,
  D3D11_UAV_DIMENSION_TEXTURE3D = 0x8,
};

/* 279 */
struct D3D11_BUFFER_UAV
{
  UINT FirstElement;
  UINT NumElements;
  UINT Flags;
};

/* 280 */
struct D3D11_TEX1D_UAV
{
  UINT MipSlice;
};

/* 281 */
struct D3D11_TEX1D_ARRAY_UAV
{
  UINT MipSlice;
  UINT FirstArraySlice;
  UINT ArraySize;
};

/* 282 */
struct D3D11_TEX2D_UAV
{
  UINT MipSlice;
};

/* 283 */
struct D3D11_TEX2D_ARRAY_UAV
{
  UINT MipSlice;
  UINT FirstArraySlice;
  UINT ArraySize;
};

/* 284 */
struct D3D11_TEX3D_UAV
{
  UINT MipSlice;
  UINT FirstWSlice;
  UINT WSize;
};

/* 278 */
union D3D11_UNORDERED_ACCESS_VIEW_DESC::$11FBB28DCA371DE929B44BAF9661EF48
{
  D3D11_BUFFER_UAV Buffer;
  D3D11_TEX1D_UAV Texture1D;
  D3D11_TEX1D_ARRAY_UAV Texture1DArray;
  D3D11_TEX2D_UAV Texture2D;
  D3D11_TEX2D_ARRAY_UAV Texture2DArray;
  D3D11_TEX3D_UAV Texture3D;
};

/* 276 */
struct D3D11_UNORDERED_ACCESS_VIEW_DESC
{
  DXGI_FORMAT Format;
  D3D11_UAV_DIMENSION ViewDimension;
  union
  {
    D3D11_BUFFER_UAV Buffer;
    D3D11_TEX1D_UAV Texture1D;
    D3D11_TEX1D_ARRAY_UAV Texture1DArray;
    D3D11_TEX2D_UAV Texture2D;
    D3D11_TEX2D_ARRAY_UAV Texture2DArray;
    D3D11_TEX3D_UAV Texture3D;
  };
};

/* 285 */
struct ID3D11UnorderedAccessView
{
  struct ID3D11UnorderedAccessViewVtbl *lpVtbl;
};

/* 287 */
struct ID3D11RenderTargetView
{
  struct ID3D11RenderTargetViewVtbl *lpVtbl;
};

/* 290 */
enum D3D11_DSV_DIMENSION
{
  D3D11_DSV_DIMENSION_UNKNOWN = 0x0,
  D3D11_DSV_DIMENSION_TEXTURE1D = 0x1,
  D3D11_DSV_DIMENSION_TEXTURE1DARRAY = 0x2,
  D3D11_DSV_DIMENSION_TEXTURE2D = 0x3,
  D3D11_DSV_DIMENSION_TEXTURE2DARRAY = 0x4,
  D3D11_DSV_DIMENSION_TEXTURE2DMS = 0x5,
  D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY = 0x6,
};

/* 292 */
struct D3D11_TEX1D_DSV
{
  UINT MipSlice;
};

/* 293 */
struct D3D11_TEX1D_ARRAY_DSV
{
  UINT MipSlice;
  UINT FirstArraySlice;
  UINT ArraySize;
};

/* 294 */
struct D3D11_TEX2D_DSV
{
  UINT MipSlice;
};

/* 295 */
struct D3D11_TEX2D_ARRAY_DSV
{
  UINT MipSlice;
  UINT FirstArraySlice;
  UINT ArraySize;
};

/* 296 */
struct D3D11_TEX2DMS_DSV
{
  UINT UnusedField_NothingToDefine;
};

/* 297 */
struct D3D11_TEX2DMS_ARRAY_DSV
{
  UINT FirstArraySlice;
  UINT ArraySize;
};

/* 291 */
union D3D11_DEPTH_STENCIL_VIEW_DESC::$739C91EBC9F294EBF282B4AF07A94D76
{
  D3D11_TEX1D_DSV Texture1D;
  D3D11_TEX1D_ARRAY_DSV Texture1DArray;
  D3D11_TEX2D_DSV Texture2D;
  D3D11_TEX2D_ARRAY_DSV Texture2DArray;
  D3D11_TEX2DMS_DSV Texture2DMS;
  D3D11_TEX2DMS_ARRAY_DSV Texture2DMSArray;
};

/* 289 */
struct D3D11_DEPTH_STENCIL_VIEW_DESC
{
  DXGI_FORMAT Format;
  D3D11_DSV_DIMENSION ViewDimension;
  UINT Flags;
  union
  {
    D3D11_TEX1D_DSV Texture1D;
    D3D11_TEX1D_ARRAY_DSV Texture1DArray;
    D3D11_TEX2D_DSV Texture2D;
    D3D11_TEX2D_ARRAY_DSV Texture2DArray;
    D3D11_TEX2DMS_DSV Texture2DMS;
    D3D11_TEX2DMS_ARRAY_DSV Texture2DMSArray;
  };
};

/* 298 */
struct ID3D11DepthStencilView
{
  struct ID3D11DepthStencilViewVtbl *lpVtbl;
};

/* 300 */
struct ID3D11InputLayout
{
  struct ID3D11InputLayoutVtbl *lpVtbl;
};

/* 302 */
struct ID3D11ClassLinkage
{
  struct ID3D11ClassLinkageVtbl *lpVtbl;
};

/* 307 */
struct ID3D11VertexShader
{
  struct ID3D11VertexShaderVtbl *lpVtbl;
};

/* 309 */
struct ID3D11GeometryShader
{
  struct ID3D11GeometryShaderVtbl *lpVtbl;
};

/* 47 */
typedef unsigned __int8 BYTE;

/* 311 */
struct D3D11_SO_DECLARATION_ENTRY
{
  UINT Stream;
  LPCSTR SemanticName;
  UINT SemanticIndex;
  BYTE StartComponent;
  BYTE ComponentCount;
  BYTE OutputSlot;
};

/* 312 */
struct ID3D11PixelShader
{
  struct ID3D11PixelShaderVtbl *lpVtbl;
};

/* 314 */
struct ID3D11HullShader
{
  struct ID3D11HullShaderVtbl *lpVtbl;
};

/* 316 */
struct ID3D11DomainShader
{
  struct ID3D11DomainShaderVtbl *lpVtbl;
};

/* 318 */
struct ID3D11ComputeShader
{
  struct ID3D11ComputeShaderVtbl *lpVtbl;
};

/* 320 */
struct ID3D11DepthStencilState
{
  struct ID3D11DepthStencilStateVtbl *lpVtbl;
};

/* 322 */
struct ID3D11RasterizerState
{
  struct ID3D11RasterizerStateVtbl *lpVtbl;
};

/* 324 */
struct ID3D11SamplerState
{
  struct ID3D11SamplerStateVtbl *lpVtbl;
};

/* 327 */
enum D3D11_QUERY
{
  D3D11_QUERY_EVENT = 0x0,
  D3D11_QUERY_OCCLUSION = 0x1,
  D3D11_QUERY_TIMESTAMP = 0x2,
  D3D11_QUERY_TIMESTAMP_DISJOINT = 0x3,
  D3D11_QUERY_PIPELINE_STATISTICS = 0x4,
  D3D11_QUERY_OCCLUSION_PREDICATE = 0x5,
  D3D11_QUERY_SO_STATISTICS = 0x6,
  D3D11_QUERY_SO_OVERFLOW_PREDICATE = 0x7,
  D3D11_QUERY_SO_STATISTICS_STREAM0 = 0x8,
  D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM0 = 0x9,
  D3D11_QUERY_SO_STATISTICS_STREAM1 = 0xA,
  D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM1 = 0xB,
  D3D11_QUERY_SO_STATISTICS_STREAM2 = 0xC,
  D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM2 = 0xD,
  D3D11_QUERY_SO_STATISTICS_STREAM3 = 0xE,
  D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM3 = 0xF,
};

/* 326 */
struct D3D11_QUERY_DESC
{
  D3D11_QUERY Query;
  UINT MiscFlags;
};

/* 328 */
struct ID3D11Query
{
  struct ID3D11QueryVtbl *lpVtbl;
};

/* 330 */
struct ID3D11Predicate
{
  struct ID3D11PredicateVtbl *lpVtbl;
};

/* 333 */
enum __bitmask D3D11_COUNTER
{
  D3D11_COUNTER_DEVICE_DEPENDENT_0 = 0x40000000,
};

/* 332 */
struct D3D11_COUNTER_DESC
{
  D3D11_COUNTER Counter;
  UINT MiscFlags;
};

/* 334 */
struct ID3D11Counter
{
  struct ID3D11CounterVtbl *lpVtbl;
};

/* 336 */
struct ID3D11DeviceContext
{
  struct ID3D11DeviceContextVtbl *lpVtbl;
};

/* 348 */
struct D3D11_COUNTER_INFO
{
  D3D11_COUNTER LastDeviceDependentCounter;
  UINT NumSimultaneousCounters;
  UINT8 NumDetectableParallelUnits;
};

/* 263 */
enum D3D11_RESOURCE_DIMENSION
{
  D3D11_RESOURCE_DIMENSION_UNKNOWN = 0x0,
  D3D11_RESOURCE_DIMENSION_BUFFER = 0x1,
  D3D11_RESOURCE_DIMENSION_TEXTURE1D = 0x2,
  D3D11_RESOURCE_DIMENSION_TEXTURE2D = 0x3,
  D3D11_RESOURCE_DIMENSION_TEXTURE3D = 0x4,
};

/* 260 */
struct ID3D11BufferVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11Buffer *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11Buffer *This);
  ULONG (__stdcall *Release)(ID3D11Buffer *This);
  void (__stdcall *GetDevice)(ID3D11Buffer *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11Buffer *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11Buffer *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11Buffer *This, const GUID *const guid, const IUnknown *pData);
  void (__stdcall *GetType)(ID3D11Buffer *This, D3D11_RESOURCE_DIMENSION *pResourceDimension);
  void (__stdcall *SetEvictionPriority)(ID3D11Buffer *This, UINT EvictionPriority);
  UINT (__stdcall *GetEvictionPriority)(ID3D11Buffer *This);
  void (__stdcall *GetDesc)(ID3D11Buffer *This, D3D11_BUFFER_DESC *pDesc);
};

/* 266 */
struct ID3D11Texture1DVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11Texture1D *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11Texture1D *This);
  ULONG (__stdcall *Release)(ID3D11Texture1D *This);
  void (__stdcall *GetDevice)(ID3D11Texture1D *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11Texture1D *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11Texture1D *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11Texture1D *This, const GUID *const guid, const IUnknown *pData);
  void (__stdcall *GetType)(ID3D11Texture1D *This, D3D11_RESOURCE_DIMENSION *pResourceDimension);
  void (__stdcall *SetEvictionPriority)(ID3D11Texture1D *This, UINT EvictionPriority);
  UINT (__stdcall *GetEvictionPriority)(ID3D11Texture1D *This);
  void (__stdcall *GetDesc)(ID3D11Texture1D *This, D3D11_TEXTURE1D_DESC *pDesc);
};

/* 268 */
struct ID3D11Texture2DVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11Texture2D *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11Texture2D *This);
  ULONG (__stdcall *Release)(ID3D11Texture2D *This);
  void (__stdcall *GetDevice)(ID3D11Texture2D *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11Texture2D *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11Texture2D *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11Texture2D *This, const GUID *const guid, const IUnknown *pData);
  void (__stdcall *GetType)(ID3D11Texture2D *This, D3D11_RESOURCE_DIMENSION *pResourceDimension);
  void (__stdcall *SetEvictionPriority)(ID3D11Texture2D *This, UINT EvictionPriority);
  UINT (__stdcall *GetEvictionPriority)(ID3D11Texture2D *This);
  void (__stdcall *GetDesc)(ID3D11Texture2D *This, D3D11_TEXTURE2D_DESC *pDesc);
};

/* 271 */
struct ID3D11Texture3DVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11Texture3D *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11Texture3D *This);
  ULONG (__stdcall *Release)(ID3D11Texture3D *This);
  void (__stdcall *GetDevice)(ID3D11Texture3D *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11Texture3D *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11Texture3D *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11Texture3D *This, const GUID *const guid, const IUnknown *pData);
  void (__stdcall *GetType)(ID3D11Texture3D *This, D3D11_RESOURCE_DIMENSION *pResourceDimension);
  void (__stdcall *SetEvictionPriority)(ID3D11Texture3D *This, UINT EvictionPriority);
  UINT (__stdcall *GetEvictionPriority)(ID3D11Texture3D *This);
  void (__stdcall *GetDesc)(ID3D11Texture3D *This, D3D11_TEXTURE3D_DESC *pDesc);
};

/* 273 */
struct ID3D11ResourceVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11Resource *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11Resource *This);
  ULONG (__stdcall *Release)(ID3D11Resource *This);
  void (__stdcall *GetDevice)(ID3D11Resource *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11Resource *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11Resource *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11Resource *This, const GUID *const guid, const IUnknown *pData);
  void (__stdcall *GetType)(ID3D11Resource *This, D3D11_RESOURCE_DIMENSION *pResourceDimension);
  void (__stdcall *SetEvictionPriority)(ID3D11Resource *This, UINT EvictionPriority);
  UINT (__stdcall *GetEvictionPriority)(ID3D11Resource *This);
};

/* 275 */
struct ID3D11ShaderResourceViewVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11ShaderResourceView *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11ShaderResourceView *This);
  ULONG (__stdcall *Release)(ID3D11ShaderResourceView *This);
  void (__stdcall *GetDevice)(ID3D11ShaderResourceView *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11ShaderResourceView *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11ShaderResourceView *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11ShaderResourceView *This, const GUID *const guid, const IUnknown *pData);
  void (__stdcall *GetResource)(ID3D11ShaderResourceView *This, ID3D11Resource **ppResource);
  void (__stdcall *GetDesc)(ID3D11ShaderResourceView *This, D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc);
};

/* 286 */
struct ID3D11UnorderedAccessViewVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11UnorderedAccessView *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11UnorderedAccessView *This);
  ULONG (__stdcall *Release)(ID3D11UnorderedAccessView *This);
  void (__stdcall *GetDevice)(ID3D11UnorderedAccessView *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11UnorderedAccessView *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11UnorderedAccessView *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11UnorderedAccessView *This, const GUID *const guid, const IUnknown *pData);
  void (__stdcall *GetResource)(ID3D11UnorderedAccessView *This, ID3D11Resource **ppResource);
  void (__stdcall *GetDesc)(ID3D11UnorderedAccessView *This, D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc);
};

/* 288 */
struct ID3D11RenderTargetViewVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11RenderTargetView *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11RenderTargetView *This);
  ULONG (__stdcall *Release)(ID3D11RenderTargetView *This);
  void (__stdcall *GetDevice)(ID3D11RenderTargetView *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11RenderTargetView *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11RenderTargetView *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11RenderTargetView *This, const GUID *const guid, const IUnknown *pData);
  void (__stdcall *GetResource)(ID3D11RenderTargetView *This, ID3D11Resource **ppResource);
  void (__stdcall *GetDesc)(ID3D11RenderTargetView *This, D3D11_RENDER_TARGET_VIEW_DESC *pDesc);
};

/* 299 */
struct ID3D11DepthStencilViewVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11DepthStencilView *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11DepthStencilView *This);
  ULONG (__stdcall *Release)(ID3D11DepthStencilView *This);
  void (__stdcall *GetDevice)(ID3D11DepthStencilView *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11DepthStencilView *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11DepthStencilView *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11DepthStencilView *This, const GUID *const guid, const IUnknown *pData);
  void (__stdcall *GetResource)(ID3D11DepthStencilView *This, ID3D11Resource **ppResource);
  void (__stdcall *GetDesc)(ID3D11DepthStencilView *This, D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc);
};

/* 301 */
struct ID3D11InputLayoutVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11InputLayout *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11InputLayout *This);
  ULONG (__stdcall *Release)(ID3D11InputLayout *This);
  void (__stdcall *GetDevice)(ID3D11InputLayout *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11InputLayout *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11InputLayout *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11InputLayout *This, const GUID *const guid, const IUnknown *pData);
};

/* 303 */
struct ID3D11ClassLinkageVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11ClassLinkage *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11ClassLinkage *This);
  ULONG (__stdcall *Release)(ID3D11ClassLinkage *This);
  void (__stdcall *GetDevice)(ID3D11ClassLinkage *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11ClassLinkage *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11ClassLinkage *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11ClassLinkage *This, const GUID *const guid, const IUnknown *pData);
  HRESULT (__stdcall *GetClassInstance)(ID3D11ClassLinkage *This, LPCSTR pClassInstanceName, UINT InstanceIndex, ID3D11ClassInstance **ppInstance);
  HRESULT (__stdcall *CreateClassInstance)(ID3D11ClassLinkage *This, LPCSTR pClassTypeName, UINT ConstantBufferOffset, UINT ConstantVectorOffset, UINT TextureOffset, UINT SamplerOffset, ID3D11ClassInstance **ppInstance);
};

/* 308 */
struct ID3D11VertexShaderVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11VertexShader *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11VertexShader *This);
  ULONG (__stdcall *Release)(ID3D11VertexShader *This);
  void (__stdcall *GetDevice)(ID3D11VertexShader *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11VertexShader *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11VertexShader *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11VertexShader *This, const GUID *const guid, const IUnknown *pData);
};

/* 310 */
struct ID3D11GeometryShaderVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11GeometryShader *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11GeometryShader *This);
  ULONG (__stdcall *Release)(ID3D11GeometryShader *This);
  void (__stdcall *GetDevice)(ID3D11GeometryShader *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11GeometryShader *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11GeometryShader *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11GeometryShader *This, const GUID *const guid, const IUnknown *pData);
};

/* 313 */
struct ID3D11PixelShaderVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11PixelShader *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11PixelShader *This);
  ULONG (__stdcall *Release)(ID3D11PixelShader *This);
  void (__stdcall *GetDevice)(ID3D11PixelShader *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11PixelShader *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11PixelShader *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11PixelShader *This, const GUID *const guid, const IUnknown *pData);
};

/* 315 */
struct ID3D11HullShaderVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11HullShader *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11HullShader *This);
  ULONG (__stdcall *Release)(ID3D11HullShader *This);
  void (__stdcall *GetDevice)(ID3D11HullShader *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11HullShader *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11HullShader *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11HullShader *This, const GUID *const guid, const IUnknown *pData);
};

/* 317 */
struct ID3D11DomainShaderVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11DomainShader *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11DomainShader *This);
  ULONG (__stdcall *Release)(ID3D11DomainShader *This);
  void (__stdcall *GetDevice)(ID3D11DomainShader *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11DomainShader *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11DomainShader *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11DomainShader *This, const GUID *const guid, const IUnknown *pData);
};

/* 319 */
struct ID3D11ComputeShaderVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11ComputeShader *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11ComputeShader *This);
  ULONG (__stdcall *Release)(ID3D11ComputeShader *This);
  void (__stdcall *GetDevice)(ID3D11ComputeShader *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11ComputeShader *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11ComputeShader *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11ComputeShader *This, const GUID *const guid, const IUnknown *pData);
};

/* 321 */
struct ID3D11DepthStencilStateVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11DepthStencilState *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11DepthStencilState *This);
  ULONG (__stdcall *Release)(ID3D11DepthStencilState *This);
  void (__stdcall *GetDevice)(ID3D11DepthStencilState *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11DepthStencilState *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11DepthStencilState *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11DepthStencilState *This, const GUID *const guid, const IUnknown *pData);
  void (__stdcall *GetDesc)(ID3D11DepthStencilState *This, D3D11_DEPTH_STENCIL_DESC *pDesc);
};

/* 323 */
struct ID3D11RasterizerStateVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11RasterizerState *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11RasterizerState *This);
  ULONG (__stdcall *Release)(ID3D11RasterizerState *This);
  void (__stdcall *GetDevice)(ID3D11RasterizerState *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11RasterizerState *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11RasterizerState *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11RasterizerState *This, const GUID *const guid, const IUnknown *pData);
  void (__stdcall *GetDesc)(ID3D11RasterizerState *This, D3D11_RASTERIZER_DESC *pDesc);
};

/* 325 */
struct ID3D11SamplerStateVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11SamplerState *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11SamplerState *This);
  ULONG (__stdcall *Release)(ID3D11SamplerState *This);
  void (__stdcall *GetDevice)(ID3D11SamplerState *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11SamplerState *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11SamplerState *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11SamplerState *This, const GUID *const guid, const IUnknown *pData);
  void (__stdcall *GetDesc)(ID3D11SamplerState *This, D3D11_SAMPLER_DESC *pDesc);
};

/* 329 */
struct ID3D11QueryVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11Query *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11Query *This);
  ULONG (__stdcall *Release)(ID3D11Query *This);
  void (__stdcall *GetDevice)(ID3D11Query *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11Query *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11Query *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11Query *This, const GUID *const guid, const IUnknown *pData);
  UINT (__stdcall *GetDataSize)(ID3D11Query *This);
  void (__stdcall *GetDesc)(ID3D11Query *This, D3D11_QUERY_DESC *pDesc);
};

/* 331 */
struct ID3D11PredicateVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11Predicate *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11Predicate *This);
  ULONG (__stdcall *Release)(ID3D11Predicate *This);
  void (__stdcall *GetDevice)(ID3D11Predicate *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11Predicate *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11Predicate *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11Predicate *This, const GUID *const guid, const IUnknown *pData);
  UINT (__stdcall *GetDataSize)(ID3D11Predicate *This);
  void (__stdcall *GetDesc)(ID3D11Predicate *This, D3D11_QUERY_DESC *pDesc);
};

/* 335 */
struct ID3D11CounterVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11Counter *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11Counter *This);
  ULONG (__stdcall *Release)(ID3D11Counter *This);
  void (__stdcall *GetDevice)(ID3D11Counter *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11Counter *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11Counter *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11Counter *This, const GUID *const guid, const IUnknown *pData);
  UINT (__stdcall *GetDataSize)(ID3D11Counter *This);
  void (__stdcall *GetDesc)(ID3D11Counter *This, D3D11_COUNTER_DESC *pDesc);
};

/* 338 */
enum D3D11_MAP
{
  D3D11_MAP_READ = 0x1,
  D3D11_MAP_WRITE = 0x2,
  D3D11_MAP_READ_WRITE = 0x3,
  D3D11_MAP_WRITE_DISCARD = 0x4,
  D3D11_MAP_WRITE_NO_OVERWRITE = 0x5,
};

/* 340 */
enum D3D_PRIMITIVE_TOPOLOGY
{
  D3D_PRIMITIVE_TOPOLOGY_UNDEFINED = 0x0,
  D3D_PRIMITIVE_TOPOLOGY_POINTLIST = 0x1,
  D3D_PRIMITIVE_TOPOLOGY_LINELIST = 0x2,
  D3D_PRIMITIVE_TOPOLOGY_LINESTRIP = 0x3,
  D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST = 0x4,
  D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP = 0x5,
  D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ = 0xA,
  D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ = 0xB,
  D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ = 0xC,
  D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ = 0xD,
  D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST = 0x21,
  D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST = 0x22,
  D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST = 0x23,
  D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST = 0x24,
  D3D_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST = 0x25,
  D3D_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST = 0x26,
  D3D_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST = 0x27,
  D3D_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST = 0x28,
  D3D_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST = 0x29,
  D3D_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST = 0x2A,
  D3D_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST = 0x2B,
  D3D_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST = 0x2C,
  D3D_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST = 0x2D,
  D3D_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST = 0x2E,
  D3D_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST = 0x2F,
  D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST = 0x30,
  D3D_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST = 0x31,
  D3D_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST = 0x32,
  D3D_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST = 0x33,
  D3D_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST = 0x34,
  D3D_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST = 0x35,
  D3D_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST = 0x36,
  D3D_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST = 0x37,
  D3D_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST = 0x38,
  D3D_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST = 0x39,
  D3D_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST = 0x3A,
  D3D_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST = 0x3B,
  D3D_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST = 0x3C,
  D3D_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST = 0x3D,
  D3D_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST = 0x3E,
  D3D_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST = 0x3F,
  D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST = 0x40,
  D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED = 0x0,
  D3D10_PRIMITIVE_TOPOLOGY_POINTLIST = 0x1,
  D3D10_PRIMITIVE_TOPOLOGY_LINELIST = 0x2,
  D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP = 0x3,
  D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST = 0x4,
  D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP = 0x5,
  D3D10_PRIMITIVE_TOPOLOGY_LINELIST_ADJ = 0xA,
  D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ = 0xB,
  D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ = 0xC,
  D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ = 0xD,
  D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED = 0x0,
  D3D11_PRIMITIVE_TOPOLOGY_POINTLIST = 0x1,
  D3D11_PRIMITIVE_TOPOLOGY_LINELIST = 0x2,
  D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP = 0x3,
  D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST = 0x4,
  D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP = 0x5,
  D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ = 0xA,
  D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ = 0xB,
  D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ = 0xC,
  D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ = 0xD,
  D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST = 0x21,
  D3D11_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST = 0x22,
  D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST = 0x23,
  D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST = 0x24,
  D3D11_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST = 0x25,
  D3D11_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST = 0x26,
  D3D11_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST = 0x27,
  D3D11_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST = 0x28,
  D3D11_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST = 0x29,
  D3D11_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST = 0x2A,
  D3D11_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST = 0x2B,
  D3D11_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST = 0x2C,
  D3D11_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST = 0x2D,
  D3D11_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST = 0x2E,
  D3D11_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST = 0x2F,
  D3D11_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST = 0x30,
  D3D11_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST = 0x31,
  D3D11_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST = 0x32,
  D3D11_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST = 0x33,
  D3D11_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST = 0x34,
  D3D11_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST = 0x35,
  D3D11_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST = 0x36,
  D3D11_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST = 0x37,
  D3D11_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST = 0x38,
  D3D11_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST = 0x39,
  D3D11_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST = 0x3A,
  D3D11_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST = 0x3B,
  D3D11_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST = 0x3C,
  D3D11_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST = 0x3D,
  D3D11_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST = 0x3E,
  D3D11_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST = 0x3F,
  D3D11_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST = 0x40,
};

/* 339 */
typedef D3D_PRIMITIVE_TOPOLOGY D3D11_PRIMITIVE_TOPOLOGY;

/* 34 */
typedef struct tagRECT RECT;

/* 343 */
typedef RECT D3D11_RECT;

/* 347 */
enum D3D11_DEVICE_CONTEXT_TYPE
{
  D3D11_DEVICE_CONTEXT_IMMEDIATE = 0x0,
  D3D11_DEVICE_CONTEXT_DEFERRED = 0x1,
};

/* 337 */
struct ID3D11DeviceContextVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11DeviceContext *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11DeviceContext *This);
  ULONG (__stdcall *Release)(ID3D11DeviceContext *This);
  void (__stdcall *GetDevice)(ID3D11DeviceContext *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11DeviceContext *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11DeviceContext *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11DeviceContext *This, const GUID *const guid, const IUnknown *pData);
  void (__stdcall *VSSetConstantBuffers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers);
  void (__stdcall *PSSetShaderResources)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews);
  void (__stdcall *PSSetShader)(ID3D11DeviceContext *This, ID3D11PixelShader *pPixelShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances);
  void (__stdcall *PSSetSamplers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers);
  void (__stdcall *VSSetShader)(ID3D11DeviceContext *This, ID3D11VertexShader *pVertexShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances);
  void (__stdcall *DrawIndexed)(ID3D11DeviceContext *This, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);
  void (__stdcall *Draw)(ID3D11DeviceContext *This, UINT VertexCount, UINT StartVertexLocation);
  HRESULT (__stdcall *Map)(ID3D11DeviceContext *This, ID3D11Resource *pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE *pMappedResource);
  void (__stdcall *Unmap)(ID3D11DeviceContext *This, ID3D11Resource *pResource, UINT Subresource);
  void (__stdcall *PSSetConstantBuffers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers);
  void (__stdcall *IASetInputLayout)(ID3D11DeviceContext *This, ID3D11InputLayout *pInputLayout);
  void (__stdcall *IASetVertexBuffers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppVertexBuffers, const UINT *pStrides, const UINT *pOffsets);
  void (__stdcall *IASetIndexBuffer)(ID3D11DeviceContext *This, ID3D11Buffer *pIndexBuffer, DXGI_FORMAT Format, UINT Offset);
  void (__stdcall *DrawIndexedInstanced)(ID3D11DeviceContext *This, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);
  void (__stdcall *DrawInstanced)(ID3D11DeviceContext *This, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation);
  void (__stdcall *GSSetConstantBuffers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers);
  void (__stdcall *GSSetShader)(ID3D11DeviceContext *This, ID3D11GeometryShader *pShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances);
  void (__stdcall *IASetPrimitiveTopology)(ID3D11DeviceContext *This, D3D11_PRIMITIVE_TOPOLOGY Topology);
  void (__stdcall *VSSetShaderResources)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews);
  void (__stdcall *VSSetSamplers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers);
  void (__stdcall *Begin)(ID3D11DeviceContext *This, ID3D11Asynchronous *pAsync);
  void (__stdcall *End)(ID3D11DeviceContext *This, ID3D11Asynchronous *pAsync);
  HRESULT (__stdcall *GetData)(ID3D11DeviceContext *This, ID3D11Asynchronous *pAsync, void *pData, UINT DataSize, UINT GetDataFlags);
  void (__stdcall *SetPredication)(ID3D11DeviceContext *This, ID3D11Predicate *pPredicate, BOOL PredicateValue);
  void (__stdcall *GSSetShaderResources)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews);
  void (__stdcall *GSSetSamplers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers);
  void (__stdcall *OMSetRenderTargets)(ID3D11DeviceContext *This, UINT NumViews, ID3D11RenderTargetView *const *ppRenderTargetViews, ID3D11DepthStencilView *pDepthStencilView);
  void (__stdcall *OMSetRenderTargetsAndUnorderedAccessViews)(ID3D11DeviceContext *This, UINT NumRTVs, ID3D11RenderTargetView *const *ppRenderTargetViews, ID3D11DepthStencilView *pDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView *const *ppUnorderedAccessViews, const UINT *pUAVInitialCounts);
  void (__stdcall *OMSetBlendState)(ID3D11DeviceContext *This, ID3D11BlendState *pBlendState, const FLOAT BlendFactor[4], UINT SampleMask);
  void (__stdcall *OMSetDepthStencilState)(ID3D11DeviceContext *This, ID3D11DepthStencilState *pDepthStencilState, UINT StencilRef);
  void (__stdcall *SOSetTargets)(ID3D11DeviceContext *This, UINT NumBuffers, ID3D11Buffer *const *ppSOTargets, const UINT *pOffsets);
  void (__stdcall *DrawAuto)(ID3D11DeviceContext *This);
  void (__stdcall *DrawIndexedInstancedIndirect)(ID3D11DeviceContext *This, ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs);
  void (__stdcall *DrawInstancedIndirect)(ID3D11DeviceContext *This, ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs);
  void (__stdcall *Dispatch)(ID3D11DeviceContext *This, UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ);
  void (__stdcall *DispatchIndirect)(ID3D11DeviceContext *This, ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs);
  void (__stdcall *RSSetState)(ID3D11DeviceContext *This, ID3D11RasterizerState *pRasterizerState);
  void (__stdcall *RSSetViewports)(ID3D11DeviceContext *This, UINT NumViewports, const D3D11_VIEWPORT *pViewports);
  void (__stdcall *RSSetScissorRects)(ID3D11DeviceContext *This, UINT NumRects, const D3D11_RECT *pRects);
  void (__stdcall *CopySubresourceRegion)(ID3D11DeviceContext *This, ID3D11Resource *pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource *pSrcResource, UINT SrcSubresource, const D3D11_BOX *pSrcBox);
  void (__stdcall *CopyResource)(ID3D11DeviceContext *This, ID3D11Resource *pDstResource, ID3D11Resource *pSrcResource);
  void (__stdcall *UpdateSubresource)(ID3D11DeviceContext *This, ID3D11Resource *pDstResource, UINT DstSubresource, const D3D11_BOX *pDstBox, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch);
  void (__stdcall *CopyStructureCount)(ID3D11DeviceContext *This, ID3D11Buffer *pDstBuffer, UINT DstAlignedByteOffset, ID3D11UnorderedAccessView *pSrcView);
  void (__stdcall *ClearRenderTargetView)(ID3D11DeviceContext *This, ID3D11RenderTargetView *pRenderTargetView, const FLOAT ColorRGBA[4]);
  void (__stdcall *ClearUnorderedAccessViewUint)(ID3D11DeviceContext *This, ID3D11UnorderedAccessView *pUnorderedAccessView, const UINT Values[4]);
  void (__stdcall *ClearUnorderedAccessViewFloat)(ID3D11DeviceContext *This, ID3D11UnorderedAccessView *pUnorderedAccessView, const FLOAT Values[4]);
  void (__stdcall *ClearDepthStencilView)(ID3D11DeviceContext *This, ID3D11DepthStencilView *pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil);
  void (__stdcall *GenerateMips)(ID3D11DeviceContext *This, ID3D11ShaderResourceView *pShaderResourceView);
  void (__stdcall *SetResourceMinLOD)(ID3D11DeviceContext *This, ID3D11Resource *pResource, FLOAT MinLOD);
  FLOAT (__stdcall *GetResourceMinLOD)(ID3D11DeviceContext *This, ID3D11Resource *pResource);
  void (__stdcall *ResolveSubresource)(ID3D11DeviceContext *This, ID3D11Resource *pDstResource, UINT DstSubresource, ID3D11Resource *pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format);
  void (__stdcall *ExecuteCommandList)(ID3D11DeviceContext *This, ID3D11CommandList *pCommandList, BOOL RestoreContextState);
  void (__stdcall *HSSetShaderResources)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews);
  void (__stdcall *HSSetShader)(ID3D11DeviceContext *This, ID3D11HullShader *pHullShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances);
  void (__stdcall *HSSetSamplers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers);
  void (__stdcall *HSSetConstantBuffers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers);
  void (__stdcall *DSSetShaderResources)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews);
  void (__stdcall *DSSetShader)(ID3D11DeviceContext *This, ID3D11DomainShader *pDomainShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances);
  void (__stdcall *DSSetSamplers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers);
  void (__stdcall *DSSetConstantBuffers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers);
  void (__stdcall *CSSetShaderResources)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews);
  void (__stdcall *CSSetUnorderedAccessViews)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView *const *ppUnorderedAccessViews, const UINT *pUAVInitialCounts);
  void (__stdcall *CSSetShader)(ID3D11DeviceContext *This, ID3D11ComputeShader *pComputeShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances);
  void (__stdcall *CSSetSamplers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers);
  void (__stdcall *CSSetConstantBuffers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers);
  void (__stdcall *VSGetConstantBuffers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers);
  void (__stdcall *PSGetShaderResources)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews);
  void (__stdcall *PSGetShader)(ID3D11DeviceContext *This, ID3D11PixelShader **ppPixelShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances);
  void (__stdcall *PSGetSamplers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers);
  void (__stdcall *VSGetShader)(ID3D11DeviceContext *This, ID3D11VertexShader **ppVertexShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances);
  void (__stdcall *PSGetConstantBuffers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers);
  void (__stdcall *IAGetInputLayout)(ID3D11DeviceContext *This, ID3D11InputLayout **ppInputLayout);
  void (__stdcall *IAGetVertexBuffers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppVertexBuffers, UINT *pStrides, UINT *pOffsets);
  void (__stdcall *IAGetIndexBuffer)(ID3D11DeviceContext *This, ID3D11Buffer **pIndexBuffer, DXGI_FORMAT *Format, UINT *Offset);
  void (__stdcall *GSGetConstantBuffers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers);
  void (__stdcall *GSGetShader)(ID3D11DeviceContext *This, ID3D11GeometryShader **ppGeometryShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances);
  void (__stdcall *IAGetPrimitiveTopology)(ID3D11DeviceContext *This, D3D11_PRIMITIVE_TOPOLOGY *pTopology);
  void (__stdcall *VSGetShaderResources)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews);
  void (__stdcall *VSGetSamplers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers);
  void (__stdcall *GetPredication)(ID3D11DeviceContext *This, ID3D11Predicate **ppPredicate, BOOL *pPredicateValue);
  void (__stdcall *GSGetShaderResources)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews);
  void (__stdcall *GSGetSamplers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers);
  void (__stdcall *OMGetRenderTargets)(ID3D11DeviceContext *This, UINT NumViews, ID3D11RenderTargetView **ppRenderTargetViews, ID3D11DepthStencilView **ppDepthStencilView);
  void (__stdcall *OMGetRenderTargetsAndUnorderedAccessViews)(ID3D11DeviceContext *This, UINT NumRTVs, ID3D11RenderTargetView **ppRenderTargetViews, ID3D11DepthStencilView **ppDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView **ppUnorderedAccessViews);
  void (__stdcall *OMGetBlendState)(ID3D11DeviceContext *This, ID3D11BlendState **ppBlendState, FLOAT BlendFactor[4], UINT *pSampleMask);
  void (__stdcall *OMGetDepthStencilState)(ID3D11DeviceContext *This, ID3D11DepthStencilState **ppDepthStencilState, UINT *pStencilRef);
  void (__stdcall *SOGetTargets)(ID3D11DeviceContext *This, UINT NumBuffers, ID3D11Buffer **ppSOTargets);
  void (__stdcall *RSGetState)(ID3D11DeviceContext *This, ID3D11RasterizerState **ppRasterizerState);
  void (__stdcall *RSGetViewports)(ID3D11DeviceContext *This, UINT *pNumViewports, D3D11_VIEWPORT *pViewports);
  void (__stdcall *RSGetScissorRects)(ID3D11DeviceContext *This, UINT *pNumRects, D3D11_RECT *pRects);
  void (__stdcall *HSGetShaderResources)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews);
  void (__stdcall *HSGetShader)(ID3D11DeviceContext *This, ID3D11HullShader **ppHullShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances);
  void (__stdcall *HSGetSamplers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers);
  void (__stdcall *HSGetConstantBuffers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers);
  void (__stdcall *DSGetShaderResources)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews);
  void (__stdcall *DSGetShader)(ID3D11DeviceContext *This, ID3D11DomainShader **ppDomainShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances);
  void (__stdcall *DSGetSamplers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers);
  void (__stdcall *DSGetConstantBuffers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers);
  void (__stdcall *CSGetShaderResources)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews);
  void (__stdcall *CSGetUnorderedAccessViews)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView **ppUnorderedAccessViews);
  void (__stdcall *CSGetShader)(ID3D11DeviceContext *This, ID3D11ComputeShader **ppComputeShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances);
  void (__stdcall *CSGetSamplers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers);
  void (__stdcall *CSGetConstantBuffers)(ID3D11DeviceContext *This, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers);
  void (__stdcall *ClearState)(ID3D11DeviceContext *This);
  void (__stdcall *Flush)(ID3D11DeviceContext *This);
  D3D11_DEVICE_CONTEXT_TYPE (__stdcall *GetType)(ID3D11DeviceContext *This);
  UINT (__stdcall *GetContextFlags)(ID3D11DeviceContext *This);
  HRESULT (__stdcall *FinishCommandList)(ID3D11DeviceContext *This, BOOL RestoreDeferredContextState, ID3D11CommandList **ppCommandList);
};

/* 304 */
struct ID3D11ClassInstance
{
  struct ID3D11ClassInstanceVtbl *lpVtbl;
};

/* 341 */
struct ID3D11Asynchronous
{
  struct ID3D11AsynchronousVtbl *lpVtbl;
};

/* 36 */
typedef int LONG;

/* 35 */
struct tagRECT
{
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;
};

/* 344 */
struct D3D11_BOX
{
  UINT left;
  UINT top;
  UINT front;
  UINT right;
  UINT bottom;
  UINT back;
};

/* 345 */
struct ID3D11CommandList
{
  struct ID3D11CommandListVtbl *lpVtbl;
};

/* 305 */
struct ID3D11ClassInstanceVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11ClassInstance *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11ClassInstance *This);
  ULONG (__stdcall *Release)(ID3D11ClassInstance *This);
  void (__stdcall *GetDevice)(ID3D11ClassInstance *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11ClassInstance *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11ClassInstance *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11ClassInstance *This, const GUID *const guid, const IUnknown *pData);
  void (__stdcall *GetClassLinkage)(ID3D11ClassInstance *This, ID3D11ClassLinkage **ppLinkage);
  void (__stdcall *GetDesc)(ID3D11ClassInstance *This, D3D11_CLASS_INSTANCE_DESC *pDesc);
  void (__stdcall *GetInstanceName)(ID3D11ClassInstance *This, LPSTR pInstanceName, SIZE_T *pBufferLength);
  void (__stdcall *GetTypeName)(ID3D11ClassInstance *This, LPSTR pTypeName, SIZE_T *pBufferLength);
};

/* 342 */
struct ID3D11AsynchronousVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11Asynchronous *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11Asynchronous *This);
  ULONG (__stdcall *Release)(ID3D11Asynchronous *This);
  void (__stdcall *GetDevice)(ID3D11Asynchronous *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11Asynchronous *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11Asynchronous *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11Asynchronous *This, const GUID *const guid, const IUnknown *pData);
  UINT (__stdcall *GetDataSize)(ID3D11Asynchronous *This);
};

/* 346 */
struct ID3D11CommandListVtbl
{
  HRESULT (__stdcall *QueryInterface)(ID3D11CommandList *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(ID3D11CommandList *This);
  ULONG (__stdcall *Release)(ID3D11CommandList *This);
  void (__stdcall *GetDevice)(ID3D11CommandList *This, ID3D11Device **ppDevice);
  HRESULT (__stdcall *GetPrivateData)(ID3D11CommandList *This, const GUID *const guid, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *SetPrivateData)(ID3D11CommandList *This, const GUID *const guid, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(ID3D11CommandList *This, const GUID *const guid, const IUnknown *pData);
  UINT (__stdcall *GetContextFlags)(ID3D11CommandList *This);
};

/* 306 */
struct D3D11_CLASS_INSTANCE_DESC
{
  UINT InstanceId;
  UINT InstanceIndex;
  UINT TypeId;
  UINT ConstantBuffer;
  UINT BaseConstantBufferOffset;
  UINT BaseTexture;
  UINT BaseSampler;
  BOOL Created;
};

/* 351 */
struct IDXGIAdapter
{
  struct IDXGIAdapterVtbl *lpVtbl;
};

/* 52 */
typedef union _LARGE_INTEGER LARGE_INTEGER;

/* 352 */
struct IDXGIAdapterVtbl
{
  HRESULT (__stdcall *QueryInterface)(IDXGIAdapter *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(IDXGIAdapter *This);
  ULONG (__stdcall *Release)(IDXGIAdapter *This);
  HRESULT (__stdcall *SetPrivateData)(IDXGIAdapter *This, const GUID *const Name, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(IDXGIAdapter *This, const GUID *const Name, const IUnknown *pUnknown);
  HRESULT (__stdcall *GetPrivateData)(IDXGIAdapter *This, const GUID *const Name, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *GetParent)(IDXGIAdapter *This, const IID *const riid, void **ppParent);
  HRESULT (__stdcall *EnumOutputs)(IDXGIAdapter *This, UINT Output, IDXGIOutput **ppOutput);
  HRESULT (__stdcall *GetDesc)(IDXGIAdapter *This, DXGI_ADAPTER_DESC *pDesc);
  HRESULT (__stdcall *CheckInterfaceSupport)(IDXGIAdapter *This, const GUID *const InterfaceName, LARGE_INTEGER *pUMDVersion);
};

/* 353 */
struct IDXGIOutput
{
  struct IDXGIOutputVtbl *lpVtbl;
};

/* 17 */
typedef unsigned __int16 wchar_t;

/* 16 */
typedef wchar_t WCHAR;

/* 6 */
typedef unsigned int DWORD;

/* 369 */
struct _LUID
{
  DWORD LowPart;
  LONG HighPart;
};

/* 368 */
typedef struct _LUID LUID;

/* 367 */
struct DXGI_ADAPTER_DESC
{
  WCHAR Description[128];
  UINT VendorId;
  UINT DeviceId;
  UINT SubSysId;
  UINT Revision;
  SIZE_T DedicatedVideoMemory;
  SIZE_T DedicatedSystemMemory;
  SIZE_T SharedSystemMemory;
  LUID AdapterLuid;
};

/* 54 */
struct _LARGE_INTEGER::$837407842DC9087486FDFA5FEB63B74E
{
  DWORD LowPart;
  LONG HighPart;
};

/* 55 */
typedef __int64 LONGLONG;

/* 53 */
union _LARGE_INTEGER
{
  struct
  {
    DWORD LowPart;
    LONG HighPart;
  };
  struct
  {
    DWORD LowPart;
    LONG HighPart;
  } u;
  LONGLONG QuadPart;
};

/* 354 */
struct IDXGIOutputVtbl
{
  HRESULT (__stdcall *QueryInterface)(IDXGIOutput *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(IDXGIOutput *This);
  ULONG (__stdcall *Release)(IDXGIOutput *This);
  HRESULT (__stdcall *SetPrivateData)(IDXGIOutput *This, const GUID *const Name, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(IDXGIOutput *This, const GUID *const Name, const IUnknown *pUnknown);
  HRESULT (__stdcall *GetPrivateData)(IDXGIOutput *This, const GUID *const Name, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *GetParent)(IDXGIOutput *This, const IID *const riid, void **ppParent);
  HRESULT (__stdcall *GetDesc)(IDXGIOutput *This, DXGI_OUTPUT_DESC *pDesc);
  HRESULT (__stdcall *GetDisplayModeList)(IDXGIOutput *This, DXGI_FORMAT EnumFormat, UINT Flags, UINT *pNumModes, DXGI_MODE_DESC *pDesc);
  HRESULT (__stdcall *FindClosestMatchingMode)(IDXGIOutput *This, const DXGI_MODE_DESC *pModeToMatch, DXGI_MODE_DESC *pClosestMatch, IUnknown *pConcernedDevice);
  HRESULT (__stdcall *WaitForVBlank)(IDXGIOutput *This);
  HRESULT (__stdcall *TakeOwnership)(IDXGIOutput *This, IUnknown *pDevice, BOOL Exclusive);
  void (__stdcall *ReleaseOwnership)(IDXGIOutput *This);
  HRESULT (__stdcall *GetGammaControlCapabilities)(IDXGIOutput *This, DXGI_GAMMA_CONTROL_CAPABILITIES *pGammaCaps);
  HRESULT (__stdcall *SetGammaControl)(IDXGIOutput *This, const DXGI_GAMMA_CONTROL *pArray);
  HRESULT (__stdcall *GetGammaControl)(IDXGIOutput *This, DXGI_GAMMA_CONTROL *pArray);
  HRESULT (__stdcall *SetDisplaySurface)(IDXGIOutput *This, IDXGISurface *pScanoutSurface);
  HRESULT (__stdcall *GetDisplaySurfaceData)(IDXGIOutput *This, IDXGISurface *pDestination);
  HRESULT (__stdcall *GetFrameStatistics)(IDXGIOutput *This, DXGI_FRAME_STATISTICS *pStats);
};

/* 356 */
enum DXGI_MODE_ROTATION
{
  DXGI_MODE_ROTATION_UNSPECIFIED = 0x0,
  DXGI_MODE_ROTATION_IDENTITY = 0x1,
  DXGI_MODE_ROTATION_ROTATE90 = 0x2,
  DXGI_MODE_ROTATION_ROTATE180 = 0x3,
  DXGI_MODE_ROTATION_ROTATE270 = 0x4,
};

/* 357 */
typedef struct HMONITOR__ *HMONITOR;

/* 355 */
struct DXGI_OUTPUT_DESC
{
  WCHAR DeviceName[32];
  RECT DesktopCoordinates;
  BOOL AttachedToDesktop;
  DXGI_MODE_ROTATION Rotation;
  HMONITOR Monitor;
};

/* 22 */
struct DXGI_RATIONAL
{
  UINT Numerator;
  UINT Denominator;
};

/* 24 */
enum DXGI_MODE_SCANLINE_ORDER
{
  DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED = 0x0,
  DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE = 0x1,
  DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST = 0x2,
  DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST = 0x3,
};

/* 25 */
enum DXGI_MODE_SCALING
{
  DXGI_MODE_SCALING_UNSPECIFIED = 0x0,
  DXGI_MODE_SCALING_CENTERED = 0x1,
  DXGI_MODE_SCALING_STRETCHED = 0x2,
};

/* 20 */
struct DXGI_MODE_DESC
{
  UINT Width;
  UINT Height;
  DXGI_RATIONAL RefreshRate;
  DXGI_FORMAT Format;
  DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;
  DXGI_MODE_SCALING Scaling;
};

/* 359 */
struct DXGI_GAMMA_CONTROL_CAPABILITIES
{
  BOOL ScaleAndOffsetSupported;
  float MaxConvertedValue;
  float MinConvertedValue;
  UINT NumGammaControlPoints;
  float ControlPointPositions[1025];
};

/* 361 */
struct DXGI_RGB
{
  float Red;
  float Green;
  float Blue;
};

/* 360 */
struct DXGI_GAMMA_CONTROL
{
  DXGI_RGB Scale;
  DXGI_RGB Offset;
  DXGI_RGB GammaCurve[1025];
};

/* 362 */
struct IDXGISurface
{
  struct IDXGISurfaceVtbl *lpVtbl;
};

/* 366 */
struct DXGI_FRAME_STATISTICS
{
  UINT PresentCount;
  UINT PresentRefreshCount;
  UINT SyncRefreshCount;
  LARGE_INTEGER SyncQPCTime;
  LARGE_INTEGER SyncGPUTime;
};

/* 358 */
struct HMONITOR__
{
  int unused;
};

/* 363 */
struct IDXGISurfaceVtbl
{
  HRESULT (__stdcall *QueryInterface)(IDXGISurface *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(IDXGISurface *This);
  ULONG (__stdcall *Release)(IDXGISurface *This);
  HRESULT (__stdcall *SetPrivateData)(IDXGISurface *This, const GUID *const Name, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(IDXGISurface *This, const GUID *const Name, const IUnknown *pUnknown);
  HRESULT (__stdcall *GetPrivateData)(IDXGISurface *This, const GUID *const Name, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *GetParent)(IDXGISurface *This, const IID *const riid, void **ppParent);
  HRESULT (__stdcall *GetDevice)(IDXGISurface *This, const IID *const riid, void **ppDevice);
  HRESULT (__stdcall *GetDesc)(IDXGISurface *This, DXGI_SURFACE_DESC *pDesc);
  HRESULT (__stdcall *Map)(IDXGISurface *This, DXGI_MAPPED_RECT *pLockedRect, UINT MapFlags);
  HRESULT (__stdcall *Unmap)(IDXGISurface *This);
};

/* 364 */
struct DXGI_SURFACE_DESC
{
  UINT Width;
  UINT Height;
  DXGI_FORMAT Format;
  DXGI_SAMPLE_DESC SampleDesc;
};

/* 365 */
struct DXGI_MAPPED_RECT
{
  INT Pitch;
  BYTE *pBits;
};

/* 370 */
struct IDXGISwapChain
{
  struct IDXGISwapChainVtbl *lpVtbl;
};

/* 371 */
struct IDXGISwapChainVtbl
{
  HRESULT (__stdcall *QueryInterface)(IDXGISwapChain *This, const IID *const riid, void **ppvObject);
  ULONG (__stdcall *AddRef)(IDXGISwapChain *This);
  ULONG (__stdcall *Release)(IDXGISwapChain *This);
  HRESULT (__stdcall *SetPrivateData)(IDXGISwapChain *This, const GUID *const Name, UINT DataSize, const void *pData);
  HRESULT (__stdcall *SetPrivateDataInterface)(IDXGISwapChain *This, const GUID *const Name, const IUnknown *pUnknown);
  HRESULT (__stdcall *GetPrivateData)(IDXGISwapChain *This, const GUID *const Name, UINT *pDataSize, void *pData);
  HRESULT (__stdcall *GetParent)(IDXGISwapChain *This, const IID *const riid, void **ppParent);
  HRESULT (__stdcall *GetDevice)(IDXGISwapChain *This, const IID *const riid, void **ppDevice);
  HRESULT (__stdcall *Present)(IDXGISwapChain *This, UINT SyncInterval, UINT Flags);
  HRESULT (__stdcall *GetBuffer)(IDXGISwapChain *This, UINT Buffer, const IID *const riid, void **ppSurface);
  HRESULT (__stdcall *SetFullscreenState)(IDXGISwapChain *This, BOOL Fullscreen, IDXGIOutput *pTarget);
  HRESULT (__stdcall *GetFullscreenState)(IDXGISwapChain *This, BOOL *pFullscreen, IDXGIOutput **ppTarget);
  HRESULT (__stdcall *GetDesc)(IDXGISwapChain *This, DXGI_SWAP_CHAIN_DESC *pDesc);
  HRESULT (__stdcall *ResizeBuffers)(IDXGISwapChain *This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
  HRESULT (__stdcall *ResizeTarget)(IDXGISwapChain *This, const DXGI_MODE_DESC *pNewTargetParameters);
  HRESULT (__stdcall *GetContainingOutput)(IDXGISwapChain *This, IDXGIOutput **ppOutput);
  HRESULT (__stdcall *GetFrameStatistics)(IDXGISwapChain *This, DXGI_FRAME_STATISTICS *pStats);
  HRESULT (__stdcall *GetLastPresentCount)(IDXGISwapChain *This, UINT *pLastPresentCount);
};

/* 27 */
typedef UINT DXGI_USAGE;

/* 28 */
typedef struct HWND__ *HWND;

/* 31 */
enum DXGI_SWAP_EFFECT
{
  DXGI_SWAP_EFFECT_DISCARD = 0x0,
  DXGI_SWAP_EFFECT_SEQUENTIAL = 0x1,
  DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL = 0x3,
  DXGI_SWAP_EFFECT_FLIP_DISCARD = 0x4,
};

/* 19 */
struct DXGI_SWAP_CHAIN_DESC
{
  DXGI_MODE_DESC BufferDesc;
  DXGI_SAMPLE_DESC SampleDesc;
  DXGI_USAGE BufferUsage;
  UINT BufferCount;
  HWND OutputWindow;
  BOOL Windowed;
  DXGI_SWAP_EFFECT SwapEffect;
  UINT Flags;
};

/* 29 */
struct HWND__
{
  int unused;
};

