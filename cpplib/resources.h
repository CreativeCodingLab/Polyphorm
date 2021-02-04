#pragma once
#include <stdint.h>

struct Sound;

struct VertexShader;
struct PixelShader;
struct GeometryShader;
struct Mesh;

typedef uint32_t sid;

#define EXPAND(x) x
#define HASH1(string) assert(false)
#define HASH2(string, hash) hash

#define SELECTOR(_1, _2, C, ...) C
#define HASH(...) EXPAND(SELECTOR(__VA_ARGS__, HASH2, HASH1)(__VA_ARGS__))

struct AssetInfo
{
	char *path;
	uint32_t type;
};

const uint32_t ASSET_MAX_PATH_LENGTH= 50;
const uint32_t ASSET_MAX_NAME_LENGTH = 50;
const uint32_t ASSET_MAX_TYPE_LENGTH = 20;

#define NONE 0
#define MESH 1
#define VERTEX_SHADER 2
#define PIXEL_SHADER 3
#define GEOMETRY_SHADER 4
#define AUDIO_OGG 5
#define FONT 6

struct AssetDatabase
{
	uint32_t asset_count;

	sid *keys;
	AssetInfo *asset_infos;
};

namespace resources
{
	void init(char *adf_file);
	void load_all();
	void release_all();

	Mesh *get_mesh(sid id);
	VertexShader *get_vertex_shader(sid id);
	PixelShader *get_pixel_shader(sid id);
	GeometryShader *get_geometry_shader(sid id);
	Sound *get_sound(sid id);
	uint8_t *get_font_data(sid id);
}