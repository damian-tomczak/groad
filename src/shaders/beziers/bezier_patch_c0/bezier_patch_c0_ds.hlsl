matrix modelMtx;
matrix viewMtx;
matrix invView;
matrix projMtx;
matrix invProj;
matrix texMtx;
float4 cameraPos;
float4 color;
int flags;
int screenWidth;
int screenHeight;

struct DSOutput
{
    float4 position  : SV_POSITION;
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

float3 deCastiljeau3(float3 controlPoints[4], float t)
{
    float3 a = lerp(controlPoints[0], controlPoints[1], t);
    float3 b = lerp(controlPoints[1], controlPoints[2], t);
    float3 c = lerp(controlPoints[2], controlPoints[3], t);
    float3 d = lerp(a, b, t);
    float3 e = lerp(b, c, t);

    return lerp(d, e, t);
}

[domain("quad")]
DSOutput main(
    HSConstantDataOutput input,
    float2 domain : SV_DomainLocation,
    const OutputPatch<HSOutput, NUM_CONTROL_POINTS> patch)
{
    DSOutput o;

    float u = domain.x, v = domain.y;
	
	float3 uControlPoints0[4] = { patch[0].controlPoint0, patch[0].controlPoint1, patch[0].controlPoint2, patch[0].controlPoint3 };
	float3 uControlPoints1[4] = { patch[0].controlPoint4, patch[0].controlPoint5, patch[0].controlPoint6, patch[0].controlPoint7 };
	float3 uControlPoints2[4] = { patch[0].controlPoint8, patch[0].controlPoint9, patch[0].controlPoint10, patch[0].controlPoint11 };
    float3 uControlPoints3[4] = { patch[0].controlPoint12, patch[0].controlPoint13, patch[0].controlPoint14, patch[0].controlPoint15 };

    float3 vControlPoints0[4] = { patch[0].controlPoint0, patch[0].controlPoint4, patch[0].controlPoint8, patch[0].controlPoint12 };
    float3 vControlPoints1[4] = { patch[0].controlPoint1, patch[0].controlPoint5, patch[0].controlPoint9, patch[0].controlPoint13 };
    float3 vControlPoints2[4] = { patch[0].controlPoint2, patch[0].controlPoint6, patch[0].controlPoint10, patch[0].controlPoint14 };
    float3 vControlPoints3[4] = { patch[0].controlPoint3, patch[0].controlPoint7, patch[0].controlPoint11, patch[0].controlPoint15 };
	
    float3 vControlPoints[4] = { deCastiljeau3(uControlPoints0, u), deCastiljeau3(uControlPoints1, u), deCastiljeau3(uControlPoints2, u), deCastiljeau3(uControlPoints3, u) };
    float3 uControlPoints[4] = { deCastiljeau3(vControlPoints0, v), deCastiljeau3(vControlPoints1, v), deCastiljeau3(vControlPoints2, v), deCastiljeau3(vControlPoints3, v) };
	
    float3 worldPos = deCastiljeau3(vControlPoints, v);
    o.position = mul(projMtx, float4(worldPos, 1.0));

    return o;
}
