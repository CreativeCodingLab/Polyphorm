#include "ovr.h"
#include "memory.h"
#ifdef DEBUG
#include<stdio.h>
#define PRINT_DEBUG(message, ...) {printf("ERROR in file %s on line %d: ", __FILE__, __LINE__); printf(message, __VA_ARGS__); printf("\n");}
#else
#define PRINT_DEBUG(message, ...)
#endif

static OVRContext ovr_context_;
static OVRContext *ovr_context = &ovr_context_;

#define max(a, b) ((a) > (b) ? a : b)

bool ovr::init(LUID **adapter)
{
    ovrInitParams ovr_params = {};
    ovr_params.Flags = ovrInit_Debug;
    ovrResult result = ovr_Initialize(&ovr_params);
    if (OVR_FAILURE(result)) {
        PRINT_DEBUG("Error initializing OculusSDK.");
        return false;
    }

    ovrGraphicsLuid luid;
    result = ovr_Create(&ovr_context->session, &luid);
    if (OVR_FAILURE(result)) {
        PRINT_DEBUG("Error obtaining Oculus HMD.");
    }

    ovr_context->adapter_luid = *((LUID *)&luid);
    *adapter = &ovr_context->adapter_luid;

    return OVR_VALID_CONTEXT((*ovr_context));
}

bool ovr::init_swap_chain()
{
    ovrHmdDesc desc = ovr_GetHmdDesc(ovr_context->session);
    ovrSizei resolution = desc.Resolution;

    ovrSizei recommended_left_tex_size = ovr_GetFovTextureSize(ovr_context->session, ovrEye_Left, desc.DefaultEyeFov[0], 1.0f);
    ovrSizei recommended_right_tex_size = ovr_GetFovTextureSize(ovr_context->session, ovrEye_Right, desc.DefaultEyeFov[1], 1.0f);

    ovrSizei buffer_size;
    buffer_size.w = recommended_left_tex_size.w + recommended_right_tex_size.w;
    buffer_size.h = max(recommended_left_tex_size.h, recommended_right_tex_size.h);

    ovrTextureSwapChainDesc swap_chain_desc = {};
    swap_chain_desc.Type = ovrTexture_2D;
    swap_chain_desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
    swap_chain_desc.ArraySize = 1;
    swap_chain_desc.Width = buffer_size.w;
    swap_chain_desc.Height = buffer_size.h;
    swap_chain_desc.MipLevels = 1;
    swap_chain_desc.SampleCount = 1;
    swap_chain_desc.StaticImage = ovrFalse;
    swap_chain_desc.MiscFlags = ovrTextureMisc_None;
    swap_chain_desc.BindFlags = ovrTextureBind_DX_RenderTarget;

    ovrResult result = ovr_CreateTextureSwapChainDX(ovr_context->session, graphics_context->device, &swap_chain_desc, &ovr_context->swap_chain);
    if (OVR_FAILURE(result)) {
        PRINT_DEBUG("Error creating oculus swap chain.");
        return false;
    }

    ovr_GetTextureSwapChainLength(ovr_context->session, ovr_context->swap_chain, &ovr_context->rts_count);

    ovr_context->rts = memory::alloc_heap<ID3D11RenderTargetView *>(ovr_context->rts_count);

    for (int32_t i = 0; i < ovr_context->rts_count; ++i)
    {
        ID3D11Texture2D *texture = NULL;
        ovr_GetTextureSwapChainBufferDX(ovr_context->session, ovr_context->swap_chain, i, IID_PPV_ARGS(&texture));
        graphics_context->device->CreateRenderTargetView(texture, NULL, &ovr_context->rts[i]);
        texture->Release();
    }

    ovr_context->depth_buffer = graphics::get_depth_buffer(buffer_size.w, buffer_size.h);

    ovrEyeRenderDesc eye_render_desc[2];
    eye_render_desc[0] = ovr_GetRenderDesc(ovr_context->session, ovrEye_Left, desc.DefaultEyeFov[0]);
    eye_render_desc[1] = ovr_GetRenderDesc(ovr_context->session, ovrEye_Right, desc.DefaultEyeFov[1]);
    ovr_context->hmd_to_eye[0] = eye_render_desc[0].HmdToEyeOffset;
    ovr_context->hmd_to_eye[1] = eye_render_desc[1].HmdToEyeOffset;

    ovr_context->layer.Header.Type     = ovrLayerType_EyeFov;
    ovr_context->layer.Header.Flags    = 0;
    ovr_context->layer.ColorTexture[0] = ovr_context->swap_chain;
    ovr_context->layer.ColorTexture[1] = ovr_context->swap_chain;
    ovr_context->layer.Fov[0]          = eye_render_desc[0].Fov;
    ovr_context->layer.Fov[1]          = eye_render_desc[1].Fov;
    ovr_context->layer.Viewport[0]     = {0, 0,                buffer_size.w / 2, buffer_size.h};
    ovr_context->layer.Viewport[1]     = {buffer_size.w / 2, 0, buffer_size.w / 2, buffer_size.h};
    return true;
}

