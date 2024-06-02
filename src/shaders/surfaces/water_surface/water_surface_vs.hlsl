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
    float3 pos : POSITION0;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float3 localPos : POSITION0;
    float3 worldPos : POSITION1;
};

PSInput main(VSInput i)
{
    PSInput o;

    o.localPos = i.pos;
    o.localPos.y = -0.4f; /* Water Level */

    o.pos = float4(i.pos, 1.0f);
    o.worldPos = mul(modelMtx, o.pos).xyz;

    o.pos = mul(modelMtx, o.pos);
    o.pos = mul(viewMtx, o.pos);
    o.pos = mul(projMtx, o.pos);

    return o;
}
