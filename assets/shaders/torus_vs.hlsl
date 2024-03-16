cbuffer CBuffer: register(b0) {
    matrix model;
    matrix view;
    matrix invView;
    matrix proj;
    matrix invProj;
};

struct VertexIn
{
    float3 pos : POSITION;
};

struct VertexOut
{
    float4 pos : SV_POSITION;
};

VertexOut main(VertexIn vin)
{
    VertexOut vout;
    matrix mvp = mul(model, view);
    mvp = mul(mvp, proj);
    vout.pos = mul(float4(vin.pos, 1.0f), mvp);
    return vout;
}
