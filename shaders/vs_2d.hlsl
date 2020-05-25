struct VertexInput
{
	float4 position: POSITION;
	float2 texcoord: TEXCOORD;
};

struct VertexOutput
{
	float4 position_out: SV_POSITION;
	float2 texcoord_out: TEXCOORD;
};

VertexOutput main(VertexInput input)
{
	VertexOutput result;

	result.position_out = input.position;
	result.texcoord_out = input.texcoord;

	return result;
}