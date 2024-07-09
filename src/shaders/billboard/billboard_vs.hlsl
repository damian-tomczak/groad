matrix modelMtx;
matrix viewMtx;
matrix invViewMtx;
matrix projMtx;
matrix invProjMtx;
matrix texMtx;
float4 cameraPos;
float4 color;
int flags;
int screenWidth;
int screenHeight;

struct PSInput
{
    float4 pos : SV_POSITION;
};

static const float3 plane[6] =
{
    float3(-0.5f,  0.5f, 0.0f),
    float3( 0.5f,  0.5f, 0.0f),
    float3(-0.5f, -0.5f, 0.0f),
    float3(-0.5f, -0.5f, 0.0f),
    float3( 0.5f,  0.5f, 0.0f),
    float3( 0.5f, -0.5f, 0.0f)
};

PSInput main(uint vertexID : SV_VertexID)
{
    PSInput result;
    result.pos = mul(mul(mul(projMtx, viewMtx), modelMtx), float4(plane[vertexID], 1.0));

    return result;
}
