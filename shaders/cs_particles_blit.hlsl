RWTexture2D<uint> tex_in: register(u0);
RWTexture2D<float> tex_out: register(u1);

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

[numthreads(1, 1, 1)]
void main(uint3 threadIDInGroup : SV_GroupThreadID, uint3 groupID : SV_GroupID,
          uint3 dispatchThreadId : SV_DispatchThreadID){
    float val = tex_in[dispatchThreadId.xy];
    if (val < 10000) {
        tex_out[dispatchThreadId.xy] = val * sample_weight;
    } else {
        tex_out[dispatchThreadId.xy] = val;
    }
}