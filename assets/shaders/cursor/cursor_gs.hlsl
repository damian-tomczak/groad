cbuffer CBuffer: register(b0)
{
    matrix model;
    matrix view;
    matrix invView;
    matrix proj;
    matrix invProj;
    int flags;
    int screenWidth;
    int screenHeight;
};

struct GeometryIn
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

struct GeometryOut
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

static float lineThickness = 0.025f;

[maxvertexcount(4)]
void main(line GeometryIn input[2], inout TriangleStream<GeometryOut> output)
{
    float4 p1 = input[0].pos;
    float4 p2 = input[1].pos;

    float2 dir = normalize(p2.xy - p1.xy);
    float2 normal = float2(-dir.y, dir.x);

    float4 offset = float4(normal * lineThickness, 0, 0);

    GeometryOut outv;

    outv.pos = p1 - offset;
    outv.color = input[0].color;
    output.Append(outv);

    outv.pos = p1 + offset;
    outv.color = input[0].color;
    output.Append(outv);

    outv.pos = p2 - offset;
    outv.color = input[1].color;
    output.Append(outv);

    outv.pos = p2 + offset;
    outv.color = input[1].color;
    output.Append(outv);

    output.RestartStrip();
}

