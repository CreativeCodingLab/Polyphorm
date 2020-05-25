struct VertexInput
{
	float4 position: POSITION;
	float3 texcoord: TEXCOORD;
};

struct VertexOutput
{
	float4 position_out: SV_POSITION;
	float3 texcoord_out: TEXCOORD;
};


cbuffer ConfigBuffer : register(b4)
{
    float4x4 projection_matrix;
    float4x4 view_matrix;
    float4x4 model_matrix;
	int texcoord_map;
};

VertexOutput main(VertexInput input)
{
	VertexOutput result;

	result.position_out = mul(projection_matrix, mul(view_matrix, mul(model_matrix, input.position)));
	if (texcoord_map == 1) {
		result.texcoord_out = float3(input.texcoord.x, input.texcoord.y, input.texcoord.z);
	} else if (texcoord_map == -1) {
		result.texcoord_out = float3(1.0-input.texcoord.x, input.texcoord.y, 1.0-input.texcoord.z);
	} else if (texcoord_map == 2) {
		result.texcoord_out = float3(input.texcoord.x, input.texcoord.z, 1.0-input.texcoord.y);
	} else if (texcoord_map == -2) {
		result.texcoord_out = float3(input.texcoord.x, 1.0-input.texcoord.z, input.texcoord.y);
	} else if (texcoord_map == 3) {
		result.texcoord_out = float3(1.0-input.texcoord.z, input.texcoord.y, input.texcoord.x);
	} else if (texcoord_map == -3) {
		result.texcoord_out = float3(input.texcoord.z, input.texcoord.y, 1.0-input.texcoord.x);
	}

	return result;
}