Viewport ovr::get_current_viewport(Eye eye)
{
    Viewport current_viewport =
    {
        (float)ovr_context->layer.Viewport[eye].Pos.x,
        (float)ovr_context->layer.Viewport[eye].Pos.y,
        (float)ovr_context->layer.Viewport[eye].Size.w,
        (float)ovr_context->layer.Viewport[eye].Size.h
    };

    return current_viewport;
}

Matrix4x4 ovr::projection_matrix(Eye eye, float near, float far)
{
    ovrMatrix4f m = ovrMatrix4f_Projection(ovr_context->layer.Fov[eye], near, far, 0);
    Matrix4x4 proj_mat;
    memcpy(&proj_mat, &m, sizeof(Matrix4x4));
    proj_mat = math::transpose(proj_mat);
    return proj_mat;
}

Vector3 ovr::get_current_eye_position(Eye eye)
{
    ovrVector3f v = ovr_context->layer.RenderPose[eye].Position;
    return Vector3(v.x, v.y, v.z);
}

Quaternion ovr::get_current_eye_orientation(Eye eye)
{
    ovrQuatf q_ = ovr_context->layer.RenderPose[eye].Orientation;
    Quaternion q(q_.x, q_.y, q_.z, q_.w);
    return q;
}

Vector3 ovr::get_current_hand_position(Hand hand)
{
    ovrVector3f v = ovr_context->hand_positions[hand].Position;
    return Vector3(v.x, v.y, v.z);
}

Quaternion ovr::get_current_hand_orientation(Hand hand)
{
    ovrQuatf q_ = ovr_context->hand_positions[hand].Orientation;
    Quaternion q(q_.x, q_.y, q_.z, q_.w);
    return q;
}

RenderTarget ovr::get_current_target_buffer()
{
    RenderTarget render_target = {};

    int current_index = 0;
    ovr_GetTextureSwapChainCurrentIndex(ovr_context->session, ovr_context->swap_chain, &current_index);
    render_target.rt_view = ovr_context->rts[current_index];

    return render_target;
}

DepthBuffer ovr::get_depth_buffer()
{
    return ovr_context->depth_buffer;
}

void ovr::update_tracking()
{
    double display_midpoint_seconds = ovr_GetPredictedDisplayTime(ovr_context->session, 0);
    ovrTrackingState hmd_state = ovr_GetTrackingState(ovr_context->session, display_midpoint_seconds, ovrTrue);
    ovr_CalcEyePoses(hmd_state.HeadPose.ThePose, ovr_context->hmd_to_eye, ovr_context->layer.RenderPose);
    ovr_context->hand_positions[0] = hmd_state.HandPoses[0].ThePose;
    ovr_context->hand_positions[1] = hmd_state.HandPoses[1].ThePose;
}

void ovr::swap_frames()
{
    ovr_CommitTextureSwapChain(ovr_context->session, ovr_context->swap_chain);
    ovrLayerHeader *layers = &ovr_context->layer.Header;
    ovrResult result = ovr_SubmitFrame(ovr_context->session, 0, NULL, &layers, 1);
}

void ovr::release()
{
    for (int32_t i = 0; i < ovr_context->rts_count; ++i)
    {
        ovr_context->rts[i]->Release();
    }
    graphics::release(&ovr_context->depth_buffer);
    memory::free_heap(ovr_context->rts);
    if (ovr_context->swap_chain) ovr_DestroyTextureSwapChain(ovr_context->session, ovr_context->swap_chain);
    ovr_Destroy(ovr_context->session);
    ovr_Shutdown();
}

float ovr::get_trigger_state(Hand hand)
{
    ovrInputState input_state;
    if (OVR_SUCCESS(ovr_GetInputState(ovr_context->session, ovrControllerType_Touch, &input_state)))
    {
        return input_state.IndexTrigger[hand];
    }
    return 0.0f;
}
