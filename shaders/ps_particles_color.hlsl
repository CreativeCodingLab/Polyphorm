struct PixelInput
{
	float4 position_out: SV_POSITION;
    float2 texcoord_out: TEXCOORD;
};

Texture2D tex_trace : register(t0);
SamplerState tex_sampler_trace : register(s0);

float4 main(PixelInput input) : SV_TARGET
{
	float v = tex_trace.Sample(tex_sampler_trace, input.texcoord_out);
	if (v > 9999.0) {
		return float4(1.0-exp(-0.0001*v), 0, 0, 1);
	}
	v /= 3.0;
	return float4(0.6*v, v, 0.7*v, 1.0);
}