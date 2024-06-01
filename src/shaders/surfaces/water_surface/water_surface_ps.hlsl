// TODO:
//#include "../../shaders.hpp"

matrix model;
matrix view;
matrix invView;
matrix proj;
matrix invProj;
matrix texMtx;
float4 cameraPos;
int flags;
int screenWidth;
int screenHeight;

Texture2D textureSampler : register(t0);
SamplerState samplerState : register(s0);

TextureCube cubemapSampler : register(t1);
SamplerState cubeSamplerState : register(s1);

struct VS_OUTPUT
{
    float2 psTexCoord : TEXCOORD0;
};

float3 cubemapCoordFromAnyPoint(float3 origin, float3 direction)
{
    float t = min(max((1 - origin.x) / direction.x, (-1 - origin.x) / direction.x),
                  min(max((1 - origin.y) / direction.y, (-1 - origin.y) / direction.y),
                      max((1 - origin.z) / direction.z, (-1 - origin.z) / direction.z)));
    return origin + t * direction;
}

static const float rzero = pow((1.33 - 1.0) / (1.33 + 1.0), 2.0);
float schlick(float cosfi)
{
    return rzero + (1.0 - rzero) * pow(1.0 - cosfi, 5.0);
}

float4 main(VS_OUTPUT input) : SV_Target
{
    float3 lightColor = float3(1, 1, 1);
    float3 lightVec = normalize(cameraPos.xyz);
    float3 cameraVec = normalize(cameraPos.xyz);

    float3 waterNormalTex = textureSampler.Sample(samplerState, input.psTexCoord).rgb;
    float3 waterNormal = normalize(2.0 * (waterNormalTex - float3(0.5, 0.5, 0.5)));

    float diffuse = clamp(dot(waterNormal, lightVec), 0.0, 1.0);
    float3 diffuseColor = diffuse * lightColor;

    float3 reflectVec = reflect(-lightVec, waterNormal);
    float specularStrength = 0.4;
    float specular = pow(max(dot(cameraVec, reflectVec), 0.0), 32);
    float3 specularColor = specularStrength * specular * lightColor;

    float2 positionOnCubePlane = (2.0 * input.psTexCoord) - 1.0;
    float3 positionInCube = float3(positionOnCubePlane.x, 0, positionOnCubePlane.y);

    float3 waterReflectionVec = reflect(-cameraVec, waterNormal);
    float3 fixedWaterReflectionVec = cubemapCoordFromAnyPoint(positionInCube, waterReflectionVec);
    float3 cubemapReflectionColor = cubemapSampler.Sample(cubeSamplerState, fixedWaterReflectionVec).rgb;

    float refractionRatio = 1.0 / 1.33;
    float3 waterRefractionVec = refract(-cameraVec, waterNormal, refractionRatio);
    float3 fixedWaterRefractionVec = cubemapCoordFromAnyPoint(positionInCube, waterRefractionVec);
    float3 cubemapRefractionColor = cubemapSampler.Sample(cubeSamplerState, fixedWaterRefractionVec).rgb;

    float fresnelFactor = schlick(abs(cameraVec.y));
    float3 reflRefrFactor = (1.0 - fresnelFactor) * cubemapRefractionColor + fresnelFactor * cubemapReflectionColor;

    float3 finalColor = reflRefrFactor * (diffuseColor + specularColor);
    return float4(finalColor, 1);
}
