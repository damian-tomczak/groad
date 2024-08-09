struct VSOutput
{
    float3 controlPoint0  : CONTROL_POINT0;
    float3 controlPoint1  : CONTROL_POINT1;
    float3 controlPoint2  : CONTROL_POINT2;
    float3 controlPoint3  : CONTROL_POINT3;
    float3 controlPoint4  : CONTROL_POINT4;
    float3 controlPoint5  : CONTROL_POINT5;
    float3 controlPoint6  : CONTROL_POINT6;
    float3 controlPoint7  : CONTROL_POINT7;
    float3 controlPoint8  : CONTROL_POINT8;
    float3 controlPoint9  : CONTROL_POINT9;
    float3 controlPoint10 : CONTROL_POINT10;
    float3 controlPoint11 : CONTROL_POINT11;
    float3 controlPoint12 : CONTROL_POINT12;
    float3 controlPoint13 : CONTROL_POINT13;
    float3 controlPoint14 : CONTROL_POINT14;
    float3 controlPoint15 : CONTROL_POINT15;
};

struct HSOutput
{
    float3 controlPoint0  : CONTROL_POINT0;
    float3 controlPoint1  : CONTROL_POINT1;
    float3 controlPoint2  : CONTROL_POINT2;
    float3 controlPoint3  : CONTROL_POINT3;
    float3 controlPoint4  : CONTROL_POINT4;
    float3 controlPoint5  : CONTROL_POINT5;
    float3 controlPoint6  : CONTROL_POINT6;
    float3 controlPoint7  : CONTROL_POINT7;
    float3 controlPoint8  : CONTROL_POINT8;
    float3 controlPoint9  : CONTROL_POINT9;
    float3 controlPoint10 : CONTROL_POINT10;
    float3 controlPoint11 : CONTROL_POINT11;
    float3 controlPoint12 : CONTROL_POINT12;
    float3 controlPoint13 : CONTROL_POINT13;
    float3 controlPoint14 : CONTROL_POINT14;
    float3 controlPoint15 : CONTROL_POINT15;
};

struct HSConstantDataOutput
{
    float edges[4] : SV_TessFactor;
    float inside[2] : SV_InsideTessFactor;
};

#define NUM_CONTROL_POINTS 1

HSConstantDataOutput CalcHSPatchConstants(
    InputPatch<VSOutput, NUM_CONTROL_POINTS> ip,
    uint PatchID : SV_PrimitiveID)
{
    HSConstantDataOutput Output;

    float3 midPoints[4] =
    {
        -0.5f * (ip[0].controlPoint0 + ip[0].controlPoint12),
        -0.5f * (ip[0].controlPoint0 + ip[0].controlPoint3),
        -0.5f * (ip[0].controlPoint3 + ip[0].controlPoint15),
        -0.5f * (ip[0].controlPoint12 + ip[0].controlPoint15)
    };

    [unroll]
    for (int i = 0; i < 4; i++)
    {
        Output.edges[i] = -8 * log10(0.01 * -midPoints[i].z) + 3;
    }

    float3 mid = -0.5f * (ip[0].controlPoint5 + ip[0].controlPoint10);
    
    Output.inside[0] = Output.inside[1] = -8 * log10(0.01 * -mid.z) + 3;

    return Output;
}

[domain("quad")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(NUM_CONTROL_POINTS)]
[patchconstantfunc("CalcHSPatchConstants")]
HSOutput main(
    InputPatch<VSOutput, NUM_CONTROL_POINTS> ip,
    uint i : SV_OutputControlPointID,
    uint PatchID : SV_PrimitiveID )
{
    HSOutput o;

    o.controlPoint0 = ip[i].controlPoint0;
    o.controlPoint1 = ip[i].controlPoint1;
    o.controlPoint2 = ip[i].controlPoint2;
    o.controlPoint3 = ip[i].controlPoint3;
    o.controlPoint4 = ip[i].controlPoint4;
    o.controlPoint5 = ip[i].controlPoint5;
    o.controlPoint6 = ip[i].controlPoint6;
    o.controlPoint7 = ip[i].controlPoint7;
    o.controlPoint8 = ip[i].controlPoint8;
    o.controlPoint9 = ip[i].controlPoint9;
    o.controlPoint10 = ip[i].controlPoint10;
    o.controlPoint11 = ip[i].controlPoint11;
    o.controlPoint12 = ip[i].controlPoint12;
    o.controlPoint13 = ip[i].controlPoint13;
    o.controlPoint14 = ip[i].controlPoint14;
    o.controlPoint15 = ip[i].controlPoint15;

    return o;
}
