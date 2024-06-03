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

float4 main() : SV_Target
{
    bool isSelected = flags & 1;
    bool isBorder = flags & 2;

    if (isSelected)
    {
        return float4(0.5, 0.5, 1.0, 1.0);
    }
    else if (isBorder)
    {
        return float4(0.0, 1.0, 0.0, 1.0);
    }
    else
    {
        return float4(1.0, 0.0, 0.0, 1.0);
    }
}
