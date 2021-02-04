#include <stdio.h>
#include "parsers.h"
#include "memory.h"
#include <assert.h>

#ifdef DEBUG
#include<stdio.h>
#define PRINT_DEBUG(message, ...) {printf("ERROR in file %s on line %d: ", __FILE__, __LINE__); printf(message, __VA_ARGS__); printf("\n");}
#else
#define PRINT_DEBUG(message, ...)
#endif

#define MIN(a, b) (a < b ? a : b)
// TODO: Restructure parsers as lexer + synth
// TODO: Better whitespace checking + better end of line checking (unify \r\n and \n)

bool parse_uint16(char *c, uint16_t *num)
{
	static uint16_t current_number = 0;
	static bool parsing = false;

	bool ended_parsing = false;

	if (isdigit(*c))
	{
		int16_t num = *c - '0';
		current_number = current_number * 10 + num;
		parsing = true;
	}
	else if (parsing)
	{
		*num = current_number;

		current_number = 0;
		parsing = false;
		ended_parsing = true;
	}
	if (*c == '/' && !parsing)
	{
		parsing = true;
	}

	return ended_parsing;
}

bool parse_float(char *c, float *num)
{
	static float current_number = 0;
	static float fraction = -1.0f;
	static float sign = 1.0f;
	static bool parsing = false;

	bool ended_parsing = false;

	if (isdigit(*c))
	{
		int16_t num = *c - '0';
		if (fraction <= 0.0f)
		{
			current_number = 10.0f * current_number + (float)num;
			fraction = 0.0f;
		}
		else
		{
			current_number = current_number + (float)num / fraction;
			fraction *= 10.0f;
		}
		parsing = true;
	}
	else if (*c == '.')
	{
		fraction = 10.0f;
	}
	else if (*c == '-' && !parsing)
	{
		sign = -1.0f;
		parsing = true;
	}
	else if (parsing)
	{
		*num = current_number * sign;

		current_number = 0;
		fraction = -1.0f;
		sign = 1.0f;
		parsing = false;
		ended_parsing = true;
	}

	return ended_parsing;
}

template <typename T>
bool parse_n_numbers(char *c, T *storage, uint32_t number, bool parsing_function(char *, T *))
{
	static uint32_t count = 0;
	bool finished = false;

	if (parsing_function(c, storage + count))
	{
		count++;
		if (count == number)
		{
			finished = true;
			count = 0;
		}
	}

	return finished;
}

enum OBJParserState
{
	START_OF_LINE,
	PARSE_POSITION,
	PARSE_NORMAL,
	PARSE_TEXCOORD,
	PARSE_FACE,
	GO_TO_NEXT_LINE
};

