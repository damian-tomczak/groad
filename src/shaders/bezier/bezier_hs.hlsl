matrix modelMtx;
matrix viewMtx;
matrix invViewMtx;
matrix projMtx;
matrix invProj;
matrix texMtx;
float4 cameraPos;
float4 color;
int flags;
int screenWidth;
int screenHeight;

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
	float3 controlPoint2 : CONTROL_POINTS_2;
	float3 controlPoint3 : CONTROL_POINTS_3;
};

struct HSConstantDataOutput
{
	float EdgeTessFactor[2] : SV_TessFactor;
};

#define NUM_CONTROL_POINTS 4

static const float adaptabilityFactor = 40.0f;

float3 projectToNDC(float3 worldPos)
{
    float4 clipSpace = mul(projMtx, float4(worldPos, 1.0));
    float3 ndc = clipSpace.xyz / clipSpace.w;
    return adaptabilityFactor * ndc;
}

HSConstantDataOutput CalcHSPatchConstants(
    InputPatch<VSOutput, NUM_CONTROL_POINTS> ip,
    uint PatchID : SV_PrimitiveID)
{
    HSConstantDataOutput Output;

    float edgeTessFactor = 1.0;

    float3 cp0Screen = projectToNDC(ip[0].controlPoint0);
    float3 cp1Screen = projectToNDC(ip[0].controlPoint1);
    float3 cp2Screen = projectToNDC(ip[0].controlPoint2);
    float3 cp3Screen = projectToNDC(ip[0].controlPoint3);

    float minX = min(min(cp0Screen.x, cp1Screen.x), min(cp2Screen.x, cp3Screen.x));
    float maxX = max(max(cp0Screen.x, cp1Screen.x), max(cp2Screen.x, cp3Screen.x));
    float minY = min(min(cp0Screen.y, cp1Screen.y), min(cp2Screen.y, cp3Screen.y));
    float maxY = max(max(cp0Screen.y, cp1Screen.y), max(cp2Screen.y, cp3Screen.y));

    float width = maxX - minX;
    float height = maxY - minY;

    edgeTessFactor = max(width, height);

    Output.EdgeTessFactor[0] = edgeTessFactor;
    Output.EdgeTessFactor[1] = edgeTessFactor;

    return Output;
}

[domain("isoline")]
[partitioning("integer")]
[outputtopology("line")]
[outputcontrolpoints(NUM_CONTROL_POINTS)]
[patchconstantfunc("CalcHSPatchConstants")]
HSOutput main(
	InputPatch<VSOutput, NUM_CONTROL_POINTS> ip,
	uint i : SV_OutputControlPointID,
	uint PatchID : SV_PrimitiveID )
{
	HSOutput o = (HSOutput) 0;

	o.controlPoint0 = ip[i].controlPoint0;
	o.controlPoint1 = ip[i].controlPoint1;
	o.controlPoint2 = ip[i].controlPoint2;
	o.controlPoint3 = ip[i].controlPoint3;

	return o;
}
