#include "graphics.h"
#include "memory.h"
#include <d3dcompiler.h>
#include <assert.h>
#ifdef CPPLIB_DEBUG_PRINTS
#include "logging.h"
#define PRINT_DEBUG(message, ...) logging::print_error(message, ##__VA_ARGS__)
#else
#define PRINT_DEBUG(message, ...)
#endif
#include <fstream>
#include <sstream>
#include <ios>
#define RELEASE_DX_RESOURCE(resource) if(resource) resource->Release(); resource = NULL;

// Global variables, only one of those for the whole application!

// Just so we don't have to reference graphics_context with &, we're doing this "trick"
static GraphicsContext graphics_context_;
GraphicsContext *graphics_context = &graphics_context_;

// The same as above
static SwapChain swap_chain_;
static SwapChain *swap_chain = &swap_chain_;

// We're simplifying blending to just two options - alpha blending and opaque (solid) blending
static ID3D11BlendState *blend_states[2];

// Let's store current blend type, useful when we want to restore original blending state in the end of the function
static BlendType current_blend_type;

static ID3D11RasterizerState *raster_states[2];

// Do the same for RasterType as for BlendType
static RasterType current_raster_type;

/////////////////////////////////////////////////////
/// Public API
/////////////////////////////////////////////////////

bool graphics::init(LUID *adapter_luid)
{
	UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#ifdef DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// Create IDXGIFactory, needed for probing avaliable devices/adapters
	IDXGIFactory *idxgi_factory;
	HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&idxgi_factory));
	if (FAILED(hr))
	{
		PRINT_DEBUG("Failed to create IDXGI factory.");
		return false;
	}

	// Get adapter to use for creating D3D11Device
	IDXGIAdapter *adapter = NULL;

	// In case adapter LUID was specified, go through available adapters and pick the one with specified LUID
	if (adapter_luid)
	{
		IDXGIAdapter *temp_adapter = NULL;
		for (uint32_t i = 0; idxgi_factory->EnumAdapters(i, &temp_adapter) != DXGI_ERROR_NOT_FOUND; ++i) 
		{ 
			DXGI_ADAPTER_DESC temp_adapter_desc;
			temp_adapter->GetDesc(&temp_adapter_desc);
			if(memcmp(&temp_adapter_desc.AdapterLuid, adapter_luid, sizeof(LUID)) == 0)
			{
				adapter = temp_adapter;
				break;
			}
			temp_adapter->Release();
		}
	}
	idxgi_factory->Release();

	// Create D3D11Device and D3D11DeviceContext
	D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL supported_feature_level;

	auto driver_type = adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE;
	hr = D3D11CreateDevice(adapter, driver_type, NULL, flags, &feature_level, 1, D3D11_SDK_VERSION, &graphics_context->device, &supported_feature_level, &graphics_context->context);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create D3D11 Device.");
		return false;
	}

	// Release adapter handle if not NULL
	if (adapter)
	{
		adapter->Release();
	}

	// Initialize blending states
	// For solid blend state, blend_state_desc is zeroed.
	D3D11_BLEND_DESC blend_state_desc = {};
	blend_state_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	hr = graphics_context->device->CreateBlendState(&blend_state_desc, &blend_states[BlendType::OPAQUE]);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create blend state.");
		return false;
	}

	// Initialize alpha blend state
	blend_state_desc.RenderTarget[0].BlendEnable 		   = TRUE;
	blend_state_desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
	blend_state_desc.RenderTarget[0].DestBlend 			   = D3D11_BLEND_INV_SRC_ALPHA;
	blend_state_desc.RenderTarget[0].BlendOp	 		   = D3D11_BLEND_OP_ADD;
	blend_state_desc.RenderTarget[0].SrcBlendAlpha	 	   = D3D11_BLEND_ONE;
	blend_state_desc.RenderTarget[0].DestBlendAlpha	 	   = D3D11_BLEND_ZERO;
	blend_state_desc.RenderTarget[0].BlendOpAlpha	 	   = D3D11_BLEND_OP_ADD;

	hr = graphics_context->device->CreateBlendState(&blend_state_desc, &blend_states[BlendType::ALPHA]);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create blend state.");
		return false;
	}
	
	current_blend_type = BlendType::OPAQUE;

	// Initialize rasterizer states
	// Initialize solid rasterizer state, let's follow RH coordinate system like sane people
	D3D11_RASTERIZER_DESC rasterizer_desc_solid = {};
	rasterizer_desc_solid.FillMode = D3D11_FILL_SOLID;
	rasterizer_desc_solid.CullMode = D3D11_CULL_NONE;
	rasterizer_desc_solid.FrontCounterClockwise = TRUE;

	hr = graphics_context->device->CreateRasterizerState(&rasterizer_desc_solid, &raster_states[RasterType::SOLID]);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create Rasterizer state");
		return false;
	}

	// Initialize wireframe rasterizer state, let's follow RH coordinate system like sane people
	D3D11_RASTERIZER_DESC rasterizer_desc_wireframe = {};
	rasterizer_desc_wireframe.FillMode = D3D11_FILL_WIREFRAME;
	rasterizer_desc_wireframe.CullMode = D3D11_CULL_BACK;
	rasterizer_desc_wireframe.FrontCounterClockwise = TRUE;

	hr = graphics_context->device->CreateRasterizerState(&rasterizer_desc_wireframe, &raster_states[RasterType::WIREFRAME]);
	if (FAILED(hr))
	{
		PRINT_DEBUG("Failed to create Rasterizer state");
		return false;
	}

	current_raster_type = RasterType::SOLID;
	graphics_context->context->RSSetState(raster_states[current_raster_type]);
	return true;
}

bool graphics::init_swap_chain(Window *window)
{
	DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};

	swap_chain_desc.BufferDesc.Width = window->window_width;
	swap_chain_desc.BufferDesc.Height = window->window_height;
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.BufferCount = 2;
	swap_chain_desc.OutputWindow = window->window_handle;
	swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
	swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
	swap_chain_desc.Windowed = true;
	swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	IDXGIFactory * idxgi_factory;
	HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&idxgi_factory));
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create IDXGI factory.");
		return false;
	}

	hr = idxgi_factory->CreateSwapChain(graphics_context->device, &swap_chain_desc, &swap_chain->swap_chain);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create DXGI swap chain.");
		return false;
	}

	idxgi_factory->Release();
	return true;
}

