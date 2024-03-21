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

struct VertexOutput {
    float4 position : SV_POSITION;
    float3 nearPoint : TEXCOORD0;
    float3 farPoint : TEXCOORD1;
};

static const float3 gridPlane[6] = {
    float3(1, 1, 0), float3(-1, -1, 0), float3(-1, 1, 0),
    float3(-1, -1, 0), float3(1, 1, 0), float3(1, -1, 0)
};

float3 UnprojectPoint(float x, float y, float z, matrix view, matrix projection) {
    matrix viewInv = invView;
    matrix projInv = invProj;
    float4 unprojectedPoint = mul(mul(float4(x, y, z, 1.0), projInv), viewInv);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

VertexOutput main(VertexInput input) {
    VertexOutput output;
    float3 p = gridPlane[input.vertexID];
    output.nearPoint = UnprojectPoint(p.x, p.y, 0.0, view, proj);
    output.farPoint = UnprojectPoint(p.x, p.y, 1.0, view, proj);
    output.position = float4(p, 1.0);
    return output;
}
