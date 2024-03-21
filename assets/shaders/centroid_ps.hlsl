struct VertexOut
{
    float4 pos : SV_POSITION;
    float PointSize : PSIZE;
};

float4 main(VertexOut vout) : SV_Target
{
    // Use input.PointSize here if needed, for example, to adjust brightness based on size
    return float4(0.0, 1.0, 1.0, 1.0); // White color
}
