struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
};

struct VSOutput
{
	float4 pos : SV_POSITION;
	float2 texCoords : TEXCOORD0;
};

static const float3 plane[6] =
{
    float3(-1.f,  1.f, 0.f),
    float3( 1.f,  1.f, 0.f),
    float3(-1.f, -1.f, 0.f),
    float3(-1.f, -1.f, 0.f),
    float3( 1.f,  1.f, 0.f),
    float3( 1.f, -1.f, 0.f)
};


VSOutput main(uint vertexID : SV_VertexID)
{
	VSOutput o = (VSOutput) 0;

	o.pos = float4(plane[vertexID], 1.0);
	o.texCoords = 0.5 * (plane[vertexID].xy + float2(1.0, 1.0));
    o.texCoords.y = 1.0 - o.texCoords.y;

	return o;
}