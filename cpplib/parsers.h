#pragma once
#include "graphics.h"
#include "file_system.h"
#include "resources.h"

struct StackAllocator;

struct MeshData
{
	float *vertices;
	uint16_t *indices;
	uint32_t index_count;
	uint32_t vertex_count;
	uint32_t vertex_stride;
};

namespace parsers
{
	MeshData get_mesh_from_obj(File obj_file, StackAllocator *stack_allocator);
	AssetDatabase get_assets_db_from_adf(File adfFile, StackAllocator *stack_allocator);
}