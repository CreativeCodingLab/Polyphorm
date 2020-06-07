struct PixelInput {
  float4 position_out: SV_POSITION;
  float2 texcoord_out: TEXCOORD;
};

Texture2D tex : register(t0);
SamplerState tex_sampler : register(s0);

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
    float grid_x;
    float grid_y;
    float grid_z;
    float screen_width;
    float screen_height;
	float sample_weight;
	float optical_thickness;
	float highlight_density;
	float galaxy_weight;
	float histogram_base;
    float overdensity_threshold_low;
    float overdensity_threshold_high;
    float camera_x;
    float camera_y;
    float camera_z;
    int pt_iteration;
    float sigma_s;
    float sigma_a;
    float sigma_e;
    float trace_max;
    float camera_offset_x;
    float camera_offset_y;
    float exposure;
    int n_bounces;
    float ambient_trace;
    int compressive_accumulation;
    int dummy2;
    int dummy3;
};

float3 tonemap(float3 L, float exposure) {
	return float3(1.0, 1.0, 1.0) - exp(-exposure * L);
}

float4 main(PixelInput input) : SV_TARGET {
  float3 L = tex.Sample(tex_sampler, input.texcoord_out).rgb;
  return float4(compressive_accumulation == 1? L : tonemap(L, exposure), 1.0);
}