struct VertexInput
{
    uint vertexID : SV_VertexID;
};

struct PixelInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

PixelInput VS(VertexInput input)
{
    PixelInput output;

    float2 vertices[4] = { float2(-1, -1), float2(-1, 1), float2(1, -1), float2(1, 1) };
    float2 vertexPosition = vertices[input.vertexID % 4];

    float3 worldPosition = cameraPosition + float3(vertexPosition.x * squareSize, vertexPosition.y * squareSize, 0);

    output.pos = mul(viewProjectionMatrix, float4(worldPosition, 1.0));

    output.color = float4(1.0, 0.0, 0.0, 1.0);

    return output;
}
