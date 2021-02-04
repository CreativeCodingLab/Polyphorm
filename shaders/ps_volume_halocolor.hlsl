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
		float trace = 2.0 * tex_trace.Sample(tex_trace_sampler, input.texcoord_out.xyz);
        trace -= 0.166 * tex_trace.Sample(tex_trace_sampler, input.texcoord_out.xyz + float3( 1./world_width, 0, 0));
        trace -= 0.166 * tex_trace.Sample(tex_trace_sampler, input.texcoord_out.xyz + float3(-1./world_width, 0, 0));
        trace -= 0.166 * tex_trace.Sample(tex_trace_sampler, input.texcoord_out.xyz + float3(0, 1./world_height, 0));
        trace -= 0.166 * tex_trace.Sample(tex_trace_sampler, input.texcoord_out.xyz + float3(0,-1./world_height, 0));
        trace -= 0.166 * tex_trace.Sample(tex_trace_sampler, input.texcoord_out.xyz + float3(0, 0,  1./world_depth));
        trace -= 0.166 * tex_trace.Sample(tex_trace_sampler, input.texcoord_out.xyz + float3(0, 0, -1./world_depth));
		float t = sample_weight * (trace - trim_density);
		float2 deposit = tex_deposit.Sample(tex_deposit_sampler, input.texcoord_out.xyz).xy;
		float2 d = galaxy_weight * deposit;
		
		fragment.rgb = remap(t, 0.6) * float3(0.5, 0.5, 0.5);
        if (d.y > 0.0)
            fragment.rgb += remap(d.x, 0.1) * float3(0.0, 0.0, 0.8);
        else// if (d.y < 0.0)
            fragment.rgb += remap(d.x, 0.1) * float3(0.7, 0.0, 0.0);
        // else
        //     fragment.rgb += remap(d.x, 0.1) * float3(0.0, 0.6, 0.0);

		fragment.a = (0.3*t + 0.15*d.x) * optical_thickness;
	}

	fragment.rgb *= 2.0; // Compensate for the drawing 1 stack of meta-quads instead of 3
	return fragment;
}