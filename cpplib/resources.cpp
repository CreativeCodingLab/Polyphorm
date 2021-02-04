#include "resources.h"
#include "file_system.h"
#include "memory.h"
#include "audio.h"
#include "graphics.h"
#include "parsers.h"
#include "font.h"

#include <cstdlib>

#ifdef DEBUG
#include<stdio.h>
#define PRINT_DEBUG(message, ...) {printf("ERROR in file %s on line %d: ", __FILE__, __LINE__); printf(message, __VA_ARGS__); printf("\n");}
#else
#define PRINT_DEBUG(message, ...)
#endif


template <typename T, uint32_t N>
struct Table
{
	sid sids[N];
	T items[N];
};

template <uint32_t N>
uint32_t hash_to_n(uint32_t num)
{
	// NOTE: maybe it's worth it to restrict Table to only have no. items
	// be a power of 2. In that case, hashing amounts to taking N lowest bits
	// as a value (bitwise AND).
	uint32_t result = (uint32_t)num % N;
	return result;
}

#define SID_TEMPLATE "%#010x"

namespace table
{
	// TODO: This will loop infintely if item doesn't exist.
	template<typename T, uint32_t N>
	uint32_t get_slot(Table<T, N> *table, sid id)
	{
		uint32_t hash = hash_to_n<N>(id);

		while (id != table->sids[hash])
		{
			hash = hash_to_n<N>(hash + 1);
		}

		return hash;
	}

	template<typename T, uint32_t N>
	uint32_t get_free_slot(Table<T, N> *table, sid id)
	{
		uint32_t hash = hash_to_n<N>(id);

		while (table->sids[hash] && id != table->sids[hash])
		{
			hash = hash_to_n<N>(hash + 1);
		}

		return hash;
	}

	template <typename T, uint32_t N>
	void add(Table<T, N> *table, sid id, T item)
	{
		uint32_t hash = table::get_free_slot(table, id);

		table->sids[hash] = id;
		table->items[hash] = item;
	}

	template <typename T, uint32_t N>
	void remove(Table<T, N> *table, sid id)
	{
		uint32_t hash = table::get_slot(table, id);

		table->sids[hash] = 0;
	}

	template <typename T, uint32_t N>
	T *get(Table<T, N> *table, sid id)
	{
		uint32_t hash = table::get_slot(table, id);
		T *item = &(table->items[hash]);
		return item;
	}
}

static Table<Mesh, 256> meshes = { 0 };
static Table<VertexShader, 16> vertex_shaders = { 0 };
static Table<PixelShader, 16> pixel_shaders = { 0 };
static Table<GeometryShader, 8> geometry_shaders = { 0 };
static Table<Sound, 16> sounds = { 0 };
static Table<uint8_t *, 16> fonts = { 0 };
static Table<AssetInfo, 512> assets = { 0 };
static Table<bool, 512> asset_load_states = { 0 };

static StackAllocator allocator;
static AssetDatabase asset_db;

void resources::init(char *adf_file)
{
	allocator = memory::get_stack_allocator(MEGABYTES(10));

	File asset_definition_file = file_system::read_file(adf_file);
	if (asset_definition_file.size > 0) {
		asset_db = parsers::get_assets_db_from_adf(asset_definition_file, &allocator);
		for (uint32_t i = 0; i < asset_db.asset_count; ++i)
		{
			table::add(&assets, asset_db.keys[i], asset_db.asset_infos[i]);
			table::add(&asset_load_states, asset_db.keys[i], true);
		}
		file_system::release_file(asset_definition_file);
	} else {
		PRINT_DEBUG("Could not load asset definition file!!!");
	}
}

AssetInfo *get_info_from_db(sid id)
{
	AssetInfo *info = table::get(&assets, id);
	if (info->type == NONE) {
		PRINT_DEBUG("Resource with id %d not found in ADF.", id);
	}

	return info;
}

void process_mesh(AssetInfo mesh_info, sid id)
{
	File mesh_file = file_system::read_file(mesh_info.path);

	if (mesh_file.size == 0) {
		PRINT_DEBUG("Could not read mesh file %s.", mesh_info.path);
		mesh_info = *get_info_from_db(HASH("cube", 0x7c9557c5));
		mesh_file = file_system::read_file(mesh_info.path);
	}

	StackAllocatorState stack_state = memory::save_stack_state(&allocator);

	MeshData mesh_data = parsers::get_mesh_from_obj(mesh_file, &allocator);
	file_system::release_file(mesh_file);

	Mesh mesh = graphics::get_mesh(mesh_data.vertices, mesh_data.vertex_count, mesh_data.vertex_stride,
								   mesh_data.indices, mesh_data.index_count, sizeof(uint16_t));
	table::add(&meshes, id, mesh);

	memory::load_stack_state(&allocator, stack_state);
}

