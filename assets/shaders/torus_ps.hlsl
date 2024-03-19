struct VOut
{
    float4 pos : SV_POSITION;
    bool isSelected : BOOLEAN;
};

float4 main(VOut vout) : SV_Target
{
    if (vout.isSelected)
    {
        return float4(0.5, 0.5, 1.0, 1.0);
    }
    else
    {
        return float4(1.0, 1.0, 1.0, 1.0);
    }
}
