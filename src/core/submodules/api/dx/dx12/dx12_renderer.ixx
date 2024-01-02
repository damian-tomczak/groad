module;

export module dx12renderer;
export import renderer;

export class DX12Renderer : public IRenderer
{
    DX12Renderer(std::weak_ptr<IWindow> pWindow) : IRenderer{pWindow}
    {
    }
};