MeshData parsers::get_mesh_from_obj(File file, StackAllocator *allocator)
{
	char *file_start = (char *)file.data;
	uint32_t size = file.size;

	OBJParserState state = START_OF_LINE;

	uint32_t position_count = 0;
	uint32_t normal_count = 0;
	uint32_t texcoord_count = 0;
	uint32_t index_count = 0;

	char *ptr = 0;
	uint32_t counter = 0;
	for (counter = 0, ptr = file_start; ptr && counter < size; counter++, ptr++)
	{
		switch (state)
		{
		case START_OF_LINE:
		{
			if (strncmp(ptr, "vn", 2) == 0)
			{
				normal_count++;
			}
			else if (strncmp(ptr, "vt", 2) == 0)
			{
				texcoord_count++;
			}
			else if (*ptr == 'v')
			{
				position_count++;
			}
			else if (*ptr == 'f')
			{
				index_count += 3;
			}
			state = GO_TO_NEXT_LINE;
		} break;
		case GO_TO_NEXT_LINE:
		{
			if (*ptr == '\n')
			{
				state = START_OF_LINE;
			}
		} break;
		}
	}

	assert(position_count > 0);
	if (position_count == 0) {
		PRINT_DEBUG("No. positions in OBJ file is 0.");
	}

	uint32_t vertex_stride = 4;
	if (texcoord_count > 0)
	{
		vertex_stride += 2;
	}

	if (normal_count > 0)
	{
		vertex_stride += 4;
	}
	float *vertices = memory::alloc_stack<float>(allocator, index_count * vertex_stride);
	uint16_t *indices = memory::alloc_stack<uint16_t>(allocator, index_count);

	StackAllocatorState stack_state = memory::save_stack_state(allocator);

	// helper_indices store indices for positions/normals/texcoords for each vertex.
	// e.g. if you have f 1/1/1 in obj file, helperIndices will store [1,1,1].
	uint16_t *helper_indices = memory::alloc_stack<uint16_t>(allocator, index_count * 3);
	float *positions = memory::alloc_stack<float>(allocator, position_count * 4);
	float *normals = memory::alloc_stack<float>(allocator, normal_count * 4);
	float *texcoords = memory::alloc_stack<float>(allocator, texcoord_count * 2);

	uint16_t *index_ptr = helper_indices;
	float *position_ptr = positions;
	float *normal_ptr = normals;
	float *texcoord_ptr = texcoords;

	state = START_OF_LINE;
	for (counter = 0, ptr = file_start; ptr && counter < size; counter++, ptr++)
	{
		switch (state)
		{
		case START_OF_LINE:
		{
			if (strncmp(ptr, "vn", 2) == 0)
			{
				state = PARSE_NORMAL;
			}
			else if (strncmp(ptr, "vt", 2) == 0)
			{
				state = PARSE_TEXCOORD;
			}
			else if (*ptr == 'v')
			{
				state = PARSE_POSITION;
			}
			else if (*ptr == 'f')
			{
				state = PARSE_FACE;
			}
		} break;
		case PARSE_FACE:
		{
			if (parse_n_numbers<uint16_t>(ptr, index_ptr, 9, parse_uint16))
			{
				index_ptr += 9;
				state = GO_TO_NEXT_LINE;
			}
		} break;
		case PARSE_POSITION:
		{
			if (parse_n_numbers<float>(ptr, position_ptr, 4, parse_float))
			{
				position_ptr += 4;
				state = GO_TO_NEXT_LINE;
			}
		} break;
		case PARSE_NORMAL:
		{
			if (parse_n_numbers<float>(ptr, normal_ptr, 4, parse_float))
			{
				normal_ptr += 4;
				state = GO_TO_NEXT_LINE;
			}
		} break;
		case PARSE_TEXCOORD:
		{
			if (parse_n_numbers<float>(ptr, texcoord_ptr, 2, parse_float))
			{
				texcoord_ptr += 2;
				state = GO_TO_NEXT_LINE;
			}
		} break;
		}
		if (state == GO_TO_NEXT_LINE)
		{
			if (*ptr == '\n')
			{
				state = START_OF_LINE;
			}
		}
	}

	// TODO: Check if vertex exists, do not duplicate
	float *vertices_ptr = vertices;
	uint16_t *indices_ptr = indices;
	uint16_t *helper_indices_ptr = helper_indices;
	for (uint32_t i = 0; i < index_count; ++i)
	{
		*(indices_ptr++) = i;

		uint16_t position_index = *(helper_indices_ptr) - 1;
		float *position = positions + position_index * 4;
		memcpy(vertices_ptr, position, sizeof(float) * 4);
		vertices_ptr += 4;

		if (texcoord_count > 0)
		{
			uint16_t texcoord_index = *(helper_indices_ptr + 1) - 1;
			float *texcoord = texcoords + texcoord_index * 2;
			memcpy(vertices_ptr, texcoord, sizeof(float) * 2);
			vertices_ptr += 2;
		}

		if (normal_count > 0)
		{
			uint16_t normal_index = *(helper_indices_ptr + 2) - 1;
			float *normal = normals + normal_index * 4;
			memcpy(vertices_ptr, normal, sizeof(float) * 4);
			vertices_ptr += 4;
		}

		helper_indices_ptr += 3;
	}

	MeshData mesh_data;

	mesh_data.vertices = vertices;
	mesh_data.indices = indices;
	mesh_data.vertex_count = index_count;
	mesh_data.index_count = index_count;
	mesh_data.vertex_stride = sizeof(float) * vertex_stride;

	memory::load_stack_state(allocator, stack_state);

	return mesh_data;
}

char *asset_strings[] =
{
	"NONE",
	"MESH",
	"VERTEX_SHADER",
	"PIXEL_SHADER",
	"GEOMETRY_SHADER",
	"AUDIO_OGG",
	"FONT"
};

