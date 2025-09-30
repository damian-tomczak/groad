Texture2D glowTexture : register(t0);
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

static const float offsets[5] = {
    0.0f,
    1.3846153846f,
    3.2307692308f,
    5.0769230769f,
    6.9230769231f
};

static const float weights[5] = {
    0.2270270270f,
    0.3162162162f,
    0.0702702703f,
    0.0262162162f,
    0.0121621621f
};

float4 main(PSInput input) : SV_Target
{
    float2 texelSize = params1.xy;
    float2 direction = params1.zw;
    float blurStrength = max(params0.z, 0.0001f);

    float dirLength = length(direction);
    if (dirLength == 0.0f)
    {
        direction = float2(1.0f, 0.0f);
    }
    else
    {
        direction /= dirLength;
    }
    direction *= blurStrength;

    float3 color = glowTexture.Sample(linearSampler, input.tex).rgb * weights[0];

    [unroll]
    for (int i = 1; i < 5; ++i)
    {
        float2 offset = direction * offsets[i];
        float2 sampleOffset = offset * texelSize;
        color += glowTexture.Sample(linearSampler, input.tex + sampleOffset).rgb * weights[i];
        color += glowTexture.Sample(linearSampler, input.tex - sampleOffset).rgb * weights[i];
    }

    return float4(color, 1.0f);
}
