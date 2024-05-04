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

float4 main() : SV_Target
{
    bool isSelected = flags & 1;

    if (isSelected)
    {
        return float4(0.5, 0.5, 1.0, 1.0);
    }
    else
    {
        return float4(1.0, 0.0, 0.0, 1.0);
    }
}
