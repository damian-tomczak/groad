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
	float3 controlPoint0 : CONTROL_POINT0;
	float3 controlPoint1 : CONTROL_POINT1;
	float3 controlPoint2 : CONTROL_POINT2;
	float3 controlPoint3 : CONTROL_POINT3;
};

struct VSOutput
{
	float3 controlPoint0 : CONTROL_POINTS_0;
	float3 controlPoint1 : CONTROL_POINTS_1;
	float3 controlPoint2 : CONTROL_POINTS_2;
	float3 controlPoint3 : CONTROL_POINTS_3;
};

VSOutput main(VSInput i)
{
	VSOutput o = (VSOutput) 0;

	o.controlPoint0 = mul(viewMtx, float4(i.controlPoint0, 1.0)).xyz;
	o.controlPoint1 = mul(viewMtx, float4(i.controlPoint1, 1.0)).xyz;
	o.controlPoint2 = mul(viewMtx, float4(i.controlPoint2, 1.0)).xyz;
	o.controlPoint3 = mul(viewMtx, float4(i.controlPoint3, 1.0)).xyz;

	return o;
}