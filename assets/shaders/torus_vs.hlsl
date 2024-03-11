cbuffer cbPerObject
{
    float4x4 mvp;
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
    vout.pos = mul(float4(vin.pos, 1.0f), mvp);
    return vout;
}
