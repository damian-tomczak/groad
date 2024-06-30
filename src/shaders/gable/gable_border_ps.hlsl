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

struct PSInput
{
    float4 pos : SV_POSITION;
};

float4 main(PSInput i) : SV_TARGET
{
    return float4(0.8, 0.0, 0.0, 1.0);
;
}