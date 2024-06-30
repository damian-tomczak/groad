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

struct HSInput
{
    float4 pos : POSITION;
};

HSInput main(VSInput i)
{
    HSInput o;
    o.pos = float4(i.pos, 1.0f);

    return o;
}