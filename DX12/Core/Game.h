#pragma once

#include "Interfaces/EngineEventHandlers.h"
#include "Engine.h"
#include "Events.h"
#include <DirectXMath.h>

class Game
    : public std::enable_shared_from_this<Game>,
      public IStartupEventHandler,
      public IUpdateEventHandler,
      public IRenderEventHandler,
      public IPaintEventHandler
{
public:
    Game();
    Game& operator= (const Game&) = delete;

    virtual void Startup() override;
    virtual void Update(double deltaTime) override;
    virtual void Render() override;
    virtual void Paint() override;

protected:
    void OnKeyEvent(const KeyEventArgs &event);
    void OnResizeEvent(const ResizeEventArgs& event);
    void OnWindowDestroyed();

    void ResizeDepthBuffer(uint32_t width, uint32_t height);

private:
    void CreateInstanceBuffer();
    void UpdateInstanceData();
    void UpdateInstanceBuffer(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList);

    void InitImGui();

    std::shared_ptr<Window> m_window;

    uint32_t m_windowWidth;
    uint32_t m_windowHeight;

    std::vector<uint64_t> m_fenceValues;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_instanceBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_instanceUploadBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_instanceBufferView;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_depthBuffer;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSVHeap;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;

    float m_FoV;

    DirectX::XMMATRIX m_viewMatrix;
    DirectX::XMMATRIX m_projectionMatrix;

    struct InstanceData
    {
        DirectX::XMMATRIX model;
    };
    std::unique_ptr<InstanceData[]> m_instanceData;

    double m_currentTime = 0;
};