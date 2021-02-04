#pragma once
#include "maths.h"

namespace exports
{
    bool export_to_obj(char *filename, Vector4 *vertices, uint32_t vertex_count, uint32_t vertex_stride, uint16_t *indices, uint32_t index_count);
}