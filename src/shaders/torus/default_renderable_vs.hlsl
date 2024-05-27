matrix model;
matrix view;
matrix invView;
matrix proj;
matrix invProj;
matrix texMtx;
float3 cameraPos;
int flags;
int screenWidth;
int screenHeight;

struct VertexIn {
    float3 pos : POSITION;
};

struct VertexOut
{
    float4 pos : SV_POSITION;
};

VertexOut main(VertexIn vin)
{
    VertexOut vout;

    vout.pos = mul(mul(mul(proj, view), model), float4(vin.pos, 1.0));

    return vout;
}

