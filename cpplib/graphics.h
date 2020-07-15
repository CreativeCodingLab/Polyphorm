#pragma once
#include <D3D11.h>
#include <dxgi.h>
#include <DirectXTex.h>
//#include <DDSTextureLoader.h>
#include <string>

#include "platform.h"

// GraphicsContext wraps handles to D3D device and context
struct GraphicsContext
{
	ID3D11Device *device;
	ID3D11DeviceContext *context;
};

// We usually want a single GraphicsContext, this way
// it's created only in graphics.cpp and accessible from anywhere.
extern GraphicsContext *graphics_context;

////////////////////////////////////////////////////////////
/// D3D11 structures
////////////////////////////////////////////////////////////

struct SwapChain
{
	IDXGISwapChain *swap_chain;
};

// Render target can be used both as render target and shader resource view.
// Exception to this is RenderTarget obtained by calling get_render_target_window, which
// cannot be used as a shader resource view.
struct RenderTarget
{
	ID3D11RenderTargetView *rt_view;
	ID3D11ShaderResourceView *sr_view;
	ID3D11Texture2D *texture;
	uint32_t width;
	uint32_t height;
};

// Depth buffer can be used both as depth buffer (target) and shader resource view.
struct DepthBuffer
{
	ID3D11DepthStencilView *ds_view;
	ID3D11ShaderResourceView *sr_view;
	ID3D11Texture2D *texture;
	uint32_t width;
	uint32_t height;
};

struct Texture2D
{
	ID3D11Texture2D *texture;
	ID3D11ShaderResourceView *sr_view;
	ID3D11UnorderedAccessView *ua_view;
	uint32_t width;
	uint32_t height;
};

struct Texture3D
{
	ID3D11Texture3D *texture;
	ID3D11ShaderResourceView *sr_view;
	ID3D11UnorderedAccessView *ua_view;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
};

// Mesh references GPU-side memory with vertices and indices
struct Mesh
{
	ID3D11Buffer *vertex_buffer;
	ID3D11Buffer *index_buffer;
	uint32_t vertex_stride;
	uint32_t vertex_offset;
	uint32_t vertex_count;
	uint32_t index_count;
	DXGI_FORMAT index_format;
	D3D11_PRIMITIVE_TOPOLOGY topology;
};

// Vertex shader encapsulates both D3D vertex shader and input layou
struct VertexShader
{
	ID3D11VertexShader *vertex_shader;
	ID3D11InputLayout *input_layout;
};

struct GeometryShader
{
	ID3D11GeometryShader *geometry_shader;
};

struct ComputeShader
{
	ID3D11ComputeShader *compute_shader;
};

struct PixelShader
{
	ID3D11PixelShader *pixel_shader;
};

struct ConstantBuffer
{
	ID3D11Buffer *buffer;
	uint32_t size;
};

// TODO: Maybe unify with ConstantBuffer?
struct StructuredBuffer
{
	ID3D11Buffer *buffer;
	ID3D11UnorderedAccessView *ua_view;
	uint32_t size;
};

// VertexInputDesc represents a single input to a vertex shader.
// Needs semantic name and a format:
// Example: 
// 	float4 position: POSITION;
// in vertex shader input translates to VertexInputDesc with
// .semantic_name = "POSITION" and .format = DXGI_R32G32B32A32_FLOAT
//
const uint32_t MAX_SEMANTIC_NAME_LENGTH = 10;
struct VertexInputDesc
{
	char semantic_name[MAX_SEMANTIC_NAME_LENGTH];
	DXGI_FORMAT format;
};

// Compiled shader represents shader byte code, compiled from source code.
struct CompiledShader
{
	ID3DBlob *blob;
};

// Sample modes for the texture sampling.
enum SampleMode
{
	CLAMP = 0,
	WRAP,
	BORDER,
};

struct TextureSampler
{
	ID3D11SamplerState *sampler;
};

#undef OPAQUE
enum BlendType
{
	ALPHA = 0,
	OPAQUE = 1
};

enum RasterType
{
	SOLID = 0,
	WIREFRAME = 1
};

struct Viewport
{
	float x;
	float y;
	float width;
	float height;
};

//////////////////////////////////////////
/// Low level API
//////////////////////////////////////////

// `graphics` namespace contains functions that provide mid-level API on top of D3D11
namespace graphics
{
	// Initialize graphics context and blending states
	//
	// Args:
	//  - adapter_luid: LUID for the graphics adapter (GPU)
	bool init(LUID *adapter_luid = NULL);

	// Initialize swap chain for rendering to window
	bool init_swap_chain(Window *window);
	

	// Get RenderTarget to use for rendering directly to a window passed in to `init_swap_chain`
	RenderTarget get_render_target_window();

	// Get RenderTarget with specified width, height and format
	RenderTarget get_render_target(uint32_t width, uint32_t height, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);

	// Clear RenderTarget to a specified color
	void clear_render_target(RenderTarget *buffer, float r, float g, float b, float a);

