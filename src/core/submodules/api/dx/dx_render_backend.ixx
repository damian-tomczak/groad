module;

#include "utils.h"

#include <d3d11.h>
#include <d3d12.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dxgiformat.h>
#include <wrl/client.h>

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

export module dx.render_backend;

import std;

using Microsoft::WRL::ComPtr;

export enum class PrimitiveTopology
{
    TriangleList,
    TriangleStrip,
    LineList,
    LineStrip,
    PointList,
};

constexpr D3D_PRIMITIVE_TOPOLOGY toD3DTopology(PrimitiveTopology topology)
{
    switch (topology)
    {
    case PrimitiveTopology::TriangleList:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case PrimitiveTopology::TriangleStrip:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case PrimitiveTopology::LineList:
        return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case PrimitiveTopology::LineStrip:
        return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case PrimitiveTopology::PointList:
        return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    default:
        ASSERT(false);
        return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
}

constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE toD3D12TopologyType(PrimitiveTopology topology)
{
    switch (topology)
    {
    case PrimitiveTopology::TriangleList:
    case PrimitiveTopology::TriangleStrip:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    case PrimitiveTopology::LineList:
    case PrimitiveTopology::LineStrip:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    case PrimitiveTopology::PointList:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    default:
        ASSERT(false);
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    }
}

export struct ShaderSource
{
    std::string_view code;
    std::string_view entryPoint;
    std::string_view target;
};

export struct InputElementDesc
{
    std::string semanticName;
    UINT semanticIndex;
    DXGI_FORMAT format;
    UINT inputSlot;
    UINT alignedByteOffset;
};

export struct GraphicsPipelineDesc
{
    ShaderSource vertexShader;
    ShaderSource pixelShader;
    std::vector<InputElementDesc> inputElements;
    PrimitiveTopology topology = PrimitiveTopology::TriangleList;
    DXGI_FORMAT renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
};

export struct VertexBufferDesc
{
    std::span<const std::byte> data;
    uint32_t stride;
};

export struct DrawParams
{
    uint32_t vertexCount;
    uint32_t startVertex = 0u;
    uint32_t instanceCount = 1u;
};

export class IRenderBackend
{
public:
    virtual ~IRenderBackend() = default;

    virtual void createPipeline(const GraphicsPipelineDesc& desc) = 0;
    virtual void createVertexBuffer(const VertexBufferDesc& desc) = 0;
    virtual void draw(const DrawParams& params) = 0;
};

export inline ComPtr<ID3DBlob> compileShader(const ShaderSource& shader)
{
    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;

    const HRESULT hr = D3DCompile(shader.code.data(), shader.code.size(), nullptr, nullptr, nullptr,
                                  shader.entryPoint.data(), shader.target.data(), 0u, 0u,
                                  shaderBlob.GetAddressOf(), errorBlob.GetAddressOf());

    if (FAILED(hr))
    {
        std::string errorMessage = "Failed to compile shader";
        if (errorBlob != nullptr)
        {
            errorMessage += std::string{": "} +
                            static_cast<const char*>(errorBlob->GetBufferPointer());
        }

        ERR(errorMessage.c_str());
    }

    return shaderBlob;
}

export constexpr D3D12_PRIMITIVE_TOPOLOGY toD3D12Topology(PrimitiveTopology topology)
{
    switch (topology)
    {
    case PrimitiveTopology::TriangleList:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case PrimitiveTopology::TriangleStrip:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case PrimitiveTopology::LineList:
        return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case PrimitiveTopology::LineStrip:
        return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case PrimitiveTopology::PointList:
        return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    default:
        ASSERT(false);
        return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
}
