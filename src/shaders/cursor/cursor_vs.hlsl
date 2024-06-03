matrix model;
matrix view;
matrix invView;
matrix proj;
matrix invProj;
matrix texMtx;
float4 cameraPos;
int flags;
int screenWidth;
int screenHeight;

struct VertexInput {
    uint vertexID : SV_VertexID;
};

struct VertexOut {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

static const float3 axisLines[6] = {
    float3(0, 0, 0), float3(1, 0, 0),
    float3(0, 0, 0), float3(0, 1, 0),
    float3(0, 0, 0), float3(0, 0, 1)
};

VertexOut main(VertexInput input)
{
    VertexOut vout;
    vout.pos = mul(mul(mul(proj, view), model), float4(axisLines[input.vertexID], 1.0));

    float3 color = axisLines[input.vertexID + ((input.vertexID % 2 == 0 ) ? 1 : 0)];
    vout.color = float4(color, 1.0);

    return vout;
}
