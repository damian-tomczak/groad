struct VOut {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};


float4 main(VOut vout) : SV_Target
{
    return vout.color;
}