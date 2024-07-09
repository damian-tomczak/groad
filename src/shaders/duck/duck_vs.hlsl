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

static const float3 light_position = float3(2.5f, 2.5f, 0.0f);

struct VSInput {
    float3 pos : POSITION0;
    float3 normal : NORMAL0;
    float2 tex : TEXCOORD0;
};

struct PSInput {
    float4 pos : SV_POSITION;
    float3 worldPos : POSITION0;
    float3 light_position: POSITION1;
    float3 normal : NORMAL0;
    float2 tex : TEXCOORD0;
};

PSInput main(VSInput i) {
    PSInput o;

    o.tex = i.tex;

    o.normal = mul(modelMtx, float4(i.normal, 0.0f)).xyz;

    o.pos = float4(i.pos, 1.0f);
    o.worldPos = mul(modelMtx, o.pos).xyz;

    o.pos = mul(modelMtx, o.pos);
    o.pos = mul(viewMtx, o.pos);
    o.pos = mul(projMtx, o.pos);

    o.light_position = light_position;

    return o;
}
