cbuffer CBuffer : register(b0) {
    matrix modelMtx;
    matrix viewMtx;
    matrix invViewMtx;
    matrix projMtx;
    matrix invProjMtx;
    int flags;
};

struct DSOutput
{
    float4 position  : SV_POSITION;
};

struct HSOutput
{
    float3 controlPoint0 : CONTROL_POINTS_0;
    float3 controlPoint1 : CONTROL_POINTS_1;
};

struct HSConstantDataOutput
{
    float EdgeTessFactor[2] : SV_TessFactor;
};

#define NUM_CONTROL_POINTS 2

[domain("isoline")]
DSOutput main(
    HSConstantDataOutput input,
    float2 domain : SV_DomainLocation,
    const OutputPatch<HSOutput, NUM_CONTROL_POINTS> patch)
{
    DSOutput o = (DSOutput)0;

    float u = domain.x;
    float3 controlPoints[2] = { patch[0].controlPoint0, patch[0].controlPoint1 };

    float3 worldPosition = lerp(controlPoints[0], controlPoints[1], u);
    o.position = mul(projMtx, float4(worldPosition, 1.0));
    o.position.z = 0;

    return o;
}
