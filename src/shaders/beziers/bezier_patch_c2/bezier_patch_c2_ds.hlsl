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

float3 deCastiljeau(float3 controlPoints[4], float t)
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

[domain("isoline")]
DSOutput main(
    HSConstantDataOutput input,
    float2 domain : SV_DomainLocation,
    const OutputPatch<HSOutput, NUM_CONTROL_POINTS> patch)
{
    DSOutput o;

    float4x3 pointsMatrix = { patch[0].controlPoint0, patch[0].controlPoint1, patch[0].controlPoint2, patch[0].controlPoint3 };

    float4x3 pointsInBspline = mul(bSplineToBernstein, pointsMatrix);

    float u = domain.x;
    float3 controlPoints[4] = { pointsInBspline._11_12_13, pointsInBspline._21_22_23, pointsInBspline._31_32_33, pointsInBspline._41_42_43 };

    float3 worldPosition = deCastiljeau(controlPoints, u);
    o.position = mul(projMtx, float4(worldPosition, 1.0));

    return o;
}