uint32_t asset_type_from_string(char *string, uint32_t string_length)
{
	uint32_t type = NONE;

	for (uint32_t asset_type = 0; asset_type < ARRAYSIZE(asset_strings); ++asset_type)
	{
		if (strlen(asset_strings[asset_type]) == string_length && strncmp(string, asset_strings[asset_type], string_length) == 0)
		{
			type = asset_type;
			break;
		}
	}

	return type;
}

AssetDatabase parsers::get_assets_db_from_adf(File adf_file, StackAllocator *stack_allocator)
{
	enum State
	{
		PARSING_NAME,
		PARSING_PATH,
		PARSING_TYPE
	};

	char *c = (char *)adf_file.data;
	State state = PARSING_NAME;

	uint32_t asset_count = 0;
	for (uint32_t i = 0; i < adf_file.size; ++i, ++c)
	{
		// Increase asset count in case there's end of line or end of file, AND you're not on a blank line
		if ((*c == '\n' || i == adf_file.size - 1) && *(c - 2) != '\n')
		{
			asset_count++;
		}
	}

	PRINT_DEBUG("Asset count: %d", asset_count);

	sid *keys = memory::alloc_stack<sid>(stack_allocator, asset_count);
	AssetInfo *infos = memory::alloc_stack<AssetInfo>(stack_allocator, asset_count);
	for (uint32_t i = 0; i < asset_count; ++i)
	{
		infos[i].path = memory::alloc_stack<char>(stack_allocator, ASSET_MAX_PATH_LENGTH);
	}

	c = (char *)adf_file.data;
	uint32_t asset_counter = 0;
	for (uint32_t i = 0; i < adf_file.size; ++i, ++c)
	{
		switch (state)
		{
		case PARSING_NAME:
		{
			static uint32_t name_length = 0;
			if (*c == '\r' || *c == '\n')
			{
				if (name_length == 0) {
					// OK, just an empty line
				} else {
					PRINT_DEBUG("Incorrect ADF format! End of line after asset name.");
				}
			}
			else if (*c == ' ' && *(c - 1) == ')')
			{
				char *name_start = c - 11;
				name_length = 10;
				char name_buffer[ASSET_MAX_NAME_LENGTH];
				if (name_length > ASSET_MAX_NAME_LENGTH - 1) // Have to account for 0 terminating character
				{
					PRINT_DEBUG("Name length read from ADF too long!");
					name_length = ASSET_MAX_NAME_LENGTH - 1;
				}
				memcpy(name_buffer, name_start, MIN(name_length, ASSET_MAX_NAME_LENGTH - 1));
				name_buffer[name_length] = 0;
				keys[asset_counter] = strtoul(name_buffer, NULL, 16);
				state = PARSING_PATH;
				name_length = 0;
			}
			else
			{
				name_length++;
			}
		}
		break;
		case PARSING_PATH:
		{
			static uint32_t path_length = 0;
			if (*c == '\r' || *c == '\n') {
				PRINT_DEBUG("Incorrect ADF format! End of line before asset type read.");
			}
			else if (*c == ' ')
			{
				char *path_start = c - path_length;
				if (path_length > ASSET_MAX_PATH_LENGTH - 1) // Have to account for 0 terminating character
				{
					PRINT_DEBUG("Path read from ADF too long!");
					path_length = ASSET_MAX_PATH_LENGTH - 1;
				}
				memcpy(infos[asset_counter].path, path_start, path_length);
				infos[asset_counter].path[path_length] = 0;

				path_length = 0;
				state = PARSING_TYPE;
			}
			else
			{
				path_length++;
			}
		}
		break;
		case PARSING_TYPE:
		{
			static uint32_t type_length = 0;
			if (*c == '\r' || *c == '\n' || i == adf_file.size - 1)
			{
				char *type_start = c - type_length;
				if (i == adf_file.size - 1)
				{
					type_length++;
				}
				infos[asset_counter].type = asset_type_from_string(type_start, type_length);

				type_length = 0;
				state = PARSING_NAME;

				type_length++;
			}
			else
			{
				type_length++;
			}
		}
		break;
		}
	}

	AssetDatabase result = {};

	result.asset_count = asset_count;
	result.keys = keys;
	result.asset_infos = infos;

	return result;
}

