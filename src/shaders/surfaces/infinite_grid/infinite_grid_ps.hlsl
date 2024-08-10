matrix modelMtx;
matrix viewMtx;
matrix invViewMtx;
matrix projMtx;
matrix invProjMtx;
matrix texMtx;
float4 cameraPos;
float4 color;
int flags;
int screenWidth;
int screenHeight;

#define EPSILON 0.5f
#define INFINITY 100.0f

struct VertexOutput {
    float4 position : SV_POSITION;
    float3 nearPoint : TEXCOORD0;
    float3 farPoint : TEXCOORD1;
};

struct PSOutput {
    float4 color : SV_Target;
    float depth : SV_Depth;
};

float4 grid(float3 fragPos3D, float scale)
{
    float2 coord = fragPos3D.xz * scale;
    float2 derivative = fwidth(coord);
    float2 grid = abs(frac(coord - 0.5) - 0.5) / derivative;
    float line2 = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1.0f);
    float minimumx = min(derivative.x, 1.0f);
    float4 color = float4(0.2, 0.2, 0.2, 1.0 - min(line2, 1.0f));

    if (fragPos3D.x > -0.1 * minimumx && fragPos3D.x < 0.1 * minimumx)
    {
        color.z = 1.0;
    }
    if (fragPos3D.z > -0.1 * minimumz && fragPos3D.z < 0.1 * minimumz)
    {
        color.x = 1.0;
    }

    return color;
}

float computeDepth(float3 pos)
{
    float4 clipSpacePos = mul(mul(projMtx, viewMtx), float4(pos, 1.0));
    return clipSpacePos.z / clipSpacePos.w;
}

PSOutput main(VertexOutput input)
{
    PSOutput output;

    float t = -input.nearPoint.y / (input.farPoint.y - input.nearPoint.y);
    float3 fragPos3D = input.nearPoint + t * (input.farPoint - input.nearPoint);

    output.depth = computeDepth(fragPos3D);
    output.color = grid(fragPos3D, 10.0) * (t > 0 ? 1.0 : 0.0);
    output.depth = ((abs(output.color.a - 1.0) < EPSILON) && (t > 0)) ? computeDepth(fragPos3D) : INFINITY;

    return output;
}