RenderTarget graphics::get_render_target_window()
{
	// NOTE: buffer.sr_view is not filled and remains NULL - window render target cannot be used as a texture in shader
	RenderTarget buffer = {};

	HRESULT hr = swap_chain->swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&buffer.texture);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to get swap chain buffer.");
		return RenderTarget{};
	}

	DXGI_SWAP_CHAIN_DESC swap_chain_desc;
	hr = swap_chain->swap_chain->GetDesc(&swap_chain_desc);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to get swap chain description.");
		return RenderTarget{};
	}

	D3D11_RENDER_TARGET_VIEW_DESC render_target_desc = {};
	render_target_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	render_target_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	hr = graphics_context->device->CreateRenderTargetView(buffer.texture, &render_target_desc, &buffer.rt_view);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create swap chain render target.");
		return RenderTarget{};
	}

	buffer.width = swap_chain_desc.BufferDesc.Width;
	buffer.height = swap_chain_desc.BufferDesc.Height;

	return buffer;
}

RenderTarget graphics::get_render_target(uint32_t width, uint32_t height, DXGI_FORMAT format)
{
	RenderTarget buffer = {};

	D3D11_TEXTURE2D_DESC texture_desc = {};
	texture_desc.Width = width;
	texture_desc.Height = height;
	texture_desc.MipLevels = 1;
	texture_desc.ArraySize = 1;
	texture_desc.Format = format;
	texture_desc.SampleDesc.Count = 1;
	texture_desc.SampleDesc.Quality = 0;
	texture_desc.Usage = D3D11_USAGE_DEFAULT;
	texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

	HRESULT hr = graphics_context->device->CreateTexture2D(&texture_desc, NULL, &buffer.texture);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create texture for render target buffer.");
		return RenderTarget{};
	}

	D3D11_RENDER_TARGET_VIEW_DESC render_target_desc = {};
	render_target_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	render_target_desc.Format = format;

	hr = graphics_context->device->CreateRenderTargetView(buffer.texture, &render_target_desc, &buffer.rt_view);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create render target view.");
		return RenderTarget{};
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_desc = {};
	shader_resource_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shader_resource_desc.Format = format;
	shader_resource_desc.Texture2D.MipLevels = 1;
	shader_resource_desc.Texture2D.MostDetailedMip = 0;

	hr = graphics_context->device->CreateShaderResourceView(buffer.texture, &shader_resource_desc, &buffer.sr_view);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create shader resource view.");
		return RenderTarget{};
	}

	buffer.width = width;
	buffer.height = height;

	return buffer;
}

void graphics::clear_render_target(RenderTarget *buffer, float r, float g, float b, float a)
{
	float color[4] = { r, g, b, a };
	graphics_context->context->ClearRenderTargetView(buffer->rt_view, color);
}

DepthBuffer graphics::get_depth_buffer(uint32_t width, uint32_t height)
{
	DepthBuffer buffer = {};

	D3D11_TEXTURE2D_DESC texture_desc = {};
	texture_desc.Width = width;
	texture_desc.Height = height;
	texture_desc.MipLevels = 1;
	texture_desc.ArraySize = 1;
	texture_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	texture_desc.SampleDesc.Count = 1;
	texture_desc.SampleDesc.Quality = 0;
	texture_desc.Usage = D3D11_USAGE_DEFAULT;
	texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;

	HRESULT hr = graphics_context->device->CreateTexture2D(&texture_desc, NULL, &buffer.texture);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create texture for depth stencil buffer.");
		return DepthBuffer{};
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_desc = {};
	depth_stencil_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	hr = graphics_context->device->CreateDepthStencilView(buffer.texture, &depth_stencil_desc, &buffer.ds_view);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create depth stencil view.");
		return DepthBuffer{};
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_desc = {};
	shader_resource_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shader_resource_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	shader_resource_desc.Texture2D.MipLevels = 1;
	shader_resource_desc.Texture2D.MostDetailedMip = 0;

	hr = graphics_context->device->CreateShaderResourceView(buffer.texture, &shader_resource_desc, &buffer.sr_view);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create shader resource view.");
		return DepthBuffer{};
	}

	buffer.width = width;
	buffer.height = height;

	return buffer;
}

