#pragma once
#include "array.h"
#include "maths.h"

struct Bone
{
    const char *name;
    int32_t parent_idx;

    Matrix4x4 world_from_bind_pose;
    Matrix4x4 bind_pose_from_world;
};

struct ControlPoint
{
    int32_t bone_indices[4];
    float  bone_weights[4];

    uint32_t bone_count = 0;
};

struct Skeleton
{
    Array<Bone> bones;

    ControlPoint *control_points;
    uint32_t      control_point_count;
    int32_t      *vertex_to_control_point;
    uint32_t      vertex_count;
};

struct Transform
{
    Quaternion rotation;
    Vector3 scale;
    Vector3 translation;
    Matrix4x4 matrix;
};

struct KeyFrame
{
    Transform *bone_transforms;
};

struct Animation
{
    KeyFrame   *key_frames;
    uint32_t    frame_count;  
    uint32_t    bone_count;

    char *name;
    float duration;
    uint32_t fps;
};

struct AnimationPlayer
{
    Skeleton skeleton;
    Animation *animations;
    uint32_t animation_count;

    uint32_t current_animation = 0;
    float current_time = 0;

    Matrix4x4 *output_matrices;
};

namespace animation
{
    AnimationPlayer get(Skeleton skeleton, Animation *animations, uint32_t animation_count);
    void update(AnimationPlayer *player, float dt);
    void evaluate(AnimationPlayer *player);
}