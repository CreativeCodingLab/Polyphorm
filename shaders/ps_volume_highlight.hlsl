struct PixelInput
{
	float4 position_out: SV_POSITION;
    float3 texcoord_out: TEXCOORD;
};

Texture3D tex_trace : register(t0);
SamplerState tex_trace_sampler : register(s0);
Texture3D tex_deposit : register(t1);
SamplerState tex_deposit_sampler : register(s1);

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
};

float remap(float val, float slope)
{
	return 1.0 - exp(-slope * val);
}

float4 main(PixelInput input) : SV_TARGET
{
	float4 fragment;
    if (input.texcoord_out.x < trim_x_min || input.texcoord_out.x > trim_x_max ||
        input.texcoord_out.y < trim_y_min || input.texcoord_out.y > trim_y_max ||
        input.texcoord_out.z < trim_z_min || input.texcoord_out.z > trim_z_max) {
        fragment = 0.0.xxxx;
    }
	else {
		float trace = tex_trace.Sample(tex_trace_sampler, input.texcoord_out.xyz);
		float t = sample_weight * trace;
		float deposit = tex_deposit.Sample(tex_deposit_sampler, input.texcoord_out.xyz);
		// float d = galaxy_weight * deposit;
		float d = deposit > trim_density ? galaxy_weight * deposit : 0.0;
		
		fragment.rgb = remap(t, 0.3) * float3(0.0, 2.1, 15.0) + remap(d, 0.2) * float3(1.0, 0.05, 0.0);
		// fragment.rgb = remap(t, 0.3) * float3(0.6, 0.05, 0.9) + remap(d, 0.2) * float3(1.0, 0.6, 0.0);
		fragment.a = (0.2*t + 0.1*d) * optical_thickness;

		float highlight_low = highlight_density / sqrt(histogram_base);
		float highlight_high = highlight_density * sqrt(histogram_base);
		// float highlight_low = highlight_density / sqrt(pow(histogram_base, 0.4));
		// float highlight_high = highlight_density * sqrt(pow(histogram_base, 0.4));
		if (trace > highlight_low && trace < highlight_high) {
			float smooth_weight = smoothstep(highlight_low, highlight_low + 0.2 * (highlight_high-highlight_low), trace)
						* (1.0 - smoothstep(highlight_high - 0.2 * (highlight_high-highlight_low), highlight_high, trace));
			fragment.rgb = (1.0-smooth_weight) * fragment.rgb
							+ smooth_weight * remap(trace, 10.0) * float3(0.0, 1.0, 0.0);
			fragment.a = (1.0-smooth_weight) * fragment.a
							+ smooth_weight * remap(trace, 10.0) * optical_thickness;
		}
	}

	fragment.rgb *= 2.0; // Compensate for the drawing 1 stack of meta-quads instead of 3
	return fragment;
}