module;

#include "utils.h"

#include <DirectXMath.h>
#include <d3d11.h>
#include <filesystem> // WO: std.core appears to be importing the filesystem incorrectly.

export module core.model_loader;
import dx11renderer;
import std.core;

export
{
    struct UV
    {
        float u;
        float v;
    };

    struct Vector
    {
        float x;
        float y;
        float z;
    };

    // clang-format off
    template<typename T>
    concept IsLayout = requires (T a) {
        { a.data };
        { a.layout };
        { a.stride } -> std::convertible_to<unsigned>;
        { a.offset } -> std::convertible_to<unsigned>;
    };
    // clang-format on

    struct PositionNormalLayout
    {
        struct Data
        {
            Vector position;
            Vector normal;
        } data;

        PositionNormalLayout(Vector pos, Vector norm) : data{pos, norm}
        {}

        inline const static std::array<D3D11_INPUT_ELEMENT_DESC, 2> layout
        {{
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        }};

        static constexpr unsigned stride = sizeof(Data);
        static constexpr unsigned offset = 0;
    };

    struct TexturedLayout
    {
        struct Data
        {
            Vector position;
            Vector normal;
            UV uv;
        } data;

        TexturedLayout(Vector pos, Vector norm, UV textureCoord) : data{pos, norm, textureCoord}
        {}

        inline const static std::array<D3D11_INPUT_ELEMENT_DESC, 3> layout
        {{
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Data, position), D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Data, normal), D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Data, uv), D3D11_INPUT_PER_VERTEX_DATA, 0}
        }};

        static constexpr unsigned stride = sizeof(Data);
        static constexpr unsigned offset = 0;
    };

    template <typename Layout>
    requires IsLayout<Layout>
    class Mesh : public NonCopyable
    {
    public:
        Mesh(IRenderer* pRenderer, std::pair<std::vector<Layout>, std::vector<short>>&& data)
            : mpRenderer{pRenderer}, mVertices{std::move(data.first)}, mIndices{std::move(data.second)}
        {
        }

        void init()
        {
            auto pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it

            const D3D11_BUFFER_DESC vbd{
                .ByteWidth = static_cast<unsigned>(sizeof(Layout) * mVertices.size()),
                .Usage = D3D11_USAGE_DYNAMIC,
                .BindFlags = D3D11_BIND_VERTEX_BUFFER,
                .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
                .MiscFlags = 0,
                .StructureByteStride = 0,
            };
            const D3D11_SUBRESOURCE_DATA vinitData{
                .pSysMem = mVertices.data(),
            };
            HR(pDX11Renderer->getDevice()->CreateBuffer(&vbd, &vinitData, &mpVertexBuffer));

            const D3D11_BUFFER_DESC ibd{
                .ByteWidth = static_cast<unsigned>(sizeof(short) * mIndices.size()),
                .Usage = D3D11_USAGE_DYNAMIC,
                .BindFlags = D3D11_BIND_INDEX_BUFFER,
                .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
                .MiscFlags = 0,
                .StructureByteStride = 0,
            };
            const D3D11_SUBRESOURCE_DATA initData{
                .pSysMem = mIndices.data(),
            };
            HR(pDX11Renderer->getDevice()->CreateBuffer(&ibd, &initData, &mpIndexBuffer));

            const D3D11_RASTERIZER_DESC restarizerDesc{
                .FillMode = D3D11_FILL_SOLID,
                .CullMode = D3D11_CULL_BACK,
                .FrontCounterClockwise = false,
                .DepthBias = 0,
                .DepthBiasClamp = 0.0f,
                .SlopeScaledDepthBias = 0.0f,
                .DepthClipEnable = true,
                .ScissorEnable = false,
                .MultisampleEnable = false,
                .AntialiasedLineEnable = false,
            };

            HR(pDX11Renderer->getDevice()->CreateRasterizerState(&restarizerDesc, mpRasterizer.GetAddressOf()));
        }

        void draw()
        {
            auto pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it

            pDX11Renderer->getContext()->RSSetState(mpRasterizer.Get());

            pDX11Renderer->getContext()->IASetVertexBuffers(0, 1, mpVertexBuffer.GetAddressOf(),
                                                            &Layout::stride, &Layout::offset);

            pDX11Renderer->getContext()->IASetIndexBuffer(mpIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

            pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            pDX11Renderer->getContext()->DrawIndexed(static_cast<unsigned>(mIndices.size()), 0, 0);
        }

    private:
        ComPtr<ID3D11Buffer> mpIndexBuffer;
        ComPtr<ID3D11Buffer> mpVertexBuffer;
        ComPtr<ID3D11InputLayout> mpInputLayout;
        ComPtr<ID3D11RasterizerState> mpRasterizer;

        std::vector<Layout> mVertices;
        std::vector<short> mIndices;

        IRenderer* mpRenderer;
    };

    enum class FileType
    {
        UNKNOWN,
        TXT,
        OBJ,
    };

    class MeshLoader
    {
    public:
        template <typename Layout>
        requires IsLayout<Layout>
        static Mesh<Layout> loadMesh(IRenderer* mpRenderer, const fs::path& path);

        static FileType recognizeFileType(const fs::path& path);

    private:
        static void loadMeshFromTxt(std::ifstream& file, std::vector<Vector>& positions,
                                             std::vector<Vector>& normals, std::vector<UV>& uvs, std::vector<short>& indices);
        static void loadMeshFromOBJ(std::ifstream& file, std::vector<Vector>& positions,
                                                           std::vector<Vector>& normals, std::vector<UV>& uvs, std::vector<short>& indices);
    };

    // clang-format off

    std::vector<PositionNormalLayout> boxVertices(float width, float height, float depth)
    {
        return
        {
            //Front face
            { Vector{-0.5f*width, -0.5f*height, -0.5f*depth}, Vector{0.0f, 0.0f, 1.0f} },
            { Vector{+0.5f*width, -0.5f*height, -0.5f*depth}, Vector{0.0f, 0.0f, 1.0f} },
            { Vector{+0.5f*width, +0.5f*height, -0.5f*depth}, Vector{0.0f, 0.0f, 1.0f} },
            { Vector{-0.5f*width, +0.5f*height, -0.5f*depth}, Vector{0.0f, 0.0f, 1.0f} },

            //Back face
            { Vector{+0.5f*width, -0.5f*height, +0.5f*depth}, Vector{0.0f, 0.0f, -1.0f} },
            { Vector{-0.5f*width, -0.5f*height, +0.5f*depth}, Vector{0.0f, 0.0f, -1.0f} },
            { Vector{-0.5f*width, +0.5f*height, +0.5f*depth}, Vector{0.0f, 0.0f, -1.0f} },
            { Vector{+0.5f*width, +0.5f*height, +0.5f*depth}, Vector{0.0f, 0.0f, -1.0f} },

            //Left face
            { Vector{-0.5f*width, -0.5f*height, +0.5f*depth}, Vector{1.0f, 0.0f, 0.0f} },
            { Vector{-0.5f*width, -0.5f*height, -0.5f*depth}, Vector{1.0f, 0.0f, 0.0f} },
            { Vector{-0.5f*width, +0.5f*height, -0.5f*depth}, Vector{1.0f, 0.0f, 0.0f} },
            { Vector{-0.5f*width, +0.5f*height, +0.5f*depth}, Vector{1.0f, 0.0f, 0.0f} },

            //Right face
            { Vector{+0.5f*width, -0.5f*height, -0.5f*depth}, Vector{-1.0f, 0.0f, 0.0f} },
            { Vector{+0.5f*width, -0.5f*height, +0.5f*depth}, Vector{-1.0f, 0.0f, 0.0f} },
            { Vector{+0.5f*width, +0.5f*height, +0.5f*depth}, Vector{-1.0f, 0.0f, 0.0f} },
            { Vector{+0.5f*width, +0.5f*height, -0.5f*depth}, Vector{-1.0f, 0.0f, 0.0f} },

            //Bottom face
            { Vector{-0.5f*width, -0.5f*height, +0.5f*depth}, Vector{0.0f, 1.0f, 0.0f} },
            { Vector{+0.5f*width, -0.5f*height, +0.5f*depth}, Vector{0.0f, 1.0f, 0.0f} },
            { Vector{+0.5f*width, -0.5f*height, -0.5f*depth}, Vector{0.0f, 1.0f, 0.0f} },
            { Vector{-0.5f*width, -0.5f*height, -0.5f*depth}, Vector{0.0f, 1.0f, 0.0f} },

            //Top face
            { Vector{-0.5f*width, +0.5f*height, -0.5f*depth}, Vector{0.0f, -1.0f, 0.0f} },
            { Vector{+0.5f*width, +0.5f*height, -0.5f*depth}, Vector{0.0f, -1.0f, 0.0f} },
            { Vector{+0.5f*width, +0.5f*height, +0.5f*depth}, Vector{0.0f, -1.0f, 0.0f} },
            { Vector{-0.5f*width, +0.5f*height, +0.5f*depth}, Vector{0.0f, -1.0f, 0.0f} },
        };
    }

    std::vector<short> boxIndices()
    {
        return
        {
            0, 2, 1,  0, 3, 2,
            4, 6, 5,  4, 7, 6,
            8, 10,9,  8, 11,10,
            12,14,13, 12,15,14,
            16,18,17, 16,19,18,
            20,22,21, 20,23,22,
        };
    }

    // clang-format on
};

