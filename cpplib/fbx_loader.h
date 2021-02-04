#include <cassert>
#include "array.h"
#include "maths.h"
struct Animation;
struct Skeleton;

struct MeshData
{
    Vector4  *vertices;
    uint16_t *indices;
    uint32_t vertex_count;
    uint32_t index_count;
};

namespace fbx
{
    MeshData get_mesh(char *fbx_filename);
    MeshData get_animated_mesh(char *fbx_filename, Skeleton *skeleton, Animation **animations, uint32_t *animation_count);
}