void process_vertex_shader(AssetInfo vertex_shader_info, sid id)
{
	File vertex_shader_file = file_system::read_file(vertex_shader_info.path);
	if (vertex_shader_file.size == 0) {
		PRINT_DEBUG("Could not read vertex shader file %s.", vertex_shader_info.path);
		return;
	}

	uint32_t vertex_input_count = graphics::get_vertex_input_desc_from_shader((char *)vertex_shader_file.data, vertex_shader_file.size, NULL);
	StackAllocatorState allocator_state = memory::save_stack_state(&allocator);
	VertexInputDesc *vertex_input_descs = memory::alloc_stack<VertexInputDesc>(&allocator, vertex_input_count);
	graphics::get_vertex_input_desc_from_shader((char *)vertex_shader_file.data, vertex_shader_file.size, vertex_input_descs);

	PRINT_DEBUG("Compiling vertex shader %s.", vertex_shader_info.path);
	CompiledShader vertex_shader_code = graphics::compile_vertex_shader(vertex_shader_file.data, vertex_shader_file.size);
	file_system::release_file(vertex_shader_file);

	if (vertex_shader_code.blob) {
		VertexShader vertex_shader = graphics::get_vertex_shader(&vertex_shader_code, vertex_input_descs, vertex_input_count);
		graphics::release(&vertex_shader_code);
		table::add(&vertex_shaders, id, vertex_shader);
	} else {
		PRINT_DEBUG("Failed to compile shader %s!", vertex_shader_info.path);
	}

	memory::load_stack_state(&allocator, allocator_state);
}

void process_pixel_shader(AssetInfo pixel_shader_info, sid id)
{
	File pixel_shader_file = file_system::read_file(pixel_shader_info.path);
	if (pixel_shader_file.size == 0) {
		PRINT_DEBUG("Could not read pixel shader file %s.", pixel_shader_info.path);
		return;
	}

	PRINT_DEBUG("Compiling pixel shader %s.", pixel_shader_info.path);
	CompiledShader pixel_shader_code = graphics::compile_pixel_shader(pixel_shader_file.data, pixel_shader_file.size);
	file_system::release_file(pixel_shader_file);

	if (pixel_shader_code.blob) {
		PixelShader pixel_shader = graphics::get_pixel_shader(&pixel_shader_code);
		graphics::release(&pixel_shader_code);
		table::add(&pixel_shaders, id, pixel_shader);
	} else {
		PRINT_DEBUG("Failed to compile shader %s!", pixel_shader_info.path);
	}
}

void process_geometry_shader(AssetInfo geometry_shader_info, sid id)
{
	File geometry_shader_file = file_system::read_file(geometry_shader_info.path);
	if (geometry_shader_file.size == 0) {
		PRINT_DEBUG("Could not read geometry shader file %s.", geometry_shader_info.path);
		return;
	}

	PRINT_DEBUG("Compiling geometry shader %s.", geometry_shader_info.path);
	CompiledShader geometry_shader_code = graphics::compile_geometry_shader(geometry_shader_file.data, geometry_shader_file.size);
	file_system::release_file(geometry_shader_file);

	if (geometry_shader_code.blob)
	{
		GeometryShader geometry_shader = graphics::get_geometry_shader(&geometry_shader_code);
		graphics::release(&geometry_shader_code);
		table::add(&geometry_shaders, id, geometry_shader);
	} else {
		PRINT_DEBUG("Failed to compile shader %s!", geometry_shader_info.path);
	}
}

void process_audio_ogg(AssetInfo ogg_info, sid id)
{
	File ogg_file = file_system::read_file(ogg_info.path);
	if (ogg_file.size == 0) {
		PRINT_DEBUG("Could not read ogg file %s.", ogg_info.path);
		return;
	}

    Sound sound = audio::get_sound_ogg(ogg_file.data, ogg_file.size);
	table::add(&sounds, id, sound);
	file_system::release_file(ogg_file);
}

void process_font(AssetInfo font_info, sid id)
{
	File font_file = file_system::read_file(font_info.path);
	if (font_file.size == 0) {
		PRINT_DEBUG("Could not read otf file %s.", font_info.path);
		return;
	}

	uint8_t *font_data = memory::alloc_heap<uint8_t>(font_file.size);
	memcpy(font_data, font_file.data, font_file.size);

	table::add(&fonts, id, font_data);
	file_system::release_file(font_file);
}

