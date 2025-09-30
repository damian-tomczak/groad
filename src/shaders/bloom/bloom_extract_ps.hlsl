Texture2D sceneTexture : register(t0);
SamplerState linearSampler : register(s0);

cbuffer BloomCB : register(b1)
{
    float4 params0;
    float4 params1;
};

struct PSInput
{
    float4 pos : SV_Position;
    float2 tex : TEXCOORD0;
};

float luminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

float4 main(PSInput input) : SV_Target
{
    float4 color = sceneTexture.Sample(linearSampler, input.tex);
    float threshold = params0.x;
    float softness = 0.35f * max(threshold, 0.0001f);
    float bright = luminance(color.rgb);
    float weight = saturate((bright - threshold) / max(softness, 0.0001f));
    return float4(color.rgb * weight, 1.0f);
}
