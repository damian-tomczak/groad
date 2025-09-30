module;

export module dx12renderer;

export import dx11renderer;

export class DX12Renderer final : public DX11Renderer
{
public:
    using DX11Renderer::DX11Renderer;
};