template <typename Layout>
requires IsLayout<Layout>
Mesh<Layout> MeshLoader::loadMesh(IRenderer* mpRenderer, const fs::path& filePath)
{
    std::vector<Layout> vertices;

    std::vector<Vector> positions;
    std::vector<Vector> normals;
    std::vector<UV> uvs;
    std::vector<short> indices;

    std::ifstream file{filePath};

    ASSERT(file.good());

    const FileType fileType = recognizeFileType(filePath);
    ASSERT(fileType != FileType::UNKNOWN)
    switch (fileType)
    {
    case FileType::TXT:
        loadMeshFromTxt(file, positions, normals, uvs, indices);
        break;
    case FileType::OBJ:
        loadMeshFromOBJ(file, positions, normals, uvs, indices);
        break;
    }

    vertices.reserve(positions.size());

    for (unsigned i = 0; i < positions.size(); ++i)
    {
        vertices.emplace_back(positions[i], normals[i], uvs[i]);
    }

    return Mesh<Layout>{mpRenderer, {vertices, indices}};
}

module :private;

void MeshLoader::loadMeshFromTxt(std::ifstream& file, std::vector<Vector>& positions,
                                     std::vector<Vector>& normals, std::vector<UV>& uvs, std::vector<short>& indices)
{
    int linesCount;
    file >> linesCount;

    for (int i = 0; i < linesCount; ++i)
    {
        float vertices[3];

        file >> vertices[0] >> vertices[1] >> vertices[2];
        positions.emplace_back(vertices[0], vertices[1], vertices[2]);

        float fileNormals[3];

        file >> fileNormals[0] >> fileNormals[1] >> fileNormals[2];
        normals.emplace_back(fileNormals[0], fileNormals[1], fileNormals[2]);

        float textureCoord[2];

        file >> textureCoord[0] >> textureCoord[1];
        uvs.emplace_back(textureCoord[0], textureCoord[1]);
    }

    file >> linesCount;

    for (int i = 0; i < linesCount; ++i)
    {
        short fileIndices[3];

        file >> fileIndices[0] >> fileIndices[1] >> fileIndices[2];
        indices.insert(indices.end(), {fileIndices[0], fileIndices[1], fileIndices[2]});
    }
}

