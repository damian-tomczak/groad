cbuffer CBufferDesc: register(b0) {
    matrix proj;
    matrix view;
    matrix model;
};

static float3 gridPlane[6] = {
    float3(1, 1, 0), float3(-1, -1, 0), float3(-1, 1, 0),
    float3(-1, -1, 0), float3(1, 1, 0), float3(1, -1, 0)
};

struct VSInput {
    uint vertexID : SV_VertexID;
};

struct VSOutput {
    float4 pos : SV_Position;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.pos = mul(proj, mul(view, float4(gridPlane[input.vertexID].xyz, 1.0)));
    
    return output;
}
