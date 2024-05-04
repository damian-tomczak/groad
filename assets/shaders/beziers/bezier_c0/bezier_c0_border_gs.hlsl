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

struct GSOutput
{
    float4 pos : SV_POSITION;
};

struct VSOutput
{
    float3 controlPoint0 : CONTROL_POINTS_0;
    float3 controlPoint1 : CONTROL_POINTS_1;
    float3 controlPoint2 : CONTROL_POINTS_2;
    float3 controlPoint3 : CONTROL_POINTS_3;
};

[maxvertexcount(4)]
void main(point VSOutput input[1], inout LineStream<GSOutput> output
)
{
    GSOutput o = (GSOutput) 0;

    float4 position0 = mul(projMtx, float4(input[0].controlPoint0, 1.0));
    o.pos = position0;
    output.Append(o);

    float4 position1 = mul(projMtx, float4(input[0].controlPoint1, 1.0));
    o.pos = position1;
    output.Append(o);

    float4 position2 = mul(projMtx, float4(input[0].controlPoint2, 1.0));
    o.pos = position2;
    output.Append(o);

    float4 position3 = mul(projMtx, float4(input[0].controlPoint3, 1.0));
    o.pos = position3;
    output.Append(o);
}