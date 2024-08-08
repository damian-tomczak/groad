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

[patchconstantfunc("CalcHSPatchConstants")]
void CalcHSPatchConstants(
    InputPatch<VSOutput, NUM_CONTROL_POINTS> ip, 
    const uint patchID : SV_PrimitiveID,
    out HSConstantDataOutput Output)
{
    // Initialize tessellation factors
    Output.edges[0] = 1.0; // example tessellation factor for edge 0
    Output.edges[1] = 1.0; // example tessellation factor for edge 1
    Output.edges[2] = 1.0; // example tessellation factor for edge 2
    Output.edges[3] = 1.0; // example tessellation factor for edge 3

    Output.inside[0] = 1.0; // example tessellation factor for inside 0
    Output.inside[1] = 1.0; // example tessellation factor for inside 1
}

[domain("quad")]
[partitioning("fractional_odd")]
[outputtopology("triangle_ccw")]
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
