cbuffer CBuffer: register(b0) {
    matrix model;
    matrix view;
    matrix invView;
    matrix proj;
    matrix invProj;
    int flags;
};

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
    float3 p = axisLines[input.vertexID];
    matrix mvp = mul(model, view);
    mvp = mul(mvp, proj);
    vout.pos = mul(float4(p, 1.0), mvp);

    float3 color = axisLines[input.vertexID + ((input.vertexID % 2 == 0 ) ? 1 : 0)];
    vout.color = float4(color, 1.0);

    return vout;
}
