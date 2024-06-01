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

struct VertexInput
{
    float3 position : POSITION0;
};

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float3 lightVec : TEXCOORD1;
    float3 cameraVec : TEXCOORD2;
};

PixelShaderInput main(VertexInput input)
{
    PixelShaderInput output;
    float4 worldPosition = float4(input.position, 1.0f);

    output.position = mul(mul(mul(projMtx, viewMtx), modelMtx), worldPosition);
    output.texCoord = mul(texMtx, worldPosition).xz;
    output.lightVec = float3(0, 5, 0) - input.position;
    output.cameraVec = cameraPos.xyz - input.position;

    return output;
}
