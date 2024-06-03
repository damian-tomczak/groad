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

struct VSInput
{
    uint vertexID : SV_VertexID;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float3 localPos : POSITION0;
    float3 worldPos : POSITION1;
};

static const float3 plane[6] =
{
    float3(-0.5f, 0.0f,  0.5f),
    float3( 0.5f, 0.0f,  0.5f),
    float3(-0.5f, 0.0f, -0.5f),
    float3(-0.5f, 0.0f, -0.5f),
    float3( 0.5f, 0.0f,  0.5f),
    float3( 0.5f, 0.0f, -0.5f)
};

PSInput main(VSInput input)
{
    PSInput o;

    o.localPos = plane[input.vertexID];
    o.localPos.y = -0.4f; /* Water Level */

    o.pos = float4(o.localPos, 1.0f);
    o.worldPos = mul(modelMtx, o.pos).xyz;

    o.pos = mul(modelMtx, o.pos);
    o.pos = mul(viewMtx, o.pos);
    o.pos = mul(projMtx, o.pos);

    return o;
}
