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
    float3 pos : POSITION;
};

struct PSInput
{
    float4 pos : SV_POSITION;
};

PSInput main(VSInput i)
{
    PSInput o;

    float4 v = mul(viewMtx, float4(i.pos, 1.0f));
    o.pos = mul(projMtx, v);

    return o;
}
