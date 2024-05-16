cbuffer CBuffer : register(b0) {
    matrix model;
    matrix view;
    matrix invView;
    matrix projMtx;
    matrix invProj;
    int flags;
	int screenWidth;
	int screenHeight;
};

struct DSOutput
{
	float4 position  : SV_POSITION;
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
	float EdgeTessFactor[2]	: SV_TessFactor;
};

#define NUM_CONTROL_POINTS 1

float3 castiljeau(float3 controlPoints[4], float t)
{
	float3 a = lerp(controlPoints[0], controlPoints[1], t);
	float3 b = lerp(controlPoints[1], controlPoints[2], t);
	float3 c = lerp(controlPoints[2], controlPoints[3], t);
	float3 d = lerp(a, b, t);
	float3 e = lerp(b, c, t);

	return lerp(d, e, t);
}

[domain("isoline")]
DSOutput main(
	HSConstantDataOutput input,
	float2 domain : SV_DomainLocation,
	const OutputPatch<HSOutput, NUM_CONTROL_POINTS> patch)
{
	DSOutput o = (DSOutput) 0;

	float u = domain.x;
	float3 controlPoints[4] = { patch[0].controlPoint0, patch[0].controlPoint1, patch[0].controlPoint2, patch[0].controlPoint3 };

	float3 worldPosition = castiljeau(controlPoints, u);
	o.position = mul(projMtx, float4(worldPosition, 1.0));

	return o;
}
