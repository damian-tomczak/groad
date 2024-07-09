matrix modelMtx;
matrix viewMtx;
matrix invViewMtx;
matrix projMtx;
matrix invProjMtx;
matrix texMtx;
float4 cameraPos;
float4 color;
int flags;
int screenWidth;
int screenHeight;

struct VSInput {
    float3 pos : POSITION;
    float3 norm : NORMAL0;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float3 tex : TEXCOORD0;
};

struct VertexPositionNormal {
    float3 pos;
    float3 norm;
};

#define NUM_VERTICES 36

// TODO: I have no fucking idea why bufferless approach doesn't work
// TODO: I know I was using drawIndexed instead of raw draw... fix it
// static const VertexPositionNormal box[NUM_VERTICES] = {
//     // Front face
//     {{-0.5, -0.5, -0.5}, {0, 0, 1}},
//     {{ 0.0, -0.5, -0.5}, {0, 0, 1}},
//     {{ 0.5, -0.5, -0.5}, {0, 0, 1}},
//     {{-0.5,  0.0, -0.5}, {0, 0, 1}},
//     {{ 0.0,  0.0, -0.5}, {0, 0, 1}},
//     {{ 0.5,  0.0, -0.5}, {0, 0, 1}},

//     // Back face
//     {{ 0.5, -0.5,  0.5}, {0, 0, -1}},
//     {{ 0.0, -0.5,  0.5}, {0, 0, -1}},
//     {{-0.5, -0.5,  0.5}, {0, 0, -1}},
//     {{ 0.5,  0.0,  0.5}, {0, 0, -1}},
//     {{ 0.0,  0.0,  0.5}, {0, 0, -1}},
//     {{-0.5,  0.0,  0.5}, {0, 0, -1}},

//     // Left face
//     {{-0.5, -0.5,  0.5}, {1, 0, 0}},
//     {{-0.5, -0.5,  0.0}, {1, 0, 0}},
//     {{-0.5, -0.5, -0.5}, {1, 0, 0}},
//     {{-0.5,  0.0,  0.5}, {1, 0, 0}},
//     {{-0.5,  0.0,  0.0}, {1, 0, 0}},
//     {{-0.5,  0.0, -0.5}, {1, 0, 0}},

//     // Right face
//     {{ 0.5, -0.5, -0.5}, {-1, 0, 0}},
//     {{ 0.5, -0.5,  0.0}, {-1, 0, 0}},
//     {{ 0.5, -0.5,  0.5}, {-1, 0, 0}},
//     {{ 0.5,  0.0, -0.5}, {-1, 0, 0}},
//     {{ 0.5,  0.0,  0.0}, {-1, 0, 0}},
//     {{ 0.5,  0.0,  0.5}, {-1, 0, 0}},

//     // Bottom face
//     {{-0.5, -0.5,  0.5}, {0, 1, 0}},
//     {{ 0.0, -0.5,  0.5}, {0, 1, 0}},
//     {{ 0.5, -0.5,  0.5}, {0, 1, 0}},
//     {{-0.5, -0.5,  0.0}, {0, 1, 0}},
//     {{ 0.0, -0.5,  0.0}, {0, 1, 0}},
//     {{ 0.5, -0.5,  0.0}, {0, 1, 0}},

//     // Top face
//     {{-0.5,  0.0, -0.5}, {0, -1, 0}},
//     {{ 0.0,  0.0, -0.5}, {0, -1, 0}},
//     {{ 0.5,  0.0, -0.5}, {0, -1, 0}},
//     {{-0.5,  0.0,  0.0}, {0, -1, 0}},
//     {{ 0.0,  0.0,  0.0}, {0, -1, 0}},
//     {{ 0.5,  0.0,  0.0}, {0, -1, 0}}
// };

PSInput main(VSInput i)
{
    PSInput o;

    o.tex = normalize(i.pos);

    o.pos = mul(modelMtx, float4(i.pos, 1.0f));
    o.pos = mul(viewMtx, o.pos);
    o.pos = mul(projMtx, o.pos);

    return o;
}
