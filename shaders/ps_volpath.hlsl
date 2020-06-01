struct PixelInput {
  float4 position_out: SV_POSITION;
  float2 texcoord_out: TEXCOORD;
};

Texture2D tex : register(t0);
SamplerState tex_sampler : register(s0);

float4 main(PixelInput input) : SV_TARGET {
  float3 L = tex.Sample(tex_sampler, input.texcoord_out).rgb;
  return float4(L, 1.0);
}