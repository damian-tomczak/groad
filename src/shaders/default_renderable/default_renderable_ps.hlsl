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

struct VOut
{
    float4 pos : SV_POSITION;
};

float4 main(VOut vout) : SV_Target
{
    return color;
}
