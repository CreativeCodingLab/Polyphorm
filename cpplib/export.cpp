#include <stdio.h>
#include <string.h>
#include "export.h"
#include "file_system.h"
#include "memory.h"
#ifdef DEBUG
#include<stdio.h>
#define PRINT_DEBUG(message, ...) {printf("ERROR in file %s on line %d: ", __FILE__, __LINE__); printf(message, __VA_ARGS__); printf("\n");}
#else
#define PRINT_DEBUG(message, ...)
#endif

#define print_to_string sprintf_s

bool exports::export_to_obj(char *filename, Vector4 *vertices, uint32_t vertex_count, uint32_t vertex_stride, uint16_t *indices, uint32_t index_count)
{
    uint32_t number_of_lines = vertex_count * 2 + index_count / 3;
    uint32_t max_line_length = 50; // Should suffice for +-10000

    uint32_t total_file_size = number_of_lines * max_line_length;

    char *file_memory = memory::alloc_heap<char>(total_file_size);
    char *file_ptr = file_memory;

    int32_t size_left = total_file_size;
    for (uint32_t i = 0; i < vertex_count; ++i)
    {
        Vector4 vertex_position = vertices[i * 2];
        uint32_t bytes_written = print_to_string(file_ptr, size_left, "v %.4f %.4f %.4f %.4f\n", vertex_position.x, vertex_position.y, vertex_position.z, vertex_position.w);
        if (bytes_written > max_line_length)
        {
            PRINT_DEBUG("export_to_obj: expected maximum output line length was %d, actual was %d. This may or may not cause errors.", max_line_length, bytes_written);
        }
        file_ptr += bytes_written;
        size_left -= bytes_written;
        if (size_left <= 0)
        {
            PRINT_DEBUG("Buffer overflow while exporting to OBJ!");
        }
    }
    for (uint32_t i = 0; i < vertex_count; ++i)
    {
        Vector4 vertex_normal = vertices[i * 2 + 1];
        uint32_t bytes_written = print_to_string(file_ptr, size_left, "vn %.4f %.4f %.4f %.4f\n", vertex_normal.x, vertex_normal.y, vertex_normal.z, vertex_normal.w);
        file_ptr += bytes_written;
        size_left -= bytes_written;
        if (size_left <= 0)
        {
            PRINT_DEBUG("Buffer overflow while exporting to OBJ!");
        }
    }

    for (uint32_t i = 0; i < index_count; i += 3)
    {
        uint16_t i1 = indices[i] + 1;
        uint16_t i2 = indices[i + 1] + 1;
        uint16_t i3 = indices[i + 2] + 1;
        uint32_t bytes_written = print_to_string(file_ptr, size_left, "f %d//%d %d//%d %d//%d\n", i1, i1, i2, i2, i3, i3);
        file_ptr += bytes_written;
        size_left -= bytes_written;
        if (size_left <= 0)
        {
            PRINT_DEBUG("Buffer overflow while exporting to OBJ!");
        }
    }

    if (!file_system::write_file(filename, file_memory, total_file_size - size_left))
    {
        PRINT_DEBUG("Could not write mesh to %s.", filename);
        return false;
    }
    memory::free_heap(file_memory);
    return true;
}