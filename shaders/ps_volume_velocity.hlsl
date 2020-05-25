struct PixelInput
{
	float4 position_out: SV_POSITION;
    float3 texcoord_out: TEXCOORD;
};

Texture3D tex_trace : register(t0);
SamplerState tex_sampler_trace : register(s0);

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
    float world_width;
    float world_height;
    float world_depth;
    float screen_width;
    float screen_height;
	float sample_weight;
	float optical_thickness;
	float highlight_density;
	float galaxy_weight;
	float histogram_base;
    float overdensity_threshold_low;
    float overdensity_threshold_high;
};

float remap(float val, float slope)
{
	return 1.0 - exp(-slope * val);
}

float4 main(PixelInput input) : SV_TARGET
{
    float4 fragment;
    float4 phase = tex_trace.Sample(tex_sampler_trace, input.texcoord_out.xyz).rgba;
    if (input.texcoord_out.x < trim_x_min || input.texcoord_out.x > trim_x_max ||
        input.texcoord_out.y < trim_y_min || input.texcoord_out.y > trim_y_max ||
        input.texcoord_out.z < trim_z_min || input.texcoord_out.z > trim_z_max) {
        fragment = 0.0.xxxx;
    }
    else {
        fragment.rgb = sample_weight * phase.gba * phase.gba * float3(0.6, 0.5, 1.0);
    	fragment.a = remap(0.1 * phase.r, optical_thickness);
    }

    fragment.rgb *= 2.0; // Compensate for the drawing 1 stack of meta-quads instead of 3
	return fragment;
}