#pragma once
#include "oculus/OVR_CAPI.h"
#include "oculus/OVR_CAPI_D3D.h"
#include "graphics.h"
#include "maths.h"

struct OVRContext
{
    ovrSession session;
    LUID adapter_luid;

    ovrTextureSwapChain swap_chain;
    ID3D11RenderTargetView **rts;
    DepthBuffer depth_buffer;
    int32_t rts_count;

    ovrLayerEyeFov layer;
    ovrVector3f hmd_to_eye[2];
    ovrPosef hand_positions[2];
};
#define OVR_VALID_CONTEXT(context) (context.session)

typedef uint32_t Eye;
typedef uint32_t Hand;
#define EYE_LEFT 0
#define EYE_RIGHT 1
#define HAND_LEFT 0
#define HAND_RIGHT 1

namespace ovr
{
    bool init(LUID **adapter_luid);
    bool init_swap_chain();
    RenderTarget get_current_target_buffer();
    DepthBuffer get_depth_buffer();

    Viewport get_current_viewport(Eye eye);
    Vector3 get_current_eye_position(Eye eye);
    Quaternion get_current_eye_orientation(Eye eye);
    Vector3 get_current_hand_position(Hand hand);
    Quaternion get_current_hand_orientation(Hand hand);
    Matrix4x4 projection_matrix(Eye eye, float near, float far);

    void update_tracking();
    void swap_frames();

    float get_trigger_state(Hand hand);
    
    void release();
}

