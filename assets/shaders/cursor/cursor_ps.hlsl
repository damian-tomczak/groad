struct PixelShaderOut
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

float4 main(PixelShaderOut output) : SV_Target
{
    return output.color
}
