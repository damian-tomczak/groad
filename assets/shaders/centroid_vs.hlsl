cbuffer CBuffer: register(b0) {
    matrix model;
    matrix view;
    matrix invView;
    matrix proj;
    matrix invProj;
    int flags;
};

struct VertexOut
{
    float4 pos : SV_POSITION;
    float PointSize : PSIZE;
};

VertexOut main()
{
    VertexOut vout;
    matrix mvp = mul(model, view);
    mvp = mul(mvp, proj);
    vout.pos = mul(float4(1.0f, 1.0f, 1.0f, 1.0f), mvp);
    vout.PointSize = 400;
    return vout;
}
