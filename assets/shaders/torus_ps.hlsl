struct VOut
{
    float4 pos : SV_POSITION;
    bool isSelected : BOOLEAN1;
    bool isCentroid : BOOLEAN2;
};

float4 main(VOut vout) : SV_Target
{
    if (vout.isSelected)
    {
        return float4(0.5, 0.5, 1.0, 1.0);
    }
    else if (vout.isCentroid)
    {
        return float4(0.0, 1.0f, 1.0f, 1.0f);
    }
    else
    {
        return float4(1.0, 1.0, 1.0, 1.0);
    }
}