typedef void AssetFunction(AssetInfo asset_info, sid id);

AssetFunction *process_functions[] =
{
	0,
	process_mesh,
	process_vertex_shader,
	process_pixel_shader,
	process_geometry_shader,
	process_audio_ogg,
	process_font
};

void release_mesh(AssetInfo mesh_info, sid id)
{
	Mesh *mesh = table::get(&meshes, id);
	graphics::release(mesh);
	table::remove(&meshes, id);
}

void release_vertex_shader(AssetInfo vertex_shader_info, sid id)
{
	VertexShader *vertex_shader = table::get(&vertex_shaders, id);
	graphics::release(vertex_shader);
	table::remove(&vertex_shaders, id);
}

void release_pixel_shader(AssetInfo pixel_shader_info, sid id)
{
	PixelShader *pixel_shader = table::get(&pixel_shaders, id);
	graphics::release(pixel_shader);
	table::remove(&pixel_shaders, id);
}

void release_geometry_shader(AssetInfo geometry_shader_info, sid id)
{
	GeometryShader *geometry_shader = table::get(&geometry_shaders, id);
	graphics::release(geometry_shader);
	table::remove(&geometry_shaders, id);
}

void release_audio_ogg(AssetInfo ogg_info, sid id)
{
	Sound *ogg_sound = table::get(&sounds, id);
	audio::release(ogg_sound);
	table::remove(&sounds, id);
}

void release_font(AssetInfo ogg_info, sid id)
{
	uint8_t *font_data = *(table::get(&fonts, id));
	memory::free_heap(font_data);
	table::remove(&fonts, id);
}

AssetFunction *release_functions[] =
{
	0,
	release_mesh,
	release_vertex_shader,
	release_pixel_shader,
	release_geometry_shader,
	release_audio_ogg,
	release_font
};

void resources::load_all()
{
	for (uint32_t i = 0; i < asset_db.asset_count; ++i)
	{
		sid id = asset_db.keys[i];
		AssetInfo resource_info = *get_info_from_db(id);
		process_functions[resource_info.type](resource_info, id);
	}
}

void resources::release_all()
{
	for (uint32_t i = 0; i < asset_db.asset_count; ++i)
	{
		sid id = asset_db.keys[i];
		AssetInfo resource_info = *get_info_from_db(id);
		release_functions[resource_info.type](resource_info, id);
		*table::get(&asset_load_states, asset_db.keys[i]) = false;
	}
	int x = 12;
}

Mesh *resources::get_mesh(sid id)
{
	// Check if loaded and if not, fallback to cube mesh
	bool loaded = *table::get(&asset_load_states, id);
	if (!loaded) {
		id = HASH("cube", 0x7c9557c5);
		PRINT_DEBUG("Requested mesh not loaded: " SID_TEMPLATE, id);
	}

	Mesh *mesh = table::get(&meshes, id);
	return mesh;
}

VertexShader *resources::get_vertex_shader(sid id)
{
	bool loaded = *table::get(&asset_load_states, id);
	if (!loaded) {
		PRINT_DEBUG("Requested vertex shader not loaded: " SID_TEMPLATE, id);
	}

	VertexShader *vertex_shader = table::get(&vertex_shaders, id);
	return vertex_shader;
}

PixelShader *resources::get_pixel_shader(sid id)
{
	bool loaded = *table::get(&asset_load_states, id);
	if (!loaded) {
		PRINT_DEBUG("Requested vertex shader not loaded: " SID_TEMPLATE, id);
	}

	PixelShader *pixel_shader = table::get(&pixel_shaders, id);
	return pixel_shader;
}

GeometryShader *resources::get_geometry_shader(sid id)
{
	bool loaded = *table::get(&asset_load_states, id);
	if (!loaded) {
		PRINT_DEBUG("Requested vertex shader not loaded: " SID_TEMPLATE, id);
	}

	GeometryShader *geometry_shader = table::get(&geometry_shaders, id);
	return geometry_shader;
}

Sound *resources::get_sound(sid id)
{
	bool loaded = *table::get(&asset_load_states, id);
	if (!loaded) {
		PRINT_DEBUG("Requested sound not loaded: " SID_TEMPLATE, id);
	}
	Sound *sound = table::get(&sounds, id);
	return sound;
}

uint8_t *resources::get_font_data(sid id)
{
	bool loaded = *table::get(&asset_load_states, id);
	if (!loaded) {
		PRINT_DEBUG("Requested font not loaded: " SID_TEMPLATE, id);
	}
	uint8_t *font_data = *(table::get(&fonts, id));
	return font_data;
}