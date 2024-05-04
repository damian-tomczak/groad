cbuffer CBuffer : register(b0) {
    matrix model;
    matrix view;
    matrix invView;
    matrix proj;
    matrix invProj;
    int flags;
    int screenWidth;
    int screenHeight;
};

struct VOut
{
    float4 pos : SV_POSITION;
};

float4 main(VOut vout) : SV_Target
{
    bool isSelected = flags & 1;
    bool isPivot    = flags & 2;

    if (isSelected)
    {
        return float4(0.5, 0.5, 1.0, 1.0);
    }
    else if (isPivot)
    {
        return float4(0.0, 1.0f, 1.0f, 1.0f);
    }
    else
    {
        return float4(1.0, 1.0, 1.0, 1.0);
    }
}