void graphics::clear_depth_buffer(DepthBuffer *buffer)
{
	graphics_context->context->ClearDepthStencilView(buffer->ds_view, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void graphics::set_viewport(RenderTarget *buffer)
{
	D3D11_VIEWPORT viewport = {};
	viewport.Width = (float)buffer->width;
	viewport.Height = (float)buffer->height;
	viewport.MaxDepth = 1.0f;
	graphics_context->context->RSSetViewports(1, &viewport);
}

void graphics::set_viewport(DepthBuffer *buffer)
{
	D3D11_VIEWPORT viewport = {};
	viewport.Width = (float)buffer->width;
	viewport.Height = (float)buffer->height;
	viewport.MaxDepth = 1.0f;
	graphics_context->context->RSSetViewports(1, &viewport);
}

void graphics::set_viewport(Viewport *target_viewport)
{
	D3D11_VIEWPORT viewport = {};

	viewport.Width    = target_viewport->width;
	viewport.Height   = target_viewport->height;
	viewport.TopLeftX = target_viewport->x;
	viewport.TopLeftY = target_viewport->y;
	viewport.MaxDepth = 1.0f;

	graphics_context->context->RSSetViewports(1, &viewport);
}

void graphics::set_render_targets(DepthBuffer *buffer)
{
	ID3D11RenderTargetView **null = { NULL };
	graphics_context->context->OMSetRenderTargets(0, null, buffer->ds_view);
}

void graphics::set_render_targets(RenderTarget *buffer)
{
	graphics_context->context->OMSetRenderTargets(1, &buffer->rt_view, NULL);
}

void graphics::set_render_targets(RenderTarget *buffer, DepthBuffer *depth_buffer)
{
	graphics_context->context->OMSetRenderTargets(1, &buffer->rt_view, depth_buffer->ds_view);
}

void graphics::set_render_targets_viewport(RenderTarget *buffers, uint32_t buffer_count, DepthBuffer *depth_buffer)
{
	memory::push_temp_state();
	D3D11_VIEWPORT *viewports = memory::alloc_temp<D3D11_VIEWPORT>(buffer_count);

	for (uint32_t i = 0; i < buffer_count; ++i)
	{
		viewports[i] = {};
		viewports[i].Width = (float)buffers[i].width;
		viewports[i].Height = (float)buffers[i].height;
		viewports[i].MaxDepth = 1.0f;
	}

	ID3D11RenderTargetView **rt_views = memory::alloc_temp<ID3D11RenderTargetView *>(buffer_count);
	for (uint32_t i = 0; i < buffer_count; ++i)
	{
		rt_views[i] = buffers[i].rt_view;
	}

	graphics_context->context->RSSetViewports(buffer_count, viewports);
	graphics_context->context->OMSetRenderTargets(buffer_count, rt_views, depth_buffer->ds_view);

	memory::pop_temp_state();
}

void graphics::set_render_targets_viewport(RenderTarget *buffer, DepthBuffer *depth_buffer)
{
	set_viewport(buffer);
	set_render_targets(buffer, depth_buffer);
}

void graphics::set_render_targets_viewport(RenderTarget *buffer)
{
	set_viewport(buffer);
	set_render_targets(buffer);
}

Texture2D graphics::get_texture2D(void *data, uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t pixel_byte_count)
{
	Texture2D texture;

	D3D11_TEXTURE2D_DESC texture_desc = {};
	texture_desc.Width = width;
	texture_desc.Height = height;
	texture_desc.MipLevels = 1;
	texture_desc.ArraySize = 1;
	texture_desc.Format = format;
	texture_desc.SampleDesc.Count = 1;
	texture_desc.SampleDesc.Quality = 0;
	// TODO: Maybe not the best Usage flag.
	texture_desc.Usage = D3D11_USAGE_DEFAULT;
	texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

	D3D11_SUBRESOURCE_DATA texture_data = {};
	texture_data.pSysMem = data;
	texture_data.SysMemPitch = width * pixel_byte_count;

	D3D11_SUBRESOURCE_DATA *texture_data_ptr = data ? &texture_data : NULL;
	HRESULT hr = graphics_context->device->CreateTexture2D(&texture_desc, texture_data_ptr, &texture.texture);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create 2D texture.");
		return Texture2D{};
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_desc = {};
	shader_resource_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shader_resource_desc.Format = format;
	shader_resource_desc.Texture2D.MipLevels = 1;
	shader_resource_desc.Texture2D.MostDetailedMip = 0;

	hr = graphics_context->device->CreateShaderResourceView(texture.texture, &shader_resource_desc, &texture.sr_view);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create shader resource view.");
		return Texture2D{};
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC unordered_access_desc = {};
	unordered_access_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	unordered_access_desc.Format = format;
	unordered_access_desc.Texture2D.MipSlice = 0;

	hr = graphics_context->device->CreateUnorderedAccessView(texture.texture, &unordered_access_desc, &texture.ua_view);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create unordered access view.");
		return Texture2D{};
	}

	texture.width = width;
	texture.height = height;

	return texture;
}

Texture3D graphics::get_texture3D(void *data, uint32_t width, uint32_t height, uint32_t depth, DXGI_FORMAT format, uint32_t pixel_byte_count)
{
	Texture3D texture;

	D3D11_TEXTURE3D_DESC texture_desc = {};
	texture_desc.Width = width;
	texture_desc.Height = height;
	texture_desc.Depth = depth;	
	texture_desc.MipLevels = 1;
	texture_desc.Format = format;
	// TODO: Maybe not the best Usage flag.
	texture_desc.Usage = D3D11_USAGE_DEFAULT;
	texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

	D3D11_SUBRESOURCE_DATA texture_data = {};
	texture_data.pSysMem = data;
	texture_data.SysMemPitch = width * height * pixel_byte_count;

	D3D11_SUBRESOURCE_DATA *texture_data_ptr = data ? &texture_data : NULL;
	HRESULT hr = graphics_context->device->CreateTexture3D(&texture_desc, texture_data_ptr, &texture.texture);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create 3D texture.");
		return Texture3D{};
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_desc = {};
	shader_resource_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	shader_resource_desc.Format = format;
	shader_resource_desc.Texture3D.MipLevels = 1;
	shader_resource_desc.Texture3D.MostDetailedMip = 0;

	hr = graphics_context->device->CreateShaderResourceView(texture.texture, &shader_resource_desc, &texture.sr_view);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create shader resource view.");
		return Texture3D{};
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC unordered_access_desc = {};
	unordered_access_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	unordered_access_desc.Format = format;
	unordered_access_desc.Texture3D.MipSlice = 0;
	unordered_access_desc.Texture3D.FirstWSlice = 0;
	unordered_access_desc.Texture3D.WSize = depth;

	hr = graphics_context->device->CreateUnorderedAccessView(texture.texture, &unordered_access_desc, &texture.ua_view);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create unordered access view.");
		return Texture3D{};
	}

	texture.width = width;
	texture.height = height;
	texture.depth = depth;

	return texture;
}

Texture2D graphics::load_texture2D(std::string filename)
{
	std::wstringstream stream;
	stream << filename.c_str();

	DirectX::ScratchImage image;
	DirectX::TexMetadata *metadata = NULL;
	if (!SUCCEEDED(DirectX::LoadFromTGAFile(stream.str().c_str(), metadata, image))) {
		PRINT_DEBUG("Failed to load 2D texture.");
		return Texture2D{};
	}

	ID3D11Resource *pResource = NULL;
	if (!SUCCEEDED(DirectX::CreateTexture(graphics_context->device, image.GetImages(), image.GetImageCount(), image.GetMetadata(), &pResource))) {
		PRINT_DEBUG("Failed to create 2D texture resource.");
		return Texture2D{};
	}

	Texture2D texture;
	texture.texture = (ID3D11Texture2D*)pResource;
	texture.width = (uint32_t)image.GetMetadata().width;
	texture.height = (uint32_t)image.GetMetadata().height;

	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_desc = {};
	shader_resource_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shader_resource_desc.Format = image.GetMetadata().format;
	shader_resource_desc.Texture2D.MipLevels = 1;
	shader_resource_desc.Texture2D.MostDetailedMip = 0;

	if (!SUCCEEDED(graphics_context->device->CreateShaderResourceView(texture.texture, &shader_resource_desc, &texture.sr_view))) {
		PRINT_DEBUG("Failed to create shader resource view.");
		return Texture2D{};
	}

	return texture;
}

