// gge_gl_backend.h - what the game's rendering layer calls instead of Direct3D (hand-written).
//
// tools/draft_fix.py turns every Direct3D interface call in the decompiled gge_* functions into a call of
// the function of the same name here ("ctx->lpVtbl->Map(ctx, ...)" becomes "gge_gl_Map(ctx, ...)"), keeping
// the argument lists. The "descriptions" passed in are the plain data blocks the game already fills in;
// objects handed back are this backend's own. Everything returns 0 for success, as the game expects.
#pragma once

int gge_gl_create_device( void *adapter, int driver, void *software, unsigned int flags, const void *levels, unsigned int num_levels,
                          unsigned int sdk, const void *swap_chain_desc, void **swap_chain, void **device, void *level, void **context );
unsigned int gge_gl_Release( void *object );

// device
int gge_gl_CreateBuffer( void *device, const void *desc, const void *initial, void **buffer );
int gge_gl_CreateTexture2D( void *device, const void *desc, const void *initial, void **texture );
int gge_gl_CreateShaderResourceView( void *device, void *resource, const void *desc, void **view );
int gge_gl_CreateRenderTargetView( void *device, void *resource, const void *desc, void **view );
int gge_gl_CreateDepthStencilView( void *device, void *resource, const void *desc, void **view );
int gge_gl_CreateDepthStencilState( void *device, const void *desc, void **state );
int gge_gl_CreateBlendState( void *device, const void *desc, void **state );
int gge_gl_CreateSamplerState( void *device, const void *desc, void **state );
int gge_gl_CreateRasterizerState( void *device, const void *desc, void **state );
int gge_gl_CreateInputLayout( void *device, const void *elements, unsigned int num_elements, const void *shader, unsigned int shader_size, void **layout );
int gge_gl_CreateVertexShader( void *device, const void *shader, unsigned int size, void *linkage, void **out );
int gge_gl_CreatePixelShader( void *device, const void *shader, unsigned int size, void *linkage, void **out );

// drawing context
int gge_gl_IASetInputLayout( void *context, void *layout );
int gge_gl_IASetVertexBuffers( void *context, unsigned int slot, unsigned int count, void *const *buffers, const unsigned int *strides, const unsigned int *offsets );
int gge_gl_IASetIndexBuffer( void *context, void *buffer, int format, unsigned int offset );
int gge_gl_IASetPrimitiveTopology( void *context, int topology );
int gge_gl_VSSetShader( void *context, void *shader, void *instances, unsigned int count );
int gge_gl_PSSetShader( void *context, void *shader, void *instances, unsigned int count );
int gge_gl_VSSetConstantBuffers( void *context, unsigned int slot, unsigned int count, void *const *buffers );
int gge_gl_PSSetConstantBuffers( void *context, unsigned int slot, unsigned int count, void *const *buffers );
int gge_gl_PSSetShaderResources( void *context, unsigned int slot, unsigned int count, void *const *views );
int gge_gl_PSSetSamplers( void *context, unsigned int slot, unsigned int count, void *const *samplers );
int gge_gl_OMSetBlendState( void *context, void *state, const float *factor, unsigned int mask );
int gge_gl_OMSetDepthStencilState( void *context, void *state, unsigned int reference );
int gge_gl_OMSetRenderTargets( void *context, unsigned int count, void *const *views, void *depth_view );
int gge_gl_RSSetState( void *context, void *state );
int gge_gl_RSSetViewports( void *context, unsigned int count, const void *viewports );
int gge_gl_ClearRenderTargetView( void *context, void *view, const float *color );
int gge_gl_ClearDepthStencilView( void *context, void *view, unsigned int flags, float depth, unsigned char stencil );
int gge_gl_Map( void *context, void *resource, unsigned int subresource, int type, unsigned int flags, void *mapped );
int gge_gl_Unmap( void *context, void *resource, unsigned int subresource );
int gge_gl_UpdateSubresource( void *context, void *resource, unsigned int subresource, const void *box, const void *data, unsigned int row_pitch, unsigned int depth_pitch );
int gge_gl_Draw( void *context, unsigned int count, unsigned int start );
int gge_gl_DrawIndexed( void *context, unsigned int count, unsigned int start, int base_vertex );

// swap chain
int gge_gl_GetBuffer( void *swap_chain, unsigned int index, const void *interface_id, void **surface );
int gge_gl_Present( void *swap_chain, unsigned int sync_interval, unsigned int flags );