	// Get Depth Buffer with specified width and height
	DepthBuffer get_depth_buffer(uint32_t width, uint32_t height);

	// Clear depth buffer to zeros
	void clear_depth_buffer(DepthBuffer *buffer);

	// Set only DepthBuffer, no RenderTarget
	void set_render_targets(DepthBuffer *buffer);

	// Set a single RenderTarget, no DepthBuffer
	void set_render_targets(RenderTarget *buffer);

	// Seta a single RenderTarget and DepthBuffer
	void set_render_targets(RenderTarget *buffer, DepthBuffer *depth_buffer);

	// Set viewport based based on RenderTarget
	void set_viewport(RenderTarget *buffer);

	// Set viewport based based on DepthBuffer
	void set_viewport(DepthBuffer *buffer);

	// Set viewportt
	void set_viewport(Viewport *viewport);

	// Set a single RenderTarget along with it's viewport
	void set_render_targets_viewport(RenderTarget *buffer);

	// Set a single RenderTarget and a single DepthBuffer
    void set_render_targets_viewport(RenderTarget *buffer, DepthBuffer *depth_buffer);

	// Set multiple RenderTargets and a single DepthBuffer
	void set_render_targets_viewport(RenderTarget *buffers, uint32_t buffer_count, DepthBuffer *depth_buffer);

	// Get Texture with specified width, height and format.
	// Args:
	//  - data: bitmap data
	//  - width: bitmap width
	//  - height: bitmap height
	//  - format: DXGI format
	//  - pixel_byte_count: number of bytes per pixel. Used to compute memory pitch.
	Texture2D get_texture2D(void *data, uint32_t width, uint32_t height, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, uint32_t pixel_byte_count = 4);

	// Set RenderTarget as a texture accessible from shaders
	void set_texture(RenderTarget *buffer, uint32_t slot);

	// Set DepthBuffer as a texture accessible from shaders
	void set_texture(DepthBuffer *buffer, uint32_t slot);

	// Set texture to accessible from shaders
	void set_texture(Texture2D *texture, uint32_t slot);

	// Set texture to be accessible from compute shaders
	void set_texture_compute(Texture2D *texture, uint32_t slot);
	void set_texture_sampled_compute(Texture2D *texture, uint32_t slot);

	// Clear specific compute texture slot.
	void unset_texture_compute(uint32_t slot);
	void unset_texture_sampled_compute(uint32_t slot);

	// Clear specific texture slot
	void unset_texture(uint32_t slot);

	// Get Texture3D with specified width, height and format.
	// Args:
	//  - data: bitmap data
	//  - width: bitmap width
	//  - height: bitmap height
	//  - depth: bitmap depth
	//  - format: DXGI format
	//  - pixel_byte_count: number of bytes per pixel. Used to compute memory pitch.
	Texture3D get_texture3D(void *data, uint32_t width, uint32_t height, uint32_t depth, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, uint32_t pixel_byte_count = 4);

	// Load/save texture to/from file
	Texture2D load_texture2D(std::string filename);
	Texture3D load_texture3D(std::string filename);
	void save_texture3D(Texture3D *texture, std::string filename);
	void save_texture2D(Texture2D *texture, std::string filename);
	void capture_current_frame();

	// Set texture to accessible from shaders
	void set_texture(Texture3D *texture, uint32_t slot);

	// Set texture to be accessible from compute shaders
	void set_texture_compute(Texture3D *texture, uint32_t slot);
	void set_texture_sampled_compute(Texture3D *texture, uint32_t slot);

	// Set blending state
	void set_blend_state(BlendType type);

	// Get current blending state
	BlendType get_blend_state();

	// Set rasterizer state
	void set_rasterizer_state(RasterType type);

	// Get current rasterizer state
	RasterType get_rasterizer_state();

	// Get TextureSampler with specific mode
	TextureSampler get_texture_sampler(SampleMode mode = CLAMP, D3D11_FILTER filter = D3D11_FILTER_MIN_MAG_MIP_POINT);

	// Set TextureSampler to specific slot
	void set_texture_sampler(TextureSampler *sampler, uint32_t slot);
	void set_texture_sampler_compute(TextureSampler *sampler, uint32_t slot);