Texture3D graphics::load_texture3D(std::string filename)
{
	std::wstringstream stream;
	stream << filename.c_str();

	DirectX::ScratchImage image;
	DirectX::TexMetadata *metadata = NULL;
	if (!SUCCEEDED(DirectX::LoadFromDDSFile(stream.str().c_str(), DirectX::DDS_FLAGS_NONE, metadata, image))) {
		PRINT_DEBUG("Failed to load 3D texture.");

		return Texture3D{};
	}

	Texture3D texture;

	D3D11_TEXTURE3D_DESC texture_desc = {};
	texture_desc.Width = (uint32_t)image.GetMetadata().width;
	texture_desc.Height = (uint32_t)image.GetMetadata().height;
	texture_desc.Depth = (uint32_t)image.GetMetadata().depth;
	texture_desc.MipLevels = 1;
	texture_desc.Format =  image.GetMetadata().format;
	// TODO: Maybe not the best Usage flag.
	texture_desc.Usage = D3D11_USAGE_DEFAULT;
	texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	

	D3D11_SUBRESOURCE_DATA texture_data = {};
	texture_data.pSysMem = image.GetImages()[0].pixels;
	texture_data.SysMemPitch = (uint32_t)image.GetImages()[0].rowPitch;
	texture_data.SysMemSlicePitch = (uint32_t)image.GetImages()[0].slicePitch;

	D3D11_SUBRESOURCE_DATA *texture_data_ptr = &texture_data;
	HRESULT hr = graphics_context->device->CreateTexture3D(&texture_desc, texture_data_ptr, &texture.texture);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create 3D texture.");
		printf("Failed to create 3D texture. %x\n", hr);
		return Texture3D{};
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_desc = {};
	shader_resource_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	shader_resource_desc.Format = image.GetMetadata().format;
	shader_resource_desc.Texture3D.MipLevels = 1;
	shader_resource_desc.Texture3D.MostDetailedMip = 0;

	hr = graphics_context->device->CreateShaderResourceView(texture.texture, &shader_resource_desc, &texture.sr_view);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create shader resource view.");
		printf("Failed to create shader resource view. \n");
		return Texture3D{};
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC unordered_access_desc = {};
	unordered_access_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	unordered_access_desc.Format = image.GetMetadata().format;
	unordered_access_desc.Texture3D.MipSlice = 0;
	unordered_access_desc.Texture3D.FirstWSlice = 0;
	unordered_access_desc.Texture3D.WSize = (uint32_t)image.GetMetadata().depth;

	hr = graphics_context->device->CreateUnorderedAccessView(texture.texture, &unordered_access_desc, &texture.ua_view);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create unordered access view.");
		printf("Failed to create unordered access view. \n");
		return Texture3D{};
	}

	texture.width = (uint32_t)image.GetMetadata().width;
	texture.height = (uint32_t)image.GetMetadata().height;
	texture.depth = (uint32_t)image.GetMetadata().depth;

	printf("loaded texture \n");
	return texture;
}

void graphics::save_texture2D(Texture2D *texture, std::string filename)
{
	DirectX::ScratchImage image;
	if (!SUCCEEDED(DirectX::CaptureTexture(graphics_context->device, graphics_context->context, texture->texture, image))) {
		PRINT_DEBUG("Failed to capture 2D texture.");
		printf("Failed to capture 2D texture.");
		return;
	}

	std::wstringstream stream;
	stream << filename.c_str() << ".tga";
	if (!SUCCEEDED(DirectX::SaveToTGAFile(*image.GetImages(), stream.str().c_str()))) {
		PRINT_DEBUG("Failed to store 3D texture to specified file.");
		printf("Failed to store 3D texture to specified file.");
		return;
		image.Release();
	}

	image.Release();
}

void graphics::save_texture3D(Texture3D *texture, std::string filename)
{
	DirectX::ScratchImage image;
	if (!SUCCEEDED(DirectX::CaptureTexture(graphics_context->device, graphics_context->context, texture->texture, image))) {
		PRINT_DEBUG("Failed to capture 3D texture.");
		printf("Failed to capture 3D texture.");
		return;
	}

	std::wstringstream wstream;
	wstream << filename.c_str() << ".dds";
	if (!SUCCEEDED(DirectX::SaveToDDSFile(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::DDS_FLAGS_NONE, wstream.str().c_str()))) {
		PRINT_DEBUG("Failed to store 3D texture to specified file.");
		printf("Failed to store 3D texture to specified file.");
		return;
		image.Release();
	}

	std::stringstream stream;
	stream << filename.c_str() << ".bin";
	std::ofstream bin_file(stream.str().c_str(), std::ios::out | std::ios::binary);
	if (bin_file.is_open()) {
		size_t tex_byte_size = image.GetPixelsSize();
		uint8_t* raw_data = image.GetPixels();
        bin_file.write((char*)raw_data, tex_byte_size);
        bin_file.close();
    }

	image.Release();
}

void graphics::capture_current_frame()
{
	ID3D11Texture2D* pBuffer;
	swap_chain->swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBuffer);

	Texture2D texture_to_save;
	D3D11_TEXTURE2D_DESC td;
	pBuffer->GetDesc(&td);
	graphics_context->device->CreateTexture2D(&td, NULL, &texture_to_save.texture);
	graphics_context->context->CopyResource(texture_to_save.texture, pBuffer);

	static uint32_t count = 0;
	std::stringstream stream;
	stream << "capture\\frame" << count++;
	printf("Capturing %s\n", stream.str().c_str());
	save_texture2D(&texture_to_save, stream.str());

	RELEASE_DX_RESOURCE(texture_to_save.texture);
}

void graphics::set_texture(RenderTarget *buffer, uint32_t slot)
{
	graphics_context->context->PSSetShaderResources(slot, 1, &buffer->sr_view);
}

void graphics::set_texture(DepthBuffer *buffer, uint32_t slot)
{
	graphics_context->context->PSSetShaderResources(slot, 1, &buffer->sr_view);
}

