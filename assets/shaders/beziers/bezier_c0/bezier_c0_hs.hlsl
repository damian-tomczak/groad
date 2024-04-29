struct VSOutput
{
    float3 controlPoint0 : CONTROL_POINTS_0;
    float3 controlPoint1 : CONTROL_POINTS_1;
    float3 controlPoint2 : CONTROL_POINTS_2;
    float3 controlPoint3 : CONTROL_POINTS_3;
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

#define NUM_CONTROL_POINTS 2  // We only need two control points for the line

HSConstantDataOutput CalcHSPatchConstants(
    InputPatch<VSOutput, 4> ip,  // Use 4 because we still have 4 input control points
    uint PatchID : SV_PrimitiveID)
{
    HSConstantDataOutput Output;

    // Simple tessellation factor for a single line segment
    Output.EdgeTessFactor[0] = 1.0;
    Output.EdgeTessFactor[1] = 1.0;  // Not used in isoline domain but must be defined

    return Output;
}

[domain("isoline")]
[partitioning("fractional_odd")]
[outputtopology("line")]
[outputcontrolpoints(NUM_CONTROL_POINTS)]
[patchconstantfunc("CalcHSPatchConstants")]
HSOutput main(
    InputPatch<VSOutput, 4> ip,  // Original input still has 4 control points
    uint i : SV_OutputControlPointID,
    uint PatchID : SV_PrimitiveID)
{
    HSOutput o;

    // Output control points 0 and 3 from the input as a line
    o.controlPoint0 = ip[0].controlPoint0;  // First control point
    o.controlPoint1 = ip[0].controlPoint3;  // Third control point (not second)

    return o;
}
