matrix modelMtx;
matrix viewMtx;
matrix invView;
matrix projMtx;
matrix invProj;
matrix texMtx;
float4 cameraPos;
float4 color;
int flags;
int screenWidth;
int screenHeight;

float4 main() : SV_Target
{
    return color;
}