void graphics::set_texture(Texture2D *texture, uint32_t slot)
{
	graphics_context->context->PSSetShaderResources(slot, 1, &texture->sr_view);
}

void graphics::set_texture(Texture3D *texture, uint32_t slot)
{
	graphics_context->context->PSSetShaderResources(slot, 1, &texture->sr_view);
}

void graphics::unset_texture(uint32_t slot)
{
	ID3D11ShaderResourceView *null[] = { NULL };
	graphics_context->context->PSSetShaderResources(slot, 1, null);
}

void graphics::set_texture_compute(Texture2D *texture, uint32_t slot)
{
	UINT init_counts = 0;
	graphics_context->context->CSSetUnorderedAccessViews(slot, 1, &texture->ua_view, &init_counts);
}

void graphics::set_texture_sampled_compute(Texture2D *texture, uint32_t slot)
{
	graphics_context->context->CSSetShaderResources(slot, 1, &texture->sr_view);
}

void graphics::set_texture_compute(Texture3D *texture, uint32_t slot)
{
	UINT init_counts = 0;
	graphics_context->context->CSSetUnorderedAccessViews(slot, 1, &texture->ua_view, &init_counts);
}

void graphics::set_texture_sampled_compute(Texture3D *texture, uint32_t slot)
{
	graphics_context->context->CSSetShaderResources(slot, 1, &texture->sr_view);
}

void graphics::unset_texture_compute(uint32_t slot)
{
	UINT init_counts = 0;
	ID3D11UnorderedAccessView *null[] = { NULL };
	graphics_context->context->CSSetUnorderedAccessViews(slot, 1, null, &init_counts);
}

void graphics::unset_texture_sampled_compute(uint32_t slot)
{
	ID3D11ShaderResourceView *null[] = { NULL };
	graphics_context->context->CSSetShaderResources(slot, 1, null);
}

void graphics::set_blend_state(BlendType type)
{
	current_blend_type = type;
	float blend_factor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	graphics_context->context->OMSetBlendState(blend_states[current_blend_type], blend_factor, 0xffffffff);
}

BlendType graphics::get_blend_state()
{
	return current_blend_type;
}

void graphics::set_rasterizer_state(RasterType type)
{
	current_raster_type = type;
	graphics_context->context->RSSetState(raster_states[current_raster_type]);
}

RasterType graphics::get_rasterizer_state()
{
	return current_raster_type;
}

D3D11_TEXTURE_ADDRESS_MODE m2m[3] =
{
	D3D11_TEXTURE_ADDRESS_CLAMP,
	D3D11_TEXTURE_ADDRESS_WRAP,
	D3D11_TEXTURE_ADDRESS_BORDER
};

TextureSampler graphics::get_texture_sampler(SampleMode mode, D3D11_FILTER filter)
{
	TextureSampler sampler;

	D3D11_TEXTURE_ADDRESS_MODE address_mode = m2m[mode];
	D3D11_SAMPLER_DESC sampler_desc = {};
	sampler_desc.Filter = filter;
	sampler_desc.AddressU = address_mode;
	sampler_desc.AddressV = address_mode;
	sampler_desc.AddressW = address_mode;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampler_desc.MinLOD = -D3D11_FLOAT32_MAX;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
	HRESULT hr = graphics_context->device->CreateSamplerState(&sampler_desc, &sampler.sampler);
	if(FAILED(hr)) {
		return TextureSampler{};
	}

	return sampler;
}

void graphics::set_texture_sampler(TextureSampler *sampler, uint32_t slot) 
{
	graphics_context->context->PSSetSamplers(slot, 1, &sampler->sampler);
}

void graphics::set_texture_sampler_compute(TextureSampler *sampler, uint32_t slot) 
{
	graphics_context->context->CSSetSamplers(slot, 1, &sampler->sampler);
}


Mesh graphics::get_mesh(void *vertices, uint32_t vertex_count, uint32_t vertex_stride, void *indices, uint32_t index_count, uint32_t index_byte_size, D3D11_PRIMITIVE_TOPOLOGY topology)
{
	Mesh mesh = {};
	
	D3D11_BUFFER_DESC vertex_buffer_desc = {};
	vertex_buffer_desc.ByteWidth = vertex_count * vertex_stride;
	vertex_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
	vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertex_buffer_data = {};
	vertex_buffer_data.pSysMem = vertices;
	vertex_buffer_data.SysMemPitch = vertex_stride;

	HRESULT hr = graphics_context->device->CreateBuffer(&vertex_buffer_desc, &vertex_buffer_data, &mesh.vertex_buffer);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create vertex buffer.");
		return Mesh{};
	}

	if (indices && index_count > 0)
	{
		D3D11_BUFFER_DESC index_buffer_desc = {};
		index_buffer_desc.ByteWidth = index_count * index_byte_size;
		index_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
		index_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA index_buffer_data = {};
		index_buffer_data.pSysMem = indices;
		index_buffer_data.SysMemPitch = index_byte_size;

		hr = graphics_context->device->CreateBuffer(&index_buffer_desc, &index_buffer_data, &mesh.index_buffer);
		if (FAILED(hr)) {
			PRINT_DEBUG("Failed to create index buffer.");
			return Mesh{};
		}
	}

	mesh.vertex_stride = vertex_stride;
	mesh.vertex_offset = 0;
	mesh.vertex_count = vertex_count;
	mesh.index_count = index_count;
	mesh.index_format = index_byte_size == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	mesh.topology = topology;

	return mesh;
}

void graphics::draw_mesh(Mesh *mesh)
{
	graphics_context->context->IASetVertexBuffers(0, 1, &mesh->vertex_buffer, &mesh->vertex_stride, &mesh->vertex_offset);

	graphics_context->context->IASetPrimitiveTopology(mesh->topology);
	if(mesh->index_buffer) {
		graphics_context->context->IASetIndexBuffer(mesh->index_buffer, mesh->index_format, 0);
		graphics_context->context->DrawIndexed(mesh->index_count, 0, 0);
	} else {
		graphics_context->context->Draw(mesh->vertex_count, 0);
	}
}

