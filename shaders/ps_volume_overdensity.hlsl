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

#define COLOR_UNDERDENSE float4(0.0, 0.0, 25.0, 0.08)
#define COLOR_MIDDENSE float4(0.0, 2.0, 0.0, 0.06)
#define COLOR_OVERDENSE float4(10.0, 0.0, 0.0, 0.15)

float remap(float val, float slope)
{
	return 1.0 - exp(-slope * val);
}

float4 main(PixelInput input) : SV_TARGET
{
    float4 fragment;
    float trace = tex_trace.Sample(tex_sampler_trace, input.texcoord_out.xyz);
    if (input.texcoord_out.x < trim_x_min || input.texcoord_out.x > trim_x_max ||
        input.texcoord_out.y < trim_y_min || input.texcoord_out.y > trim_y_max ||
        input.texcoord_out.z < trim_z_min || input.texcoord_out.z > trim_z_max ||
        trace < trim_density) {
        fragment = 0.0.xxxx;
    }
    else {
        if (trace < overdensity_threshold_low)
            fragment = COLOR_UNDERDENSE;
        else if (trace >= overdensity_threshold_low && trace < overdensity_threshold_high)
            fragment = COLOR_MIDDENSE;
        else if (trace >= overdensity_threshold_high)
            fragment = COLOR_OVERDENSE;
        fragment.rgb *= remap(2.71, sample_weight);
    	fragment.a *= optical_thickness * remap(trace, 1.0);
    }

    fragment.rgb *= 2.0; // Compensate for the drawing 1 stack of meta-quads instead of 3
	return fragment;
}