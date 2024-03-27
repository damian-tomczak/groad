cbuffer CBuffer : register(b0) {
    matrix model;
    matrix view;
    matrix invView;
    matrix proj;
    matrix invProj;
    int flags;
};

struct VertexIn {
    float3 pos : POSITION;
};

struct VertexOut
{
    float4 pos : SV_POSITION;
    bool isSelected : BOOLEAN1;
    bool isCentroid : BOOLEAN2;
};


VertexOut main(VertexIn vin)
{
    VertexOut vout;

    vout.pos = mul(mul(mul(proj, view), model), float4(vin.pos, 1.0));

    vout.isSelected = flags & 1;
    vout.isCentroid = flags & 2;

    return vout;
}

