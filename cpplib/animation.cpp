#include "animation.h"
#include "memory.h"

Matrix4x4 matrix_from_transform(Transform transform)
{
    Matrix4x4 scale_matrix = math::get_scale(transform.scale);
    Matrix4x4 translation_matrix = math::get_translation(transform.translation);
    Matrix4x4 rotation_matrix = math::get_rotation(transform.rotation);

    Matrix4x4 result = translation_matrix * rotation_matrix * scale_matrix;
    return result;
}

Transform interpolate(Transform t1, Transform t2, float t)
{
    Transform result = {};
    result.scale = math::lerp(t1.scale, t2.scale, t);
    result.translation = math::lerp(t1.translation, t2.translation, t);
    if (math::dot(t1.rotation, t2.rotation) < 0.0f)
    {
        result.rotation = math::nlerp(-t1.rotation, t2.rotation, t);
    }
    else
    {
        result.rotation = math::nlerp(t1.rotation, t2.rotation, t);
    }
    return result;
}

AnimationPlayer animation::get(Skeleton skeleton, Animation *animations, uint32_t animation_count)
{
    AnimationPlayer player = {};

    player.skeleton = skeleton;
    player.animations = animations;
    player.animation_count = animation_count;
    player.output_matrices = memory::alloc_heap<Matrix4x4>(skeleton.bones.count);

    return player;
}

void animation::update(AnimationPlayer *player, float dt)
{
    player->current_time += dt;
    Animation *current_animation = &player->animations[player->current_animation];
    if(player->current_time >= current_animation->duration)
    {
        player->current_time = 0.0f;
        player->current_animation = 1 - player->current_animation;
    }
}

void animation::evaluate(AnimationPlayer *player)
{
    Animation *animation = &player->animations[player->current_animation];
    Skeleton *skeleton = &player->skeleton;
    float current_part = player->current_time / animation->duration * (animation->frame_count - 1);
    uint32_t current_frame = (uint32_t)math::floor(current_part);
    float fraction = current_part - (float)current_frame;
    current_frame = math::clamp(current_frame, (uint32_t)0, animation->frame_count - 1);

    for (uint32_t i = 0; i < skeleton->bones.count; ++i)
    {
        Transform local_transform_current = animation->key_frames[current_frame].bone_transforms[i];
        Transform local_transform_next = animation->key_frames[current_frame + 1].bone_transforms[i];
        Transform local_transform = interpolate(local_transform_current, local_transform_next, fraction);
        Matrix4x4 local_mat = matrix_from_transform(local_transform);
        Matrix4x4 local_mat2 = local_transform.matrix;
        int32_t parent_index = skeleton->bones.data[i].parent_idx;
        Matrix4x4 parent_mat = math::get_identity();
        if(parent_index >= 0)
        {
            parent_mat = player->output_matrices[parent_index];
        }
        player->output_matrices[i] = parent_mat * local_mat;
    }
    for (uint32_t i = 0; i < skeleton->bones.count; ++i)
    {
        Bone *bone = &skeleton->bones.data[i];
        Matrix4x4 mat = bone->world_from_bind_pose;
        player->output_matrices[i] = player->output_matrices[i] * bone->bind_pose_from_world;
        //player->output_matrices[i] = bone->world_from_bind_pose * bone->bind_pose_from_world;
    }
}