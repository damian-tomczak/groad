matrix modelMtx;
matrix viewMtx;
matrix invViewMtx;
matrix projMtx;
matrix invProjMtx;
matrix texMtx;
float4 cameraPos;
int flags;
int screenWidth;
int screenHeight;

SamplerState samp;
TextureCube envMap;

struct PSInput
{
    float4 pos : SV_POSITION;
    float3 tex : TEXCOORD0;
};

float4 main(PSInput i) : SV_TARGET
{
    float3 color = envMap.Sample(samp , i.tex).rgb;

    return float4(color, 1.0f);
}
