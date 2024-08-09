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

static const float4x4 bSplineToBernstein =
{
    1.0 / 6, 2.0 / 3, 1.0 / 6, 0.0,
    0.0, 2.0 / 3, 1.0 / 3, 0.0,
    0.0, 1.0 / 3, 2.0 / 3, 0.0,
    0.0, 1.0 / 6, 2.0 / 3, 1.0 / 6
};

[domain("quad")]
DSOutput main(
    HSConstantDataOutput input,
    float2 domain : SV_DomainLocation,
    const OutputPatch<HSOutput, NUM_CONTROL_POINTS> patch)
{
    DSOutput o;

    float u = domain.x, v = domain.y;

    float4x3 uPointsMatrix0 = {patch[0].controlPoint0,  patch[0].controlPoint1,  patch[0].controlPoint2,   patch[0].controlPoint3};
    float4x3 uPointsMatrix1 = {patch[0].controlPoint4,  patch[0].controlPoint5,  patch[0].controlPoint6,   patch[0].controlPoint7};
    float4x3 uPointsMatrix2 = {patch[0].controlPoint8,  patch[0].controlPoint9,  patch[0].controlPoint10,  patch[0].controlPoint11};
    float4x3 uPointsMatrix3 = {patch[0].controlPoint12, patch[0].controlPoint13, patch[0].controlPoint14,  patch[0].controlPoint15};

    float4x3 vPointsMatrix0 = {patch[0].controlPoint0,  patch[0].controlPoint4,  patch[0].controlPoint8,  patch[0].controlPoint12};
    float4x3 vPointsMatrix1 = {patch[0].controlPoint1,  patch[0].controlPoint5,  patch[0].controlPoint9,  patch[0].controlPoint13};
    float4x3 vPointsMatrix2 = {patch[0].controlPoint2,  patch[0].controlPoint6,  patch[0].controlPoint10, patch[0].controlPoint14};
    float4x3 vPointsMatrix3 = {patch[0].controlPoint3,  patch[0].controlPoint7,  patch[0].controlPoint11, patch[0].controlPoint15};

    float4x3 uPointsInBernstein0 = mul(bSplineToBernstein, uPointsMatrix0);
    float4x3 uPointsInBernstein1 = mul(bSplineToBernstein, uPointsMatrix1);
    float4x3 uPointsInBernstein2 = mul(bSplineToBernstein, uPointsMatrix2);
    float4x3 uPointsInBernstein3 = mul(bSplineToBernstein, uPointsMatrix3);

    float4x3 vPointsInBernstein0 = mul(bSplineToBernstein, vPointsMatrix0);
    float4x3 vPointsInBernstein1 = mul(bSplineToBernstein, vPointsMatrix1);
    float4x3 vPointsInBernstein2 = mul(bSplineToBernstein, vPointsMatrix2);
    float4x3 vPointsInBernstein3 = mul(bSplineToBernstein, vPointsMatrix3);

    float3 uControlPoints0[4] = {uPointsInBernstein0[0].xyz, uPointsInBernstein0[1].xyz, uPointsInBernstein0[2].xyz, uPointsInBernstein0[3].xyz};
    float3 uControlPoints1[4] = {uPointsInBernstein1[0].xyz, uPointsInBernstein1[1].xyz, uPointsInBernstein1[2].xyz, uPointsInBernstein1[3].xyz};
    float3 uControlPoints2[4] = {uPointsInBernstein2[0].xyz, uPointsInBernstein2[1].xyz, uPointsInBernstein2[2].xyz, uPointsInBernstein2[3].xyz};
    float3 uControlPoints3[4] = {uPointsInBernstein3[0].xyz, uPointsInBernstein3[1].xyz, uPointsInBernstein3[2].xyz, uPointsInBernstein3[3].xyz};

    float3 vControlPoints0[4] = {vPointsInBernstein0[0].xyz, vPointsInBernstein0[1].xyz, vPointsInBernstein0[2].xyz, vPointsInBernstein0[3].xyz};
    float3 vControlPoints1[4] = {vPointsInBernstein1[0].xyz, vPointsInBernstein1[1].xyz, vPointsInBernstein1[2].xyz, vPointsInBernstein1[3].xyz};
    float3 vControlPoints2[4] = {vPointsInBernstein2[0].xyz, vPointsInBernstein2[1].xyz, vPointsInBernstein2[2].xyz, vPointsInBernstein2[3].xyz};
    float3 vControlPoints3[4] = {vPointsInBernstein3[0].xyz, vPointsInBernstein3[1].xyz, vPointsInBernstein3[2].xyz, vPointsInBernstein3[3].xyz};

    float4x3 vPointsMatrix = {deCastiljeau3(uControlPoints0, u), deCastiljeau3(uControlPoints1, u), deCastiljeau3(uControlPoints2, u), deCastiljeau3(uControlPoints3, u)};
    float4x3 uPointsMatrix = {deCastiljeau3(vControlPoints0, v), deCastiljeau3(vControlPoints1, v), deCastiljeau3(vControlPoints2, v), deCastiljeau3(vControlPoints3, v)};

    float4x3 vPointsInBernstein = mul(bSplineToBernstein, vPointsMatrix);
    float4x3 uPointsInBernstein = mul(bSplineToBernstein, uPointsMatrix);

    float3 vControlPoints[4] = {vPointsInBernstein[0].xyz, vPointsInBernstein[1].xyz, vPointsInBernstein[2].xyz, vPointsInBernstein[3].xyz};
    float3 uControlPoints[4] = {uPointsInBernstein[0].xyz, uPointsInBernstein[1].xyz, uPointsInBernstein[2].xyz, uPointsInBernstein[3].xyz};

    float3 worldPos = deCastiljeau3(vControlPoints, v);
    o.position = mul(projMtx, float4(worldPos, 1.0));

    return o;
}
