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

TextureCube envMap : register(t0);
Texture2D normTexture : register(t1);
SamplerState samp : register(s0);

float3 normalMapping(float3 normal, float3 tangent, float3 tangentNormal)
{
    float3 binormal = normalize(cross(normal, tangent));

    tangent = cross(binormal, normal);

    float3x3 mtx = { tangent, binormal, normal };

    return mul(transpose(mtx), tangentNormal);
}

float3 intersectRay(float3 position, float3 direction)
{
    float3 tPrimary = max((1 - position) / direction, (-1 - position) / direction);

    return (position + min(tPrimary.x, min(tPrimary.y, tPrimary.z)) * direction);
}

float fresnel(float index1, float index2, float theta)
{
    float F0 = pow((index2 - index1) / (index2 + index1), 2);

    return F0 + (1 - F0) * pow(1 - theta, 5);
}

struct PSInput
{
    float4 position : SV_POSITION;
    float3 localPosition : POSITION0;
    float3 worldPosition : POSITION1;
};

float4 main(PSInput input) : SV_TARGET
{
    float index1 = 1.0f;
    float index2 = 4.0f / 3.0f;

    float3 viewVector = normalize(cameraPos.xyz - input.worldPosition);
    float2 textureCoordinate = (input.localPosition.xz + float2(1.0f, 1.0f)) / 2.0f;

    float3 normal = normalize(float3(0.0f, 1.0f, 0.0f));

    float3 dPdx = ddx(input.worldPosition);
    float3 dPdy = ddy(input.worldPosition);
    float2 dtdx = ddx(textureCoordinate);
    float2 dtdy = ddy(textureCoordinate);

    float3 tangent = normalize(-dPdx * dtdy.y + dPdy * dtdx.y);

    float3 normalFromTexture = normTexture.Sample(samp, textureCoordinate).xyz;
    normalFromTexture = 2 * normalFromTexture - float3(1.0f, 1.0f, 1.0f);

    float3 norm = normalize(normalMapping(normal, tangent, normalFromTexture));

    float3 reflectVector = reflect(-viewVector, norm);
    float3 refractVector;

    if (dot(norm, viewVector) > 0)
    {
        refractVector = refract(-viewVector, norm, index1 / index2);
    }
    else
    {
        norm = -norm;
        refractVector = refract(-viewVector, norm, index2 / index1);
    }

    float3 textureColorReflect = envMap.Sample(samp, intersectRay(input.localPosition, reflectVector)).xyz;
    float3 textureColorRefract = envMap.Sample(samp, intersectRay(input.localPosition, refractVector)).xyz;

    float3 color = float3(0.0f, 0.0f, 0.0f);

    if (any(refractVector))
    {
        float fresnelValue = fresnel(index1, index2, max(dot(norm, viewVector), 0.0f));
        color = lerp(textureColorRefract, textureColorReflect, fresnelValue);
    }
    else
    {
        color = textureColorReflect;
    }

    return float4(color, 1.0f);
}

