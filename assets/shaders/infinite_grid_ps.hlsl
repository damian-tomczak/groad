struct PSInput {
    float4 pos : SV_POSITION;
};

float4 main(PSInput input) : SV_Target {
    return float4(1.0, 0.0, 0.0, 1.0);
}
