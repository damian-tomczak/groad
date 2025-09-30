Texture2D sceneTexture : register(t0);
Texture2D bloomTexture : register(t1);
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

float3 tonemap(float3 color)
{
    return 1.0f - exp(-color);
}

float4 main(PSInput input) : SV_Target
{
    float3 sceneColor = sceneTexture.Sample(linearSampler, input.tex).rgb;
    float3 bloomColor = bloomTexture.Sample(linearSampler, input.tex).rgb;
    float intensity = params0.y;

    float3 color = sceneColor + bloomColor * intensity;
    color = tonemap(color);

    return float4(color, 1.0f);
}
