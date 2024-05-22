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

    // Define the square's relative vertices
    float2 vertices[4] = { float2(-1, -1), float2(-1, 1), float2(1, -1), float2(1, 1) };
    float2 vertexPosition = vertices[input.vertexID % 4];

    // Convert vertexPosition to world space by scaling and translating
    float3 worldPosition = cameraPosition + float3(vertexPosition.x * squareSize, vertexPosition.y * squareSize, 0);

    // Construct the world position directly in front of the camera
    // No rotation is applied because the square should face the camera directly
    output.pos = mul(viewProjectionMatrix, float4(worldPosition, 1.0));

    // Simple coloration
    output.color = float4(1.0, 0.0, 0.0, 1.0);  // Red color

    return output;
}
