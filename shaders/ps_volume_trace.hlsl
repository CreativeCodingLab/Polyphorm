struct PixelInput
{
	float4 position_out: SV_POSITION;
    float3 texcoord_out: TEXCOORD;
};

Texture3D tex_trace : register(t0);
SamplerState tex_sampler_trace : register(s0);
Texture2D tex_false_color : register(t1);
SamplerState tex_false_color_sampler : register(s1);

cbuffer ConfigBuffer : register(b4)
{
    float4x4 projection_matrix;
    float4x4 view_matrix;
    float4x4 model_matrix;
	int texcoord_map;
	float trim_x_min;
    float trim_x_max;
    float trim_y_min;
    float trim_y_max;
    float trim_z_min;
    float trim_z_max;
    float trim_density;
    float world_X;
    float world_Y;
    float world_Z;
    float screen_width;
    float screen_height;
	float sample_weight;
	float optical_thickness;
	float highlight_density;
	float galaxy_weight;
	float histogram_base;
};

float remap(float val, float slope)
{
	return 1.0 - exp(-slope * val);
}

float4 main(PixelInput input) : SV_TARGET
{
    float4 fragment;
    float trace = tex_trace.Sample(tex_sampler_trace, input.texcoord_out.xyz).r;
    if (input.texcoord_out.x < trim_x_min || input.texcoord_out.x > trim_x_max ||
        input.texcoord_out.y < trim_y_min || input.texcoord_out.y > trim_y_max ||
        input.texcoord_out.z < trim_z_min || input.texcoord_out.z > trim_z_max ||
        // false) {
        trace < trim_density) {
        fragment = 0.0.xxxx;
    }
    else {
        float t = (trace - trim_density) * sample_weight;
        // t /= 5.0; // Compensate for the difference between Trace and Highlight vis modes
        fragment.rgb = tex_false_color.Sample(tex_false_color_sampler, float2(remap(t, 1.0), 0.5)).rgb;
    	fragment.a = optical_thickness * remap(t, 1.0);

        // Proxy draws config
        // float3 o = float3(0.9321, 0.5179, 0.1403); // for SDSS_huge
        // float3 v_l = float3(0.2175, 0.4660, 0.4807) - o; // for SDSS_huge

        // float3 o = float3(-0.0504, 1.0047, 0.8905); // for FRB_cigale
        // float3 v_l = float3(0.6647, 0.4100, 0.4624) - o; // for FRB_cigale

        // float3 u_l = normalize(v_l);
        // float3 w = input.texcoord_out.xyz - o;

        // // Proxy sightline
        // float3 d = w - dot(w, u_l) * u_l;
        // d *= float3(world_X, world_Y, world_Z);
        // if (length(d) < 2.0) {
        //     fragment.rgb += float3(2.0, t, 0.0);
        //     fragment.a += optical_thickness * 2.0;
        // }

        // // Proxy distance shells
        // v_l *= float3(world_X, world_Y, world_Z);
        // w *= float3(world_X, world_Y, world_Z);
        // float roi_distance = 10.0 * galaxy_weight * length(v_l);
        // float shell_thickness = 1.0; // voxels
        // if (abs(length(w) - 0.25 * roi_distance) < shell_thickness ||
        //     abs(length(w) - 0.50 * roi_distance) < shell_thickness ||
        //     abs(length(w) - 0.75 * roi_distance) < shell_thickness ||
        //     abs(length(w) - 1.00 * roi_distance) < shell_thickness ) {
        //     fragment.rgb += float3(0.5, t, 1.0);
        //     // fragment.rgb += float3(2.0, t, 0.2);
        //     fragment.a += optical_thickness * 0.3;
        // }
    }

    fragment.rgb *= 2.0; // Compensate for the drawing 1 stack of meta-quads instead of 3
	return fragment;
}