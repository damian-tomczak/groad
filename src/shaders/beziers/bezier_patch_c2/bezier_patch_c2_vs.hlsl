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

struct VSOutput
{
    float3 controlPoint0 :  CONTROL_POINT0;
    float3 controlPoint1 :  CONTROL_POINT1;
    float3 controlPoint2 :  CONTROL_POINT2;
    float3 controlPoint3 :  CONTROL_POINT3;
    float3 controlPoint4 :  CONTROL_POINT4;
    float3 controlPoint5 :  CONTROL_POINT5;
    float3 controlPoint6 :  CONTROL_POINT6;
    float3 controlPoint7 :  CONTROL_POINT7;
    float3 controlPoint8 :  CONTROL_POINT8;
    float3 controlPoint9 :  CONTROL_POINT9;
    float3 controlPoint10 : CONTROL_POINT10;
    float3 controlPoint11 : CONTROL_POINT11;
    float3 controlPoint12 : CONTROL_POINT12;
    float3 controlPoint13 : CONTROL_POINT13;
    float3 controlPoint14 : CONTROL_POINT14;
    float3 controlPoint15 : CONTROL_POINT15;
};

VSOutput main(VSInput i)
{
    VSOutput o;

    o.controlPoint0 =  mul(viewMtx, float4(i.controlPoint0,  1.0)).xyz;
    o.controlPoint1 =  mul(viewMtx, float4(i.controlPoint1,  1.0)).xyz;
    o.controlPoint2 =  mul(viewMtx, float4(i.controlPoint2,  1.0)).xyz;
    o.controlPoint3 =  mul(viewMtx, float4(i.controlPoint3,  1.0)).xyz;
    o.controlPoint4 =  mul(viewMtx, float4(i.controlPoint4,  1.0)).xyz;
    o.controlPoint5 =  mul(viewMtx, float4(i.controlPoint5,  1.0)).xyz;
    o.controlPoint6 =  mul(viewMtx, float4(i.controlPoint6,  1.0)).xyz;
    o.controlPoint7 =  mul(viewMtx, float4(i.controlPoint7,  1.0)).xyz;
    o.controlPoint8 =  mul(viewMtx, float4(i.controlPoint8,  1.0)).xyz;
    o.controlPoint9 =  mul(viewMtx, float4(i.controlPoint9,  1.0)).xyz;
    o.controlPoint10 = mul(viewMtx, float4(i.controlPoint10, 1.0)).xyz;
    o.controlPoint11 = mul(viewMtx, float4(i.controlPoint11, 1.0)).xyz;
    o.controlPoint12 = mul(viewMtx, float4(i.controlPoint12, 1.0)).xyz;
    o.controlPoint13 = mul(viewMtx, float4(i.controlPoint13, 1.0)).xyz;
    o.controlPoint14 = mul(viewMtx, float4(i.controlPoint14, 1.0)).xyz;
    o.controlPoint15 = mul(viewMtx, float4(i.controlPoint15, 1.0)).xyz;

    return o;
}