ConstantBuffer graphics::get_constant_buffer(uint32_t size)
{
	ConstantBuffer buffer = {};
	buffer.size = size;

	D3D11_BUFFER_DESC constant_buffer_desc = {};
	constant_buffer_desc.ByteWidth = size;
	constant_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	constant_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constant_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	graphics_context->device->CreateBuffer(&constant_buffer_desc, NULL, &buffer.buffer);

	return buffer;
}

StructuredBuffer graphics::get_structured_buffer(int element_stride, int num_elements)
{
	StructuredBuffer buffer = {};
	buffer.size = element_stride * num_elements;

	D3D11_BUFFER_DESC constant_buffer_desc = {};
	constant_buffer_desc.ByteWidth = element_stride * num_elements;
	constant_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	constant_buffer_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;// | D3D11_BIND_SHADER_RESOURCE;
	constant_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	constant_buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	constant_buffer_desc.StructureByteStride = element_stride;

	HRESULT hr = graphics_context->device->CreateBuffer(&constant_buffer_desc, NULL, &buffer.buffer);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create buffer.");
		return StructuredBuffer{};
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC unordered_access_desc = {};
	unordered_access_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	unordered_access_desc.Format = DXGI_FORMAT_UNKNOWN;
	unordered_access_desc.Buffer.FirstElement = 0;
	unordered_access_desc.Buffer.Flags = 0;
	unordered_access_desc.Buffer.NumElements = num_elements;

	hr = graphics_context->device->CreateUnorderedAccessView(buffer.buffer, &unordered_access_desc, &buffer.ua_view);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create unordered access view.");
		return StructuredBuffer{};
	}

	return buffer;
}

void graphics::capture_structured_buffer(StructuredBuffer *buffer, void *mapped_data, unsigned int num_elements, size_t element_size)
{
	D3D11_MAPPED_SUBRESOURCE ms;
	if (!SUCCEEDED(graphics_context->context->Map(buffer->buffer, 0, D3D11_MAP_READ, 0, &ms))) {
		PRINT_DEBUG("Failed to map GPU buffer.");
		return;
	}
	memcpy(mapped_data, ms.pData, num_elements * element_size);
	graphics_context->context->Unmap(buffer->buffer, 0);
}

void graphics::update_constant_buffer(ConstantBuffer *buffer, void *data)
{
	D3D11_MAPPED_SUBRESOURCE mapped_buffer;
	graphics_context->context->Map(buffer->buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_buffer);
	memcpy(mapped_buffer.pData, data, buffer->size);
	graphics_context->context->Unmap(buffer->buffer, 0);
}

void graphics::update_structured_buffer(StructuredBuffer *buffer, void *data)
{
	graphics_context->context->UpdateSubresource(buffer->buffer, 0, NULL, data, 0, 0);
}

void graphics::set_constant_buffer(ConstantBuffer *buffer, uint32_t slot)
{
	graphics_context->context->PSSetConstantBuffers(slot, 1, &buffer->buffer);
	graphics_context->context->GSSetConstantBuffers(slot, 1, &buffer->buffer);
	graphics_context->context->VSSetConstantBuffers(slot, 1, &buffer->buffer);
	graphics_context->context->CSSetConstantBuffers(slot, 1, &buffer->buffer);
}

void graphics::set_structured_buffer(StructuredBuffer *buffer, uint32_t slot)
{
	UINT init_counts = 0;
	graphics_context->context->CSSetUnorderedAccessViews(slot, 1, &buffer->ua_view, &init_counts);
}

CompiledShader compile_shader(void *source, uint32_t source_size, char *target)
{
	CompiledShader compiled_shader;

	uint32_t flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;// | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef DEBUG
	flags |= D3DCOMPILE_DEBUG;
#endif
	ID3DBlob *error_msg;
	HRESULT hr = D3DCompile(source, source_size, NULL, NULL, NULL, "main", target, flags, NULL, &compiled_shader.blob, &error_msg);
	if (FAILED(hr)) {
		printf("Error!\n"); 
		PRINT_DEBUG("Failed to compile shader!");
		if (error_msg) {
			printf("%s\n", (char *)error_msg->GetBufferPointer());
			printf("Error!\n"); 
			PRINT_DEBUG((char *)error_msg->GetBufferPointer());
			error_msg->Release();
		}
		return CompiledShader{};
	}

	return compiled_shader;
}

CompiledShader graphics::compile_vertex_shader(void *source, uint32_t source_size)
{
	CompiledShader vertex_shader = compile_shader(source, source_size, "vs_5_0");
	return vertex_shader;
}

CompiledShader graphics::compile_pixel_shader(void *source, uint32_t source_size)
{
	CompiledShader pixel_shader = compile_shader(source, source_size, "ps_5_0");
	return pixel_shader;
}

CompiledShader graphics::compile_geometry_shader(void *source, uint32_t source_size)
{
	CompiledShader geometry_shader = compile_shader(source, source_size, "gs_5_0");
	return geometry_shader;
}

CompiledShader graphics::compile_compute_shader(void *source, uint32_t source_size)
{
	CompiledShader compute_shader = compile_shader(source, source_size, "cs_5_0");
	return compute_shader;
}

VertexShader graphics::get_vertex_shader(CompiledShader *compiled_shader, VertexInputDesc *vertex_input_descs, uint32_t vertex_input_count) 
{
	VertexShader vertex_shader = graphics::get_vertex_shader(compiled_shader->blob->GetBufferPointer(),
															 (uint32_t)compiled_shader->blob->GetBufferSize(),
															 vertex_input_descs, vertex_input_count);
	return vertex_shader;
}

VertexShader graphics::get_vertex_shader(void *shader_byte_code, uint32_t shader_size, VertexInputDesc *vertex_input_descs, uint32_t vertex_input_count)
{
	memory::push_temp_state();

	VertexShader shader = {};
	HRESULT hr = graphics_context->device->CreateVertexShader(shader_byte_code, shader_size, NULL, &shader.vertex_shader);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create vertex shader.");
		return VertexShader{};
	}

	D3D11_INPUT_ELEMENT_DESC *input_layout_desc = memory::alloc_temp<D3D11_INPUT_ELEMENT_DESC>(vertex_input_count);
	for (uint32_t i = 0; i < vertex_input_count; ++i)
	{
		input_layout_desc[i] = {};
		input_layout_desc[i].SemanticName = vertex_input_descs[i].semantic_name;
		input_layout_desc[i].Format = vertex_input_descs[i].format;
		input_layout_desc[i].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	}

	hr = graphics_context->device->CreateInputLayout(input_layout_desc, vertex_input_count, shader_byte_code, shader_size, &shader.input_layout);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create input layout.");
		return VertexShader{};
	}

	memory::pop_temp_state();
	return shader;
}

