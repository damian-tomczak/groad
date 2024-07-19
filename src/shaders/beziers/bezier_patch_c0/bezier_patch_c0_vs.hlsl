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

struct VSInput
{
    float3 position : POSITION;
    float3 controlPoint0 : INS_POINT0;
    float3 controlPoint1 : INS_POINT1;
    float3 controlPoint2 : INS_POINT2;
    float3 controlPoint3 : INS_POINT3;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 controlPoint0 : CONTROL_POINTS_0;
    float3 controlPoint1 : CONTROL_POINTS_1;
    float3 controlPoint2 : CONTROL_POINTS_2;
    float3 controlPoint3 : CONTROL_POINTS_3;
};

VSOutput main(VSInput i)
{
    VSOutput o;

    o.position = float4(i.position, 1.0);

    o.controlPoint0 = mul(viewMtx, float4(i.controlPoint0, 1.0)).xyz;
    o.controlPoint1 = mul(viewMtx, float4(i.controlPoint1, 1.0)).xyz;
    o.controlPoint2 = mul(viewMtx, float4(i.controlPoint2, 1.0)).xyz;
    o.controlPoint3 = mul(viewMtx, float4(i.controlPoint3, 1.0)).xyz;

    return o;
}