void MeshLoader::loadMeshFromOBJ(std::ifstream& file, std::vector<Vector>& positions,
                                                        std::vector<Vector>& normals, std::vector<UV>& uvs, std::vector<short>& indices)
{
    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v")
        {
            float vertices[3];

            file >> vertices[0] >> vertices[1] >> vertices[2];
            positions.emplace_back(vertices[0], vertices[1], vertices[2]);
        }
        else if (prefix == "vt")
        {
            float textureCoord[2];

            file >> textureCoord[0] >> textureCoord[1];
            uvs.emplace_back(textureCoord[0], textureCoord[1]);
        }
        else if (prefix == "f")
        {
            short fileIndices[3];
            short textureIndices[3];
            short normalIndices[3];
            char slash;

            for (int i = 0; i < 3; ++i)
            {
                iss >> fileIndices[i] >> slash >> textureIndices[i] >> slash >> normalIndices[i];
            }

            indices.insert(indices.end(), {fileIndices[0], fileIndices[1], fileIndices[2]});
        }
    }
}

FileType MeshLoader::recognizeFileType(const fs::path& path)
{
    const fs::path extension = path.extension();

    if (extension == ".txt")
    {
        return FileType::TXT;
    }
    else if (extension == ".obj")
    {
        return FileType::OBJ;
    }

    return FileType::UNKNOWN;
}