	// Get Mesh based on mesh data
	Mesh get_mesh(void *vertices, uint32_t vertex_count, uint32_t vertex_stride, void *indices, uint32_t index_count,
				  uint32_t index_byte_size, D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Draw a Mesh
	void draw_mesh(Mesh *mesh);

	// Get ConstantBuffer with specific size
	ConstantBuffer get_constant_buffer(uint32_t size);

	// Get StructuredBuffer
	StructuredBuffer get_structured_buffer(int element_stride, int num_elements);

	// Download a structured buffer from GPU to preallocated CPU memory
	void capture_structured_buffer(StructuredBuffer *buffer, void *mapped_data, unsigned int num_elements, size_t element_size);

	// Update ConstantBuffer with data
	void update_constant_buffer(ConstantBuffer *buffer, void *data);

	// Set ConstantBuffer to a slot
	void set_constant_buffer(ConstantBuffer *buffer, uint32_t slot);

	// Update StructuredBuffer with data
	void update_structured_buffer(StructuredBuffer *buffer, void *data);

	// Set StructuredBuffer to a slot
	void set_structured_buffer(StructuredBuffer *buffer, uint32_t slot);

	// Compile a vertex shader from a source code
	CompiledShader compile_vertex_shader(void *source, uint32_t source_size);
	
	// Compile a pixel shader from a source code
	CompiledShader compile_pixel_shader(void *source, uint32_t source_size);
	
	// Compile a geometry shader from a source code
	CompiledShader compile_geometry_shader(void *source, uint32_t source_size);

	// Compile a compute shader from a source code
	CompiledShader compile_compute_shader(void *source, uint32_t source_size);

	// Get VertexShader from a CompiledShader and number of VertexInputDescs
	VertexShader get_vertex_shader(CompiledShader *compiled_shader, VertexInputDesc *vertex_input_descs, uint32_t vertex_input_count);

	// Get VertexShader from shader byte code and number of VertexInputDescs
	VertexShader get_vertex_shader(void *shader_byte_code, uint32_t shader_size, VertexInputDesc *vertex_input_descs, uint32_t vertex_input_count);

	// Bind VertexShader to a pipeline
	void set_vertex_shader(VertexShader *shader);

	// Get PixelShader from a CompiledShader
	PixelShader get_pixel_shader(CompiledShader *compiled_shader);

	// Get PixelShader from pixel shader byte code
	PixelShader get_pixel_shader(void *shader_byte_code, uint32_t shader_size);

	// Unbind pixel shader from a pipeline
	void set_pixel_shader();

	// Bind PixelShader to a pipeline
	void set_pixel_shader(PixelShader *shader);

	// Get GeometryShader from a CompiledShader
	GeometryShader get_geometry_shader(CompiledShader *compiled_shader);

	// Get GeometryShader from geometry shader byte code
	GeometryShader get_geometry_shader(void *shader_byte_code, uint32_t shader_size);

	// Unbind geometry shader from a pipeline
	void set_geometry_shader();

	// Bind GeoemtryShader to a pipeline
	void set_geometry_shader(GeometryShader *shader);

	// Get ComputeShader from a CompiledShader
	ComputeShader get_compute_shader(CompiledShader *compiled_shader);

	// Get ComputeShader from compute shader byte code
	ComputeShader get_compute_shader(void *shader_byte_code, uint32_t shader_size);

	// Unbind compute shader from a pipeline
	void set_compute_shader();

	// Bind ComputeShader to a pipeline
	void set_compute_shader(ComputeShader *shader);

	// Run compute shader
	void run_compute(int group_x, int group_y, int group_z);

	// Swap buffers between frames
	void swap_frames();

	// Show unreleased DX objects in Debug output
	void show_live_objects();

	// Functions that check whether specific `graphics` object is ready for use
	bool is_ready(Texture2D *texture);
	bool is_ready(Texture3D *texture);
	bool is_ready(RenderTarget *render_target);
	bool is_ready(DepthBuffer *depth_buffer);
	bool is_ready(Mesh *mesh);
	bool is_ready(ConstantBuffer *buffer);
	bool is_ready(TextureSampler *sampler);
	bool is_ready(VertexShader *shader);
	bool is_ready(PixelShader *shader);
	bool is_ready(ComputeShader *shader);
	bool is_ready(CompiledShader *shader);

	// Functions that release specific `graphics` objects. `release()` releases
	// d3d11 device and context and should be called as last.
	void release();
	void release(SwapChain *swap_chain);
	void release(RenderTarget *buffer);
	void release(DepthBuffer *buffer);
	void release(Texture2D *texture);
	void release(Texture3D *texture);
	void release(TextureSampler *sampler);
	void release(Mesh *mesh);
	void release(ConstantBuffer *buffer);
	void release(StructuredBuffer *buffer);
	void release(VertexShader *shader);
	void release(PixelShader *shader);
	void release(GeometryShader *shader);
	void release(ComputeShader *shader);
	void release(CompiledShader *shader);

	////////////////////////////////////////////////
	/// HIGHER LEVEL API
	////////////////////////////////////////////////

	// Obtain list of VertexInputDescs from shader code.
	// If `vertex_input_descs` is not NULL, it's filled with VertexInputDesc values. The function
	// always returns number of vertex inputs to the shader (which is also a length of `vertex_input_descs`
	// array).
	uint32_t get_vertex_input_desc_from_shader(char *vertex_string, uint32_t size, VertexInputDesc *vertex_input_descs);

	// Helper functions that return VertexShader directly from a vertex shader code
	VertexShader get_vertex_shader_from_code(char *code, uint32_t code_length);
	
	// Helper functions that return PixelShader directly from a pixel shader code
	PixelShader get_pixel_shader_from_code(char *code, uint32_t code_length);

	// Helper functions that return ComputeShader directly from a compute shader code
	ComputeShader get_compute_shader_from_code(char *code, uint32_t code_length);
}