void graphics::set_vertex_shader(VertexShader *shader)
{
	graphics_context->context->IASetInputLayout(shader->input_layout);
	graphics_context->context->VSSetShader(shader->vertex_shader, NULL, 0);
}

PixelShader graphics::get_pixel_shader(CompiledShader *compiled_shader)
{
	PixelShader pixel_shader = graphics::get_pixel_shader(compiled_shader->blob->GetBufferPointer(),
														  (uint32_t)compiled_shader->blob->GetBufferSize());
	return pixel_shader;
}

PixelShader graphics::get_pixel_shader(void *shader_byte_code, uint32_t shader_size)
{
	PixelShader shader = {};

	HRESULT hr = graphics_context->device->CreatePixelShader(shader_byte_code, shader_size, NULL, &shader.pixel_shader);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create pixel shader.");
		return PixelShader{};
	}
	
	return shader;
}

void graphics::set_pixel_shader()
{
	graphics_context->context->PSSetShader(NULL, NULL, 0);
}

void graphics::set_pixel_shader(PixelShader *shader)
{
	graphics_context->context->PSSetShader(shader->pixel_shader, NULL, 0);
}

GeometryShader graphics::get_geometry_shader(CompiledShader *compiled_shader)
{
	GeometryShader geometry_shader = graphics::get_geometry_shader(compiled_shader->blob->GetBufferPointer(),
																   (uint32_t) compiled_shader->blob->GetBufferSize());
	return geometry_shader;
}

GeometryShader graphics::get_geometry_shader(void *shader_byte_code, uint32_t shader_size)
{
	GeometryShader shader = {};

	HRESULT hr = graphics_context->device->CreateGeometryShader(shader_byte_code, shader_size, NULL, &shader.geometry_shader);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create geometry shader.");
		return GeometryShader{};
	}

	return shader;
}

void graphics::set_geometry_shader()
{
	graphics_context->context->GSSetShader(NULL, NULL, 0);
}

void graphics::set_geometry_shader(GeometryShader *shader)
{
	graphics_context->context->GSSetShader(shader->geometry_shader, NULL, 0);
}

ComputeShader graphics::get_compute_shader(CompiledShader *compiled_shader)
{
	ComputeShader compute_shader = graphics::get_compute_shader(compiled_shader->blob->GetBufferPointer(),
																(uint32_t) compiled_shader->blob->GetBufferSize());
	return compute_shader;
}

ComputeShader graphics::get_compute_shader(void *shader_byte_code, uint32_t shader_size)
{
	ComputeShader shader = {};

	HRESULT hr = graphics_context->device->CreateComputeShader(shader_byte_code, shader_size, NULL, &shader.compute_shader);
	if (FAILED(hr)) {
		PRINT_DEBUG("Failed to create compute shader.");
		return ComputeShader{};
	}

	return shader;
}

void graphics::set_compute_shader()
{
	graphics_context->context->CSSetShader(NULL, NULL, 0);
}

void graphics::set_compute_shader(ComputeShader *shader)
{
	graphics_context->context->CSSetShader(shader->compute_shader, NULL, 0);
}

void graphics::run_compute(int group_x, int group_y, int group_z)
{
	graphics_context->context->Dispatch(group_x, group_y, group_z);
}


void graphics::swap_frames()
{
	swap_chain->swap_chain->Present(1, 0);
}

void graphics::show_live_objects()
{
	ID3D11Debug *debug_device;
	graphics_context->device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&debug_device));
	debug_device->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
}

// is_ready functions

bool graphics::is_ready(Texture2D *texture)
{
	return texture->texture && texture->sr_view;
}

bool graphics::is_ready(Texture3D *texture)
{
	return texture->texture && texture->sr_view;
}

bool graphics::is_ready(RenderTarget *render_target)
{
	return render_target->rt_view && render_target->texture;
}

bool graphics::is_ready(DepthBuffer *depth_buffer)
{
	return depth_buffer->ds_view && depth_buffer->sr_view && depth_buffer->texture;
}

bool graphics::is_ready(Mesh *mesh)
{
	return mesh->vertex_buffer && mesh->index_buffer;
}

bool graphics::is_ready(ConstantBuffer *buffer)
{
	return buffer->buffer;
}

bool graphics::is_ready(TextureSampler *sampler)
{
	return sampler->sampler;
}

bool graphics::is_ready(VertexShader *shader)
{
	return shader->vertex_shader && shader->input_layout;
}

bool graphics::is_ready(PixelShader *shader)
{
	return shader->pixel_shader;
}

bool graphics::is_ready(ComputeShader *shader)
{
	return shader->compute_shader;
}

bool graphics::is_ready(CompiledShader *shader)
{
	return shader->blob;
}

// release functions
void graphics::release()
{
	RELEASE_DX_RESOURCE(swap_chain->swap_chain);
	RELEASE_DX_RESOURCE(graphics_context->context);
	RELEASE_DX_RESOURCE(graphics_context->device);
	RELEASE_DX_RESOURCE(blend_states[BlendType::OPAQUE]);
	RELEASE_DX_RESOURCE(blend_states[BlendType::ALPHA]);
	RELEASE_DX_RESOURCE(raster_states[RasterType::SOLID]);
	RELEASE_DX_RESOURCE(raster_states[RasterType::WIREFRAME]);
}

void graphics::release(RenderTarget *buffer)
{
	RELEASE_DX_RESOURCE(buffer->rt_view);
	RELEASE_DX_RESOURCE(buffer->sr_view);
	RELEASE_DX_RESOURCE(buffer->texture);
}

void graphics::release(DepthBuffer *buffer)
{
	RELEASE_DX_RESOURCE(buffer->ds_view);
	RELEASE_DX_RESOURCE(buffer->sr_view);
	RELEASE_DX_RESOURCE(buffer->texture);
}

void graphics::release(Texture2D *texture)
{
	RELEASE_DX_RESOURCE(texture->sr_view);
	RELEASE_DX_RESOURCE(texture->ua_view);
	RELEASE_DX_RESOURCE(texture->texture);
}

