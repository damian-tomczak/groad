matrix model;
matrix view;
matrix invView;
matrix proj;
matrix invProj;
matrix texMtx;
float4 cameraPos;
float4 color;
int flags;
int screenWidth;
int screenHeight;

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

static const float2 quad[6] =
{
    float2(-1.0,  1.0),
    float2( 1.0,  1.0),
    float2(-1.0, -1.0),
    float2(-1.0, -1.0),
    float2( 1.0,  1.0),
    float2( 1.0, -1.0)
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output = (VSOutput)0;

    const float2 local = quad[vertexID];
    output.position = mul(model, float4(local, 0.0, 1.0));
    output.texCoord = float2(0.5 * (local.x + 1.0), 1.0 - 0.5 * (local.y + 1.0));

    return output;
}
