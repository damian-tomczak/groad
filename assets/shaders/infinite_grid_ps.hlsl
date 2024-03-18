struct VertexOutput {
    float4 position : SV_POSITION;
    float3 nearPoint : TEXCOORD0;
    float3 farPoint : TEXCOORD1;
};

struct PSOutput {
    float4 color : SV_Target;
};

float4 grid(float3 fragPos3D, float scale)
{
    float2 coord = fragPos3D.xz * scale; // use the scale variable to set the distance between the lines
    float2 derivative = fwidth(coord);
    float2 grid = abs(frac(coord - 0.5) - 0.5) / derivative;
    float line2 = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1.0f);
    float minimumx = min(derivative.x, 1.0f);
    float4 color = float4(0.2, 0.2, 0.2, 1.0 - min(line2, 1.0f));
    // z axis
    if (fragPos3D.x > -0.1 * minimumx && fragPos3D.x < 0.1 * minimumx)
        color.z = 1.0;
    // x axis
    if (fragPos3D.z > -0.1 * minimumz && fragPos3D.z < 0.1 * minimumz)
        color.x = 1.0;
    return color;
}

PSOutput main(VertexOutput input) {
    PSOutput output;
    float t = -input.nearPoint.y / (input.farPoint.y - input.nearPoint.y);
    float3 fragPos3D = input.nearPoint + t * (input.farPoint - input.nearPoint);
    output.color = grid(fragPos3D, 10.0) * (t > 0 ? 1.0 : 0.0);
    return output;
}