void graphics::release(Texture3D *texture)
{
	RELEASE_DX_RESOURCE(texture->sr_view);
	RELEASE_DX_RESOURCE(texture->ua_view);
	RELEASE_DX_RESOURCE(texture->texture);
}

void graphics::release(Mesh *mesh)
{
	RELEASE_DX_RESOURCE(mesh->vertex_buffer);
	RELEASE_DX_RESOURCE(mesh->index_buffer);
}

void graphics::release(VertexShader *shader)
{
	RELEASE_DX_RESOURCE(shader->vertex_shader);
	RELEASE_DX_RESOURCE(shader->input_layout);
}

void graphics::release(GeometryShader *shader)
{
	RELEASE_DX_RESOURCE(shader->geometry_shader);
}

void graphics::release(PixelShader *shader)
{
	RELEASE_DX_RESOURCE(shader->pixel_shader);
}

void graphics::release(ComputeShader *shader)
{
	RELEASE_DX_RESOURCE(shader->compute_shader);
}

void graphics::release(ConstantBuffer *buffer)
{
	RELEASE_DX_RESOURCE(buffer->buffer);
}

void graphics::release(StructuredBuffer *buffer)
{
	RELEASE_DX_RESOURCE(buffer->buffer);
	RELEASE_DX_RESOURCE(buffer->ua_view);
}

void graphics::release(TextureSampler *sampler)
{
	RELEASE_DX_RESOURCE(sampler->sampler);
}

void graphics::release(CompiledShader *shader)
{
	RELEASE_DX_RESOURCE(shader->blob);
}

////////////////////////////////////////////////
/// HIGHER LEVEL API
////////////////////////////////////////////////

uint32_t graphics::get_vertex_input_desc_from_shader(char *vertex_string, uint32_t size, VertexInputDesc * vertex_input_descs)
{
	const char *struct_name = "VertexInput";
	char *c = vertex_string;
	enum State
	{
		SEARCHING,
		PARSING_TYPE,
		SKIPPING_NAME,
		PARSING_SEMANTIC_NAME
	};

	State state = SEARCHING;
	uint32_t type_length = 0;
	uint32_t semantic_name_length = 0;
	uint32_t type = 0;

#define SHADER_TYPE_FLOAT4 0
#define SHADER_TYPE_FLOAT2 1
#define SHADER_TYPE_FLOAT3 2

	char *types[] =
	{
		"float4", "float2", "float3", "int4"
	};

	DXGI_FORMAT formats[]
	{
		DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_R32G32B32_FLOAT, DXGI_FORMAT_R32G32B32A32_SINT
	};

	uint32_t i = 0;
	uint32_t vertex_input_count = 0;

	while (i < size)
	{
		switch (state)
		{
		case SEARCHING:
		{
			if (strncmp(c, struct_name, strlen(struct_name)) == 0)
			{
				state = PARSING_TYPE;
				i += (uint32_t)strlen(struct_name);
				c += (uint32_t)strlen(struct_name);
			}
		}
		break;
		case PARSING_TYPE:
		{
			if (*c == ' ' && type_length > 0)
			{
				for (uint32_t j = 0; j < ARRAYSIZE(types); ++j)
				{
					if (strncmp(c - type_length, types[j], type_length) == 0)
					{
						type = j;

						state = SKIPPING_NAME;
						type_length = 0;

						break;
					}
				}
			}
			else if (isalnum(*c))
			{
				type_length++;
			}
		}
		break;
		case SKIPPING_NAME:
		{
			if (*c == ':')
			{
				state = PARSING_SEMANTIC_NAME;
			}
		}
		break;
		case PARSING_SEMANTIC_NAME:
		{
			if ((isspace(*c) || *c == ';') && semantic_name_length > 0)
			{
				assert(semantic_name_length < MAX_SEMANTIC_NAME_LENGTH);
				if (vertex_input_descs)
				{
					vertex_input_descs[vertex_input_count].format = formats[type];
					memcpy(vertex_input_descs[vertex_input_count].semantic_name, c - semantic_name_length, semantic_name_length);
					vertex_input_descs[vertex_input_count].semantic_name[semantic_name_length] = 0;
				}
				vertex_input_count++;

				state = PARSING_TYPE;
				semantic_name_length = 0;
			}
			else if (isalnum(*c) || *c == '_')
			{
				semantic_name_length++;
			}
		}
		}
		++c; ++i;
	}

	return vertex_input_count;
}

VertexShader graphics::get_vertex_shader_from_code(char *code, uint32_t code_length)
{
	memory::push_temp_state();

	// Compile shader
    CompiledShader vertex_shader_compiled = graphics::compile_vertex_shader(code, code_length);
	assert(graphics::is_ready(&vertex_shader_compiled));

	// Get VertexInpuDescs
    uint32_t vertex_input_count = graphics::get_vertex_input_desc_from_shader(code, code_length, NULL);
	VertexInputDesc *vertex_input_descs = memory::alloc_temp<VertexInputDesc>(vertex_input_count);
	graphics::get_vertex_input_desc_from_shader(code, code_length, vertex_input_descs);

	// Get VertexShader object
    VertexShader vertex_shader = graphics::get_vertex_shader(&vertex_shader_compiled, vertex_input_descs, 2);
    graphics::release(&vertex_shader_compiled);

	memory::pop_temp_state();
	return vertex_shader;
}


PixelShader graphics::get_pixel_shader_from_code(char *code, uint32_t code_length)
{
	CompiledShader pixel_shader_compiled = graphics::compile_pixel_shader(code, code_length);
	assert(graphics::is_ready(&pixel_shader_compiled));
	
    PixelShader pixel_shader = graphics::get_pixel_shader(&pixel_shader_compiled);
    graphics::release(&pixel_shader_compiled);

	return pixel_shader;
}

ComputeShader graphics::get_compute_shader_from_code(char *code, uint32_t code_length)
{
	CompiledShader compute_shader_compiled = graphics::compile_compute_shader(code, code_length);
	assert(graphics::is_ready(&compute_shader_compiled));
	
    ComputeShader compute_shader = graphics::get_compute_shader(&compute_shader_compiled);
    graphics::release(&compute_shader_compiled);

	return compute_shader;
}