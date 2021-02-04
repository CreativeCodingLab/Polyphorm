#include "fbx_loader.h"
#include "../fbx/fbxsdk.h"
#include "memory.h"
#include "animation.h"

// TODO: faster!
Matrix4x4 get_my_matrix(FbxAMatrix matrix)
{
    Matrix4x4 result;

    result[0]  = (float)matrix.Get(0,0);
    result[1]  = (float)matrix.Get(0,1);
    result[2]  = (float)matrix.Get(0,2);
    result[3]  = (float)matrix.Get(0,3);
    result[4]  = (float)matrix.Get(1,0);
    result[5]  = (float)matrix.Get(1,1);
    result[6]  = (float)matrix.Get(1,2);
    result[7]  = (float)matrix.Get(1,3);
    result[8]  = (float)matrix.Get(2,0);
    result[9]  = (float)matrix.Get(2,1);
    result[10] = (float)matrix.Get(2,2);
    result[11] = (float)matrix.Get(2,3);
    result[12] = (float)matrix.Get(3,0);
    result[13] = (float)matrix.Get(3,1);
    result[14] = (float)matrix.Get(3,2);
    result[15] = (float)matrix.Get(3,3);

    return result;
}

Transform get_transform(FbxAMatrix matrix)
{
    FbxVector4 fbx_scale     = matrix.GetS();
    FbxVector4 fbx_translate = matrix.GetT();
    FbxQuaternion fbx_rotate = matrix.GetQ();

    Transform transform = {};

    transform.rotation[0] = (float)fbx_rotate[0];
    transform.rotation[1] = (float)fbx_rotate[1];
    transform.rotation[2] = (float)fbx_rotate[2];
    transform.rotation[3] = (float)fbx_rotate[3];
    transform.scale[0] = (float)fbx_scale[0];
    transform.scale[1] = (float)fbx_scale[1];
    transform.scale[2] = (float)fbx_scale[2];
    transform.translation[0] = (float)fbx_translate[0];
    transform.translation[1] = (float)fbx_translate[1];
    transform.translation[2] = (float)fbx_translate[2];

    return transform;
}

FbxMesh *get_mesh_node(FbxNode *root_node)
{
    if(root_node) 
    {
        for(int i = 0; i < root_node->GetChildCount(); i++)
        {
            FbxNode *child = root_node->GetChild(i);
            
            FbxNodeAttribute *attribute =  child->GetNodeAttribute();
            if(attribute && attribute->GetAttributeType() && attribute->GetAttributeType() == FbxNodeAttribute::eMesh)
            {
                return (FbxMesh *)attribute;
            }
        }
    }
    return NULL;
}

void get_skeleton_recursive(FbxNode *node, int32_t parent_index, Skeleton *skeleton)
{
    int32_t current_index = parent_index;
    if(node->GetNodeAttribute() && node->GetNodeAttribute()->GetAttributeType() &&
       node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
    {
        Bone bone;
        bone.name = node->GetName();
        bone.parent_idx = parent_index;
        current_index = skeleton->bones.count;
        array::add(&skeleton->bones, bone);
    }
    for (int child_index = 0; child_index < node->GetChildCount(); ++child_index)
	{
		FbxNode *child = node->GetChild(child_index);
		get_skeleton_recursive(child, current_index, skeleton);
	}
}

int32_t get_link_index_from_name(Skeleton *skeleton, char *name)
{
    for (uint32_t i = 0; i < skeleton->bones.count; ++i)
    {
        if(strcmp(name, skeleton->bones.data[i].name) == 0)
        {
            return i; 
        }
    }
    return -1;
}

static FbxManager *init_fbx(char *filename, FbxScene **scene)
{
    FbxManager *sdk_manager = FbxManager::Create();
    FbxIOSettings *ios = FbxIOSettings::Create(sdk_manager, IOSROOT);
    //ios->SetBoolProp(IMP_FBX_ANIMATION, true);
    sdk_manager->SetIOSettings(ios);
    FbxImporter *importer = FbxImporter::Create(sdk_manager,"");
    
    if(!importer->Initialize(filename, -1, sdk_manager->GetIOSettings())) 
    {
        printf("Call to FbxImporter::Initialize() failed.\n"); 
        printf("Error returned: %s\n\n", importer->GetStatus().GetErrorString()); 
        return NULL;
    } 
    
    *scene = FbxScene::Create(sdk_manager,"myScene");
    importer->Import(*scene);
    importer->Destroy();

    return sdk_manager;
}

// TODO: maybe move to logging
bool get_mesh(FbxScene *scene, Vector4 **vertices, uint16_t **indices, uint32_t *count, int32_t **vertex_to_control_point)
{
    FbxNode *root_node = scene->GetRootNode();
    if(!root_node) return false;

    bool found_correct_mesh = false;
    for(int i = 0; i < root_node->GetChildCount(); i++)
    {
        FbxNode *child = root_node->GetChild(i);
        
        FbxNodeAttribute *attribute =  child->GetNodeAttribute();
        if(!attribute) continue;
        if(attribute->GetAttributeType() != FbxNodeAttribute::eMesh) continue;

        FbxMesh                  *mesh         = (FbxMesh *)attribute;
        FbxVector4               *fbx_vertices = mesh->GetControlPoints();
        FbxGeometryElementNormal *fbx_normals  = mesh->GetElementNormal(0);
        if (!(fbx_normals->GetReferenceMode() == FbxGeometryElement::eDirect &&
                fbx_normals->GetMappingMode() == FbxGeometryElement::eByPolygonVertex))
        {
            printf("Found a mesh with incompatible normal mapping!\n");
            continue;
        }
        found_correct_mesh = true;

        uint32_t vertex_count  = mesh->GetControlPointsCount();
        uint32_t polygon_count = mesh->GetPolygonCount();
        *count = polygon_count * 3;
        
        *vertices = memory::alloc_heap<Vector4>(polygon_count * 3 * 2);
        *indices  = memory::alloc_heap<uint16_t>(polygon_count * 3);
        if(vertex_to_control_point) 
            *vertex_to_control_point = memory::alloc_heap<int32_t>(polygon_count * 3);
        
        uint32_t index_count = 0;
        for(uint32_t p = 0; p < polygon_count; ++p)
        {
            uint32_t polygon_size = mesh->GetPolygonSize(p);
            if(polygon_size != 3) 
            {
                printf("Found a polygon that is not a triangle!\n");
                continue;
            }
            for (uint32_t v = 0; v < 3; ++v)
            {
                uint32_t control_point_index = mesh->GetPolygonVertex(p, v);
                if(vertex_to_control_point)
                    (*vertex_to_control_point)[index_count] = control_point_index;

                FbxVector4 vertex = fbx_vertices[control_point_index];
                (*vertices)[index_count * 2].x = (float)vertex[0];
                (*vertices)[index_count * 2].y = (float)vertex[1];
                (*vertices)[index_count * 2].z = (float)vertex[2];
                (*vertices)[index_count * 2].w = 1.0f;

                FbxVector4 normal = fbx_normals->GetDirectArray().GetAt(index_count);
                (*vertices)[index_count * 2 + 1].x = (float)normal[0];
                (*vertices)[index_count * 2 + 1].y = (float)normal[1];
                (*vertices)[index_count * 2 + 1].z = (float)normal[2];
                (*vertices)[index_count * 2 + 1].w = 0.0f;

                (*indices)[index_count] = index_count;
                ++index_count;
            }
            
        }
        
        if(found_correct_mesh) break;
    }
    return found_correct_mesh;
}

bool get_skeleton(FbxScene *scene, Skeleton *skeleton, Animation **animations, uint32_t *animation_count)
{
    FbxNode *root_node = scene->GetRootNode();

    // Get skeleton
    array::init(&skeleton->bones, 50);
    for (int child_index = 0; child_index < root_node->GetChildCount(); ++child_index)
	{
		FbxNode *node = root_node->GetChild(child_index);
		get_skeleton_recursive(node, -1, skeleton);
	}

    // Get animations
    int32_t anim_stack_count = scene->GetSrcObjectCount<FbxAnimStack>();
    if(anim_stack_count < 1)
    {
        return false;
    }

    *animation_count = anim_stack_count;
    *animations = memory::alloc_heap<Animation>(anim_stack_count);
    for (int32_t a = 0 ; a < anim_stack_count; ++a)
    {
        FbxAnimStack *anim_stack = scene->GetSrcObject<FbxAnimStack>(a);
        scene->SetCurrentAnimationStack(anim_stack);

        Animation *animation = &(*animations)[a];
        
        double step_duration = 1.0f / 30.0f;
        double animation_duration = anim_stack->GetLocalTimeSpan().GetDuration().GetSecondDouble();
        uint32_t frame_count = (uint32_t)math::floor((float)(animation_duration / step_duration)) + 1;

        animation->bone_count = skeleton->bones.count;
        animation->frame_count = frame_count;
        animation->duration = (float)animation_duration;
        animation->name = (char *)anim_stack->GetName();
        animation->fps = 30;

        animation->key_frames = memory::alloc_heap<KeyFrame>(animation->frame_count);
        for(uint32_t i = 0; i < animation->frame_count; ++i)
        {
            animation->key_frames[i].bone_transforms = memory::alloc_heap<Transform>(animation->bone_count);
            for(uint32_t b = 0; b < animation->bone_count; ++b)
            {
                animation->key_frames[i].bone_transforms[b].scale = Vector3(1,1,1);
                animation->key_frames[i].bone_transforms[b].translation = Vector3(0,0,0);
                animation->key_frames[i].bone_transforms[b].rotation = Quaternion(0,0,0,1);
            }
        }

        FbxMesh *mesh = get_mesh_node(root_node);

        assert(mesh->GetDeformerCount() == 1);

        FbxSkin *skin = (FbxSkin *)mesh->GetDeformer(0, FbxDeformer::eSkin);
        assert(skin);

        skeleton->control_point_count = mesh->GetControlPointsCount();
        skeleton->control_points = memory::alloc_heap<ControlPoint>(skeleton->control_point_count);
        memset(skeleton->control_points, 0, sizeof(ControlPoint) * skeleton->control_point_count);

        for(int32_t i = 0; i < skin->GetClusterCount(); ++i)
        {
            FbxCluster* cluster = skin->GetCluster(i);
            FbxNode *link_node = cluster->GetLink();
            int32_t link_index = get_link_index_from_name(skeleton, (char *)link_node->GetName());
            assert(link_index >= 0);

            FbxAMatrix world_from_mesh, world_from_bone;
            cluster->GetTransformMatrix(world_from_mesh);
            cluster->GetTransformLinkMatrix(world_from_bone);
            FbxAMatrix mesh_from_bone = world_from_mesh.Inverse() * world_from_bone;

            skeleton->bones.data[link_index].world_from_bind_pose = get_my_matrix(world_from_bone);
            skeleton->bones.data[link_index].bind_pose_from_world = get_my_matrix(mesh_from_bone.Inverse());

            uint32_t num_control_point_indices = cluster->GetControlPointIndicesCount();
            int32_t *control_point_indices = cluster->GetControlPointIndices();
            double *control_point_weights = cluster->GetControlPointWeights();
            for(uint32_t c = 0; c < num_control_point_indices; ++c)
            {
                int32_t control_point_index = control_point_indices[c];
                double control_point_weight = control_point_weights[c];
                ControlPoint *control_point = &skeleton->control_points[control_point_index];

                if (control_point->bone_count < 4)
                {
                    control_point->bone_weights[control_point->bone_count] = (float)control_point_weight;
                    control_point->bone_indices[control_point->bone_count] = link_index;
                    
                    control_point->bone_count++;
                }
                else
                {
                    for(uint32_t o = 0; o < 4; ++o)
                    {
                        if(control_point_weight < control_point->bone_weights[o]) continue;
                        control_point->bone_weights[o] = (float)control_point_weight;
                        control_point->bone_indices[o] = link_index;
                    }
                }
            }

            for(uint32_t frame = 0; frame < frame_count; ++frame)
            {
                FbxTime current_time;
                current_time.SetSecondDouble(step_duration * frame);

                FbxAMatrix local_transform = link_node->EvaluateLocalTransform(current_time);
                if(skeleton->bones.data[link_index].parent_idx == -1)
                {
                    local_transform = link_node->EvaluateGlobalTransform(current_time);
                }
                animation->key_frames[frame].bone_transforms[link_index] = get_transform(local_transform);
                animation->key_frames[frame].bone_transforms[link_index].matrix = get_my_matrix(local_transform);
            }
        }
    }
    return true;
}

MeshData fbx::get_mesh(char *filename)
{
    MeshData mesh_data = {};
    
    FbxScene *scene = NULL;
    FbxManager *sdk_manager = init_fbx(filename, &scene);
    if (!scene || !sdk_manager)
    {
        printf("Failed to initialize scene in fbx file %s.\n", filename);
        return mesh_data;
    }

    uint32_t mesh_element_count;
    bool success = get_mesh(scene, &mesh_data.vertices, &mesh_data.indices, &mesh_element_count, NULL);
    if (!success)
    {
        printf("Failed to get mesh from file file %s.\n", filename);
        return mesh_data;
    } 
    mesh_data.vertex_count = mesh_element_count;
    mesh_data.index_count  = mesh_element_count;

    sdk_manager->Destroy();

    return mesh_data;
}

MeshData fbx::get_animated_mesh(char *filename, Skeleton *skeleton, Animation **animations, uint32_t *animation_count)
{
    MeshData mesh_data  = {};
    
    FbxScene *scene = NULL;
    FbxManager *sdk_manager = init_fbx(filename, &scene);
    if (!scene || !sdk_manager)
    {
        printf("Failed to initialize scene in fbx file %s.\n", filename);
        return mesh_data;
    }

    uint32_t mesh_element_count;
    bool success = get_mesh(scene, &mesh_data.vertices, &mesh_data.indices, &mesh_element_count, &skeleton->vertex_to_control_point);
    if (!success)
    {
        printf("Failed to get mesh from file file %s.\n", filename);
        return mesh_data;
    } 
    mesh_data.vertex_count = mesh_element_count;
    mesh_data.index_count  = mesh_element_count;


    success = get_skeleton(scene, skeleton, animations, animation_count);
    if (!success)
    {
        printf("Failed to get skeleton from file file %s.\n", filename);
        return mesh_data;
    } 

    // TODO: separate vertex buffers for animation data?
    float *animated_mesh_data = memory::alloc_heap<float>(mesh_element_count * 16);
    Vector4 *vertices = (Vector4 *)animated_mesh_data;
    Vector4 *source_data = mesh_data.vertices;
    for(uint32_t i = 0; i < mesh_element_count; ++i)
    {
        *(vertices++) = *(source_data++);
        *(vertices++) = *(source_data++);
        ControlPoint ctrl_point = skeleton->control_points[skeleton->vertex_to_control_point[i]];
        *(vertices++) = *((Vector4 *) &ctrl_point.bone_weights);
        int32_t *bone_indices = (int32_t *)vertices;
        *(bone_indices++) = ctrl_point.bone_indices[0];
        *(bone_indices++) = ctrl_point.bone_indices[1];
        *(bone_indices++) = ctrl_point.bone_indices[2];
        *(bone_indices++) = ctrl_point.bone_indices[3];
        vertices = (Vector4 *)bone_indices;
    }
    memory::free_heap(mesh_data.vertices);
    mesh_data.vertices = (Vector4 *)animated_mesh_data;

    sdk_manager->Destroy();
    return mesh_data